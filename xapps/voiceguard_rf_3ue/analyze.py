#!/usr/bin/env python3
"""Produce a compact quality report for a collected 3 UE RF dataset."""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path
from statistics import median
from typing import Any

from common import FEATURE_NAMES, POLICY_ORDER
from train import policy_acceptable


def distribution(values: list[float]) -> dict[str, float]:
    return {
        "minimum": min(values),
        "median": median(values),
        "maximum": max(values),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()

    with (arguments.dataset_dir / "raw_samples.csv").open(newline="") as source:
        rows: list[dict[str, Any]] = list(csv.DictReader(source))
    collection = json.loads((arguments.dataset_dir / "collection_results.json").read_text())
    labelled = json.loads((arguments.dataset_dir / "policy_results.json").read_text())
    original_scenarios = collection.get("scenarios", [])
    load_labels: dict[tuple[float, float], list[str]] = defaultdict(list)
    for scenario in original_scenarios:
        scales = scenario["base_scales"]
        load_labels[(float(scales["ue1"]), float(scales["ue2"]))].append(scenario["label"])
    repeat_agreements = [
        Counter(labels).most_common(1)[0][1] / len(labels)
        for labels in load_labels.values()
    ]
    non_monotonic = 0
    for scenario in labelled.get("scenarios", []):
        passed = [policy_acceptable(scenario["outcomes"][policy]) for policy in POLICY_ORDER]
        if any(passed[index] and not passed[index + 1] for index in range(len(passed) - 1)):
            non_monotonic += 1

    missing = {
        name: sum(row.get(name, "") == "" for row in rows)
        for name in FEATURE_NAMES
    }
    sample_sla = Counter(row["policy"] for row in rows if int(float(row["sla_ok"])))
    policy_samples = Counter(row["policy"] for row in rows)
    report = {
        "raw_samples": len(rows),
        "round_specific_scenarios": len(original_scenarios),
        "independent_load_pairs": len(load_labels),
        "rounds_per_load_pair": distribution([float(len(labels)) for labels in load_labels.values()]),
        "samples_per_policy": dict(policy_samples),
        "sample_sla_success_rate_by_policy": {
            policy: sample_sla[policy] / policy_samples[policy] for policy in POLICY_ORDER
        },
        "aggregated_label_distribution": dict(
            Counter(item["label"] for item in labelled.get("scenarios", []))
        ),
        "repeat_label_agreement": distribution(repeat_agreements),
        "non_monotonic_load_pairs": non_monotonic,
        "missing_feature_values": missing,
        "feature_distribution": {
            name: distribution([float(row[name]) for row in rows])
            for name in FEATURE_NAMES
        },
        "notes": [
            "Non-monotonic policy outcomes are retained as measured radio/scheduler noise.",
            "Training and validation group all rounds of one offered-load pair together.",
        ],
    }
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
