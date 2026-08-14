#!/usr/bin/env python3
"""Collect labelled 2-video/1-voice policy outcomes from a real 3 UE run."""

from __future__ import annotations

import argparse
import csv
import json
import random
import time
from pathlib import Path
from statistics import mean, median
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

from common import (
    FEATURE_NAMES,
    POLICY_ORDER,
    POLICY_SCALES,
    SLA,
    VIDEO_UES,
    VOICE_UES,
    atomic_write_json,
    extract_features,
    median_features,
    voice_sla_ok,
    write_video_policy,
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
        with urlopen(request, timeout=20) as response:
            return json.loads(response.read() or b"null")
    except HTTPError as error:
        raise RuntimeError(f"{method} {url}: {error.code} {error.read().decode()}") from error


def jobs(manager: str, run_id: str) -> list[dict[str, Any]]:
    return request_json(f"{manager}/api/runs/{run_id}/traffic")


def metric_values(manager: str, run_id: str, metric: str) -> dict[str, float]:
    try:
        payload = request_json(f"{manager}/api/runs/{run_id}/metrics/query?metric={metric}")
    except (RuntimeError, URLError, TimeoutError):
        return {}
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
    requested = [ue for ue in ues if ue not in active_ues(jobs(manager, run_id))]
    if requested:
        request_json(f"{manager}/api/runs/{run_id}/traffic/batch", "POST", {"ues": requested})


def stop_ues(manager: str, run_id: str, ues: list[str]) -> None:
    for job in jobs(manager, run_id):
        if job.get("ue") in ues and job.get("status") in {"QUEUED", "RUNNING"}:
            request_json(f"{manager}/api/runs/{run_id}/traffic/{job['id']}", "DELETE")


def wait_for_active(manager: str, run_id: str, expected: set[str], timeout: float = 30) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if expected <= active_ues(jobs(manager, run_id)):
            return
        time.sleep(1)
    raise RuntimeError(f"timed out waiting for active traffic: {sorted(expected)}")


def wait_for_inactive(manager: str, run_id: str, expected: set[str], timeout: float = 30) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not (expected & active_ues(jobs(manager, run_id))):
            return
        time.sleep(0.5)
    raise RuntimeError(f"timed out waiting for stopped traffic: {sorted(expected)}")


def sample_window(
    manager: str,
    run_id: str,
    seconds: int,
    *,
    scenario_id: str,
    round_index: int,
    ue1_base_scale: float,
    ue2_base_scale: float,
    policy: str,
) -> list[dict[str, Any]]:
    samples: list[dict[str, Any]] = []
    for sample_index in range(seconds):
        features, ue_metrics = extract_features(
            jobs(manager, run_id),
            metric_values(manager, run_id, "ue_rx_bps"),
        )
        samples.append(
            {
                "timestamp": time.time(),
                "scenario_id": scenario_id,
                "round": round_index,
                "ue1_base_scale": ue1_base_scale,
                "ue2_base_scale": ue2_base_scale,
                "policy": policy,
                "policy_scale": POLICY_SCALES[policy],
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
        "voice_delivery_ratio_median": median(float(row["voice_delivery_ratio"]) for row in samples),
        "voice_loss_percent_median": median(float(row["voice_loss_percent"]) for row in samples),
        "voice_jitter_ms_median": median(float(row["voice_jitter_ms"]) for row in samples),
        "voice_rtt_p95_ms_median": median(float(row["voice_rtt_p95_ms"]) for row in samples),
        "video_offered_mbps_median": median(float(row["video_offered_mbps"]) for row in samples),
        "video_delivered_mbps_median": median(float(row["video_delivered_mbps"]) for row in samples),
    }


def acceptable(summary: dict[str, Any]) -> bool:
    return (
        summary["sla_success_ratio"] >= 0.75
        and summary["voice_delivery_ratio_median"] >= SLA["delivery_ratio_min"]
        and summary["voice_loss_percent_median"] <= SLA["loss_percent_max"]
        and summary["voice_jitter_ms_median"] <= SLA["jitter_ms_max"]
        and summary["voice_rtt_p95_ms_median"] <= SLA["rtt_p95_ms_max"]
    )


def write_csv(path: Path, rows: list[dict[str, Any]], columns: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=columns, extrasaction="ignore", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    temporary.replace(path)


def load_csv(path: Path) -> list[dict[str, Any]]:
    if not path.is_file():
        return []
    with path.open(newline="") as source:
        return list(csv.DictReader(source))


def scenario_grid(rounds: int, levels: tuple[float, ...]) -> list[tuple[int, float, float]]:
    pairs = [(ue1_scale, ue2_scale) for ue1_scale in levels for ue2_scale in levels]
    pairs.sort(key=lambda pair: (sum(pair), abs(pair[0] - pair[1])))
    # Alternate light and heavy states so a bad parameter range is discovered
    # early instead of after an entire low-load half-grid has completed.
    ordered: list[tuple[float, float]] = []
    while pairs:
        ordered.append(pairs.pop(0))
        if pairs:
            ordered.append(pairs.pop())
    return [
        (round_index, ue1_scale, ue2_scale)
        for round_index in range(1, rounds + 1)
        for ue1_scale, ue2_scale in ordered
    ]


def checkpoint(
    output_dir: Path,
    run_id: str,
    raw_rows: list[dict[str, Any]],
    training_rows: list[dict[str, Any]],
    results: list[dict[str, Any]],
    *,
    results_filename: str = "policy_results.json",
) -> None:
    raw_columns = [
        "timestamp", "scenario_id", "round", "ue1_base_scale", "ue2_base_scale",
        "policy", "policy_scale", "sample_index", *FEATURE_NAMES, "sla_ok", "ue_metrics",
    ]
    training_columns = [
        "scenario_id", "round", "ue1_base_scale", "ue2_base_scale", *FEATURE_NAMES, "label",
    ]
    write_csv(output_dir / "raw_samples.csv", raw_rows, raw_columns)
    write_csv(output_dir / "training.csv", training_rows, training_columns)
    atomic_write_json(
        output_dir / results_filename,
        {
            "run_id": run_id,
            "updated_at": time.time(),
            "method": "test every candidate and label the least restrictive voice-SLA-compliant policy",
            "sla": SLA,
            "policy_scales": POLICY_SCALES,
            "scenarios": results,
        },
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--manager-url", default="http://127.0.0.1:8088")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--control-file", type=Path, required=True)
    parser.add_argument("--rounds", type=int, default=2)
    parser.add_argument("--warmup-seconds", type=int, default=2)
    parser.add_argument("--sample-seconds", type=int, default=4)
    parser.add_argument("--seed", type=int, default=20260803)
    parser.add_argument("--campaign", default="rf3")
    parser.add_argument("--levels", default="0.25,0.40,0.55,0.70,0.85,1.00")
    parser.add_argument("--resume", action="store_true")
    arguments = parser.parse_args()
    manager = arguments.manager_url.rstrip("/")

    run = request_json(f"{manager}/api/runs/{arguments.run_id}")
    if run.get("state") != "RUNNING":
        raise SystemExit(f"run must be RUNNING, got {run.get('state')}")
    configuration = request_json(f"{manager}/api/runs/{arguments.run_id}/traffic/config")
    types = {
        item["ue"]: (item.get("traffic", {}).get("flows") or [{}])[0].get("type")
        for item in configuration.get("ues", [])
    }
    if any(types.get(ue) != "short_video" for ue in VIDEO_UES):
        raise SystemExit("UE1 and UE2 must be configured as short_video")
    if types.get("ue3") != "rtp_voice":
        raise SystemExit("UE3 must be configured as rtp_voice")

    arguments.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path = arguments.output_dir / "raw_samples.csv"
    training_path = arguments.output_dir / "training.csv"
    # Collection progress and relabelled training output intentionally use
    # different files. Relabelling aggregates round-specific scenario IDs;
    # using that output as a resume checkpoint would discard collected rows.
    results_path = arguments.output_dir / "collection_results.json"
    if arguments.resume and results_path.is_file():
        results = json.loads(results_path.read_text()).get("scenarios", [])
    else:
        results = []
    completed = {str(item["scenario_id"]) for item in results}
    raw_rows = [
        row for row in (load_csv(raw_path) if arguments.resume else [])
        if str(row.get("scenario_id")) in completed
    ]
    training_rows = [
        row for row in (load_csv(training_path) if arguments.resume else [])
        if str(row.get("scenario_id")) in completed
    ]
    levels = tuple(float(value.strip()) for value in arguments.levels.split(",") if value.strip())
    if not levels or any(value < 0.1 or value > 1.0 for value in levels):
        raise SystemExit("levels must contain comma-separated values between 0.1 and 1.0")
    grid = scenario_grid(arguments.rounds, levels)
    random_source = random.Random(arguments.seed)
    atomic_write_json(
        arguments.output_dir / "collection_manifest.json",
        {
            "run_id": arguments.run_id,
            "manager_url": manager,
            "campaign": arguments.campaign,
            "rounds": arguments.rounds,
            "levels": list(levels),
            "warmup_seconds": arguments.warmup_seconds,
            "sample_seconds": arguments.sample_seconds,
            "random_seed": arguments.seed,
            "expected_scenarios": len(grid),
            "expected_raw_samples": len(grid) * len(POLICY_ORDER) * arguments.sample_seconds,
            "traffic_configuration": configuration,
            "sla": SLA,
            "policy_scales": POLICY_SCALES,
        },
    )

    start_ues(manager, arguments.run_id, [*VIDEO_UES, *VOICE_UES])
    wait_for_active(manager, arguments.run_id, {*VIDEO_UES, *VOICE_UES})
    time.sleep(max(2, arguments.warmup_seconds))
    try:
        for index, (round_index, ue1_scale, ue2_scale) in enumerate(grid, start=1):
            scenario_id = f"{arguments.campaign}-r{round_index:02d}-u1-{int(ue1_scale * 100):03d}-u2-{int(ue2_scale * 100):03d}"
            if scenario_id in completed:
                print(f"[{index}/{len(grid)}] {scenario_id}: already complete", flush=True)
                continue
            base_scales = {"ue1": ue1_scale, "ue2": ue2_scale}
            policy_sequence = list(POLICY_ORDER)
            random_source.shuffle(policy_sequence)
            outcomes: dict[str, dict[str, Any]] = {}
            baseline_samples: list[dict[str, Any]] = []
            print(f"[{index}/{len(grid)}] {scenario_id}: start", flush=True)
            for policy in policy_sequence:
                # Restart both video generators for every candidate so wave
                # phase and pseudo-random burst sequence are identical. This
                # makes the four policy windows a controlled A/B comparison.
                stop_ues(manager, arguments.run_id, list(VIDEO_UES))
                wait_for_inactive(manager, arguments.run_id, set(VIDEO_UES))
                write_video_policy(
                    arguments.control_file,
                    base_scales,
                    policy,
                    f"rf3_collect_{scenario_id}_{policy.lower()}",
                )
                start_ues(manager, arguments.run_id, list(VIDEO_UES))
                wait_for_active(manager, arguments.run_id, {*VIDEO_UES, *VOICE_UES})
                time.sleep(arguments.warmup_seconds)
                samples = sample_window(
                    manager,
                    arguments.run_id,
                    arguments.sample_seconds,
                    scenario_id=scenario_id,
                    round_index=round_index,
                    ue1_base_scale=ue1_scale,
                    ue2_base_scale=ue2_scale,
                    policy=policy,
                )
                raw_rows.extend(samples)
                outcomes[policy] = summarize_policy(samples)
                if policy == "EQUAL_100":
                    baseline_samples = samples
                print(
                    f"  {policy}: voice SLA {outcomes[policy]['sla_success_ratio'] * 100:.0f}% "
                    f"RTT {outcomes[policy]['voice_rtt_p95_ms_median']:.1f} ms",
                    flush=True,
                )

            label = next((policy for policy in POLICY_ORDER if acceptable(outcomes[policy])), POLICY_ORDER[-1])
            for sample in baseline_samples:
                training_rows.append(
                    {
                        "scenario_id": scenario_id,
                        "round": round_index,
                        "ue1_base_scale": ue1_scale,
                        "ue2_base_scale": ue2_scale,
                        **{name: sample[name] for name in FEATURE_NAMES},
                        "label": label,
                    }
                )
            results.append(
                {
                    "scenario_id": scenario_id,
                    "round": round_index,
                    "base_scales": base_scales,
                    "policy_sequence": policy_sequence,
                    "label": label,
                    "outcomes": outcomes,
                    "baseline_features": median_features(baseline_samples),
                }
            )
            checkpoint(
                arguments.output_dir,
                arguments.run_id,
                raw_rows,
                training_rows,
                results,
                results_filename="collection_results.json",
            )
            print(f"[{index}/{len(grid)}] {scenario_id}: selected {label}", flush=True)
    finally:
        write_video_policy(
            arguments.control_file,
            {"ue1": 1.0, "ue2": 1.0},
            "EQUAL_100",
            "rf3_collection_finished_or_interrupted",
        )
        checkpoint(
            arguments.output_dir,
            arguments.run_id,
            raw_rows,
            training_rows,
            results,
            results_filename="collection_results.json",
        )

    print(
        json.dumps(
            {
                "raw_samples": len(raw_rows),
                "training_rows": len(training_rows),
                "scenarios": len(results),
                "output_dir": str(arguments.output_dir),
            },
            ensure_ascii=False,
        ),
        flush=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
