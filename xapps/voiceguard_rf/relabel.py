#!/usr/bin/env python3
"""Rebuild policy labels from already collected raw policy samples."""

from __future__ import annotations

import argparse
import csv
import json
import time
from collections import defaultdict
from pathlib import Path

from collect import acceptable, summarize_policy, write_csv
from common import FEATURE_NAMES, POLICY_ORDER, POLICY_SCALES, SLA, median_features


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-samples", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    arguments = parser.parse_args()

    with arguments.raw_samples.open(newline="") as source:
        raw_rows = list(csv.DictReader(source))
    grouped: dict[tuple[str, str], list[dict[str, object]]] = defaultdict(list)
    for row in raw_rows:
        sla_ok = (
            float(row["active_voice_count"]) > 0
            and float(row["voice_delivery_ratio"]) >= SLA["delivery_ratio_min"]
            and float(row["voice_loss_percent"]) <= SLA["loss_percent_max"]
            and float(row["voice_jitter_ms"]) <= SLA["jitter_ms_max"]
            and float(row["voice_rtt_p95_ms"]) <= SLA["rtt_p95_ms_max"]
        )
        row["sla_ok"] = int(sla_ok)
        grouped[(row["scenario_id"], row["policy"])].append(row)

    scenarios = sorted({scenario for scenario, _ in grouped})
    results = []
    training_rows = []
    for scenario_id in scenarios:
        outcomes = {
            policy: summarize_policy(grouped[(scenario_id, policy)])
            for policy in POLICY_ORDER
        }
        label = next(
            (policy for policy in POLICY_ORDER if acceptable(outcomes[policy])),
            POLICY_ORDER[-1],
        )
        baseline = grouped[(scenario_id, "EQUAL_100")]
        for sample in baseline:
            training_rows.append(
                {
                    "scenario_id": scenario_id,
                    **{name: sample[name] for name in FEATURE_NAMES},
                    "label": label,
                }
            )
        active_voice_ues = [
            ue
            for ue, feature in (("ue9", "ue9_active"), ("ue10", "ue10_active"))
            if float(baseline[0][feature]) > 0
        ]
        results.append(
            {
                "scenario_id": scenario_id,
                "active_voice_ues": active_voice_ues,
                "label": label,
                "outcomes": outcomes,
                "baseline_features": median_features(baseline),
            }
        )

    raw_columns = list(raw_rows[0])
    training_columns = ["scenario_id", *FEATURE_NAMES, "label"]
    write_csv(arguments.output_dir / "raw_samples.csv", raw_rows, raw_columns)
    write_csv(arguments.output_dir / "training.csv", training_rows, training_columns)
    (arguments.output_dir / "policy_results.json").write_text(
        json.dumps(
            {
                "run_id": arguments.run_id,
                "created_at": time.time(),
                "source_raw_samples": str(arguments.raw_samples),
                "method": "relabel all measured candidate policy windows with the shared SLA",
                "sla": SLA,
                "policy_scales": POLICY_SCALES,
                "scenarios": results,
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
                "scenarios": len(results),
                "output_dir": str(arguments.output_dir),
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
