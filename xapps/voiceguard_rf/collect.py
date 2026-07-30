#!/usr/bin/env python3
"""Collect real 8-video/2-voice policy outcomes from an active O-RAN run."""

from __future__ import annotations

import argparse
import csv
import json
import time
from pathlib import Path
from statistics import mean, median
from typing import Any
from urllib.error import HTTPError
from urllib.request import Request, urlopen

from common import (
    FEATURE_NAMES,
    POLICY_ORDER,
    POLICY_SCALES,
    SLA,
    VIDEO_UES,
    VOICE_UES,
    extract_features,
    median_features,
    voice_sla_ok,
    write_traffic_scale,
)


def request_json(url: str, method: str = "GET", body: dict[str, Any] | None = None) -> Any:
    data = json.dumps(body).encode() if body is not None else None
    request = Request(
        url,
        data=data,
        method=method,
        headers={"Accept": "application/json", "Content-Type": "application/json"},
    )
    try:
        with urlopen(request, timeout=15) as response:
            return json.loads(response.read() or b"null")
    except HTTPError as error:
        raise RuntimeError(f"{method} {url}: {error.code} {error.read().decode()}") from error


def jobs(manager: str, run_id: str) -> list[dict[str, Any]]:
    return request_json(f"{manager}/api/runs/{run_id}/traffic")


def metric_values(manager: str, run_id: str, metric: str) -> dict[str, float]:
    payload = request_json(
        f"{manager}/api/runs/{run_id}/metrics/query?metric={metric}"
    )
    return {
        str(item.get("metric", {}).get("ue")): float(item["value"][1])
        for item in payload.get("data", {}).get("result", [])
        if item.get("metric", {}).get("ue") and item.get("value")
    }


def active_ues(current: list[dict[str, Any]]) -> set[str]:
    return {
        str(job["ue"])
        for job in current
        if job.get("status") in {"QUEUED", "RUNNING", "STOP_REQUESTED"}
    }


def start_ues(manager: str, run_id: str, ues: list[str]) -> None:
    current = active_ues(jobs(manager, run_id))
    requested = [ue for ue in ues if ue not in current]
    if requested:
        request_json(
            f"{manager}/api/runs/{run_id}/traffic/batch",
            "POST",
            {"ues": requested},
        )


def stop_ues(manager: str, run_id: str, ues: list[str]) -> None:
    for job in jobs(manager, run_id):
        if (
            job.get("ue") in ues
            and job.get("status") in {"QUEUED", "RUNNING", "STOP_REQUESTED"}
        ):
            request_json(
                f"{manager}/api/runs/{run_id}/traffic/{job['id']}",
                "DELETE",
            )


def wait_for_active(manager: str, run_id: str, expected: set[str], timeout: float = 25) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        current = active_ues(jobs(manager, run_id))
        if expected <= current:
            return
        time.sleep(1)
    raise RuntimeError(f"timed out waiting for active UEs: {sorted(expected)}")


def sample_window(
    manager: str,
    run_id: str,
    seconds: int,
    *,
    scenario_id: str,
    policy: str,
) -> list[dict[str, Any]]:
    samples = []
    for sample_index in range(seconds):
        features, ue_metrics = extract_features(
            jobs(manager, run_id),
            metric_values(manager, run_id, "ue_rx_bps"),
        )
        samples.append(
            {
                "timestamp": time.time(),
                "scenario_id": scenario_id,
                "policy": policy,
                "sample_index": sample_index,
                **features,
                "sla_ok": int(voice_sla_ok(features)),
                "ue_metrics": json.dumps(ue_metrics, ensure_ascii=False, sort_keys=True),
            }
        )
        time.sleep(1)
    return samples


def summarize_policy(samples: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "samples": len(samples),
        "sla_success_ratio": mean(float(row["sla_ok"]) for row in samples),
        "voice_delivery_ratio_median": median(
            float(row["voice_delivery_ratio"]) for row in samples
        ),
        "voice_loss_percent_median": median(
            float(row["voice_loss_percent"]) for row in samples
        ),
        "voice_jitter_ms_median": median(
            float(row["voice_jitter_ms"]) for row in samples
        ),
        "voice_rtt_p95_ms_median": median(
            float(row["voice_rtt_p95_ms"]) for row in samples
        ),
        "video_delivered_mbps_median": median(
            float(row["video_delivered_mbps"]) for row in samples
        ),
    }


def acceptable(summary: dict[str, Any]) -> bool:
    return (
        summary["sla_success_ratio"] >= 0.67
        and summary["voice_delivery_ratio_median"] >= SLA["delivery_ratio_min"]
        and summary["voice_loss_percent_median"] <= SLA["loss_percent_max"]
        and summary["voice_jitter_ms_median"] <= SLA["jitter_ms_max"]
        and summary["voice_rtt_p95_ms_median"] <= SLA["rtt_p95_ms_max"]
    )


