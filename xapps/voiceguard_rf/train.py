#!/usr/bin/env python3
"""Train and evaluate the VoiceGuard RF policy selector."""

from __future__ import annotations

import argparse
import csv
import json
import time
from collections import Counter
from pathlib import Path

import joblib
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.model_selection import GroupKFold, cross_val_predict

from common import FEATURE_NAMES, POLICY_ORDER


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--trees", type=int, default=300)
    parser.add_argument("--seed", type=int, default=20260729)
    arguments = parser.parse_args()

    with arguments.dataset.open(newline="") as source:
        rows = list(csv.DictReader(source))
    if len(rows) < 12:
        raise SystemExit(f"dataset is too small: {len(rows)} rows (need at least 12)")
    labels = [row["label"] for row in rows]
    if len(set(labels)) < 2:
        raise SystemExit(f"dataset only contains one policy class: {set(labels)}")
    x = [[float(row[name]) for name in FEATURE_NAMES] for row in rows]
    groups = [row["scenario_id"] for row in rows]
    model = RandomForestClassifier(
        n_estimators=arguments.trees,
        max_depth=2,
        min_samples_leaf=2,
        max_features=None,
        class_weight="balanced_subsample",
        random_state=arguments.seed,
        n_jobs=-1,
    )

    unique_groups = len(set(groups))
    folds = min(5, unique_groups)
    if folds >= 2:
        predictions = cross_val_predict(
            model,
            x,
            labels,
            groups=groups,
            cv=GroupKFold(n_splits=folds),
            n_jobs=-1,
        )
        validation_accuracy = accuracy_score(labels, predictions)
        matrix_labels = [name for name in POLICY_ORDER if name in set(labels)]
        matrix = confusion_matrix(labels, predictions, labels=matrix_labels).tolist()
        details = classification_report(labels, predictions, output_dict=True, zero_division=0)
    else:
        validation_accuracy = None
        matrix_labels = []
        matrix = []
        details = {}

    model.fit(x, labels)
    arguments.model.parent.mkdir(parents=True, exist_ok=True)
    joblib.dump(
        {
            "model": model,
            "feature_names": list(FEATURE_NAMES),
            "policy_order": list(POLICY_ORDER),
            "trained_at": time.time(),
            "dataset": str(arguments.dataset),
        },
        arguments.model,
    )
    report = {
        "trained_at": time.time(),
        "dataset": str(arguments.dataset),
        "model": str(arguments.model),
        "rows": len(rows),
        "scenarios": unique_groups,
        "class_distribution": dict(Counter(labels)),
        "group_cross_validation_folds": folds,
        "group_cross_validation_accuracy": validation_accuracy,
        "confusion_matrix_labels": matrix_labels,
        "confusion_matrix": matrix,
        "classification_report": details,
        "feature_importance": dict(
            sorted(
                zip(FEATURE_NAMES, model.feature_importances_),
                key=lambda item: item[1],
                reverse=True,
            )
        ),
        "parameters": model.get_params(),
    }
    arguments.report.parent.mkdir(parents=True, exist_ok=True)
    arguments.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
