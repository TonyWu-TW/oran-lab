#!/usr/bin/env python3
"""Rebuild 3 UE training labels from previously measured policy windows."""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

from collect import acceptable, checkpoint, summarize_policy
from common import FEATURE_NAMES, POLICY_ORDER, SLA, median_features


def load_pair_id(row: dict[str, Any]) -> str:
    """Return a stable group id shared by repeated measurements of one load pair."""
    ue1 = round(float(row["ue1_base_scale"]) * 100)
    ue2 = round(float(row["ue2_base_scale"]) * 100)
    return f"load-u1-{ue1:03d}-u2-{ue2:03d}"


def normalized_model_sample(row: dict[str, Any]) -> dict[str, Any]:
    """Normalize legacy/raw HTTP burst measurements for model consumption."""
    normalized = dict(row)
    offered = float(row["video_offered_mbps"])
    delivered = min(offered, float(row["video_delivered_mbps"]))
    normalized["video_delivered_mbps"] = delivered
    normalized["video_delivery_ratio"] = min(1.0, delivered / offered) if offered else 0.0
    normalized["video_gap_mbps"] = max(0.0, offered - delivered)
    return normalized


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-samples", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--run-id", default="multiple-campaigns")
    arguments = parser.parse_args()

    with arguments.raw_samples.open(newline="") as source:
        raw_rows: list[dict[str, Any]] = list(csv.DictReader(source))
    # The collector repeats every offered-load pair in multiple rounds.  Treat
    # those repetitions as one experimental condition: policy labels are then
    # based on all repeated windows, and GroupKFold cannot leak another round
    # of the same load pair into the validation fold.
    grouped: dict[str, dict[str, list[dict[str, Any]]]] = defaultdict(lambda: defaultdict(list))
    for row in raw_rows:
        grouped[load_pair_id(row)][str(row["policy"])].append(row)

    training_rows: list[dict[str, Any]] = []
    results: list[dict[str, Any]] = []
    for scenario_id, policies in sorted(grouped.items()):
        if any(not policies.get(policy) for policy in POLICY_ORDER):
            continue
        outcomes = {policy: summarize_policy(policies[policy]) for policy in POLICY_ORDER}
        label = next((policy for policy in POLICY_ORDER if acceptable(outcomes[policy])), POLICY_ORDER[-1])
        baseline = policies["EQUAL_100"]
        first = baseline[0]
        source_scenarios = sorted({str(row["scenario_id"]) for row in baseline})
        rounds = sorted({int(float(row["round"])) for row in baseline})
        # Match runtime inference: one feature vector is the median of a
        # three-second window. Keep windows within a collector round so radio
        # discontinuities are never averaged across separate traffic starts.
        by_source: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for sample in baseline:
            by_source[str(sample["scenario_id"])].append(sample)
        for source_samples in by_source.values():
            source_samples.sort(key=lambda row: int(float(row["sample_index"])))
            windows = [source_samples[index:index + 3] for index in range(max(1, len(source_samples) - 2))]
            for window in windows:
                if len(window) < 3:
                    continue
                sample = window[-1]
                normalized_window = [normalized_model_sample(row) for row in window]
                training_rows.append(
                    {
                        "scenario_id": scenario_id,
                        "round": sample.get("round", ""),
                        "ue1_base_scale": sample.get("ue1_base_scale", ""),
                        "ue2_base_scale": sample.get("ue2_base_scale", ""),
                        **median_features(normalized_window),
                        "label": label,
                    }
                )
        results.append(
            {
                "scenario_id": scenario_id,
                "rounds": rounds,
                "source_scenarios": source_scenarios,
                "base_scales": {
                    "ue1": float(first.get("ue1_base_scale", 1)),
                    "ue2": float(first.get("ue2_base_scale", 1)),
                },
                "label": label,
                "outcomes": outcomes,
                "baseline_features": median_features(baseline),
                "relabelled_sla": SLA,
                "aggregation": "all repeated rounds for this offered-load pair",
            }
        )

    checkpoint(arguments.output_dir, arguments.run_id, raw_rows, training_rows, results)
    print(
        json.dumps(
            {
                "raw_samples": len(raw_rows),
                "complete_scenarios": len(results),
                "training_rows": len(training_rows),
                "sla": SLA,
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