def write_csv(path: Path, rows: list[dict[str, Any]], columns: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(
            output,
            fieldnames=columns,
            extrasaction="ignore",
            lineterminator="\n",
        )
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--manager-url", default="http://127.0.0.1:8088")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--control-file", type=Path, required=True)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--warmup-seconds", type=int, default=3)
    parser.add_argument("--sample-seconds", type=int, default=6)
    arguments = parser.parse_args()
    manager = arguments.manager_url.rstrip("/")

    run = request_json(f"{manager}/api/runs/{arguments.run_id}")
    if run.get("state") != "RUNNING":
        raise SystemExit(f"run must be RUNNING, got {run.get('state')}")
    configuration = request_json(
        f"{manager}/api/runs/{arguments.run_id}/traffic/config"
    )
    types = {
        item["ue"]: (item.get("traffic", {}).get("flows") or [{}])[0].get("type")
        for item in configuration.get("ues", [])
    }
    if any(types.get(ue) != "short_video" for ue in VIDEO_UES):
        raise SystemExit("UE1..UE8 must be configured as short_video")
    if any(types.get(ue) != "rtp_voice" for ue in VOICE_UES):
        raise SystemExit("UE9..UE10 must be configured as rtp_voice")

    arguments.output_dir.mkdir(parents=True, exist_ok=True)
    raw_rows: list[dict[str, Any]] = []
    training_rows: list[dict[str, Any]] = []
    policy_results: list[dict[str, Any]] = []
    combinations = (("ue9",), ("ue10",), ("ue9", "ue10"))
    try:
        write_traffic_scale(arguments.control_file, 1.0, "rf_collection_start")
        start_ues(manager, arguments.run_id, list(VIDEO_UES))
        wait_for_active(manager, arguments.run_id, set(VIDEO_UES))
        time.sleep(arguments.warmup_seconds)

        for round_index in range(1, arguments.rounds + 1):
            for combination in combinations:
                scenario_id = (
                    f"round-{round_index}-"
                    + "-".join(combination)
                    + f"-{int(time.time())}"
                )
                stop_ues(
                    manager,
                    arguments.run_id,
                    [ue for ue in VOICE_UES if ue not in combination],
                )
                time.sleep(1)
                start_ues(manager, arguments.run_id, list(combination))
                wait_for_active(
                    manager,
                    arguments.run_id,
                    set(VIDEO_UES) | set(combination),
                )
                time.sleep(arguments.warmup_seconds)

                outcomes: dict[str, dict[str, Any]] = {}
                baseline_samples: list[dict[str, Any]] = []
                for policy in POLICY_ORDER:
                    scale = POLICY_SCALES[policy]
                    print(
                        f"{scenario_id}: {policy} ({scale * 100:.0f}%)",
                        flush=True,
                    )
                    write_traffic_scale(
                        arguments.control_file,
                        scale,
                        f"rf_collect_{scenario_id}_{policy}",
                    )
                    time.sleep(arguments.warmup_seconds)
                    samples = sample_window(
                        manager,
                        arguments.run_id,
                        arguments.sample_seconds,
                        scenario_id=scenario_id,
                        policy=policy,
                    )
                    raw_rows.extend(samples)
                    outcomes[policy] = summarize_policy(samples)
                    if policy == "EQUAL_100":
                        baseline_samples = samples

                label = next(
                    (policy for policy in POLICY_ORDER if acceptable(outcomes[policy])),
                    POLICY_ORDER[-1],
                )
                for sample in baseline_samples:
                    training_rows.append(
                        {
                            "scenario_id": scenario_id,
                            **{name: sample[name] for name in FEATURE_NAMES},
                            "label": label,
                        }
                    )
                policy_results.append(
                    {
                        "scenario_id": scenario_id,
                        "round": round_index,
                        "active_voice_ues": list(combination),
                        "label": label,
                        "outcomes": outcomes,
                        "baseline_features": median_features(baseline_samples),
                    }
                )
                print(f"{scenario_id}: selected {label}", flush=True)
    finally:
        write_traffic_scale(arguments.control_file, 1.0, "rf_collection_finished")
        stop_ues(manager, arguments.run_id, list(VOICE_UES))

    raw_columns = [
        "timestamp",
        "scenario_id",
        "policy",
        "sample_index",
        *FEATURE_NAMES,
        "sla_ok",
        "ue_metrics",
    ]
    training_columns = ["scenario_id", *FEATURE_NAMES, "label"]
    write_csv(arguments.output_dir / "raw_samples.csv", raw_rows, raw_columns)
    write_csv(arguments.output_dir / "training.csv", training_rows, training_columns)
    (arguments.output_dir / "policy_results.json").write_text(
        json.dumps(
            {
                "run_id": arguments.run_id,
                "created_at": time.time(),
                "method": "evaluate all candidate policies and label the least restrictive SLA-compliant policy",
                "sla": SLA,
                "policy_scales": POLICY_SCALES,
                "scenarios": policy_results,
            },
            ensure_ascii=False,
            indent=2,
        )
        + "\n"
    )
    print(
        json.dumps(
            {
                "raw_samples": len(raw_rows),
                "training_rows": len(training_rows),
                "scenarios": len(policy_results),
                "output_dir": str(arguments.output_dir),
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
