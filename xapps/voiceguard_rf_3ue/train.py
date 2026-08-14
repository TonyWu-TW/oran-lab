#!/usr/bin/env python3
"""Train and scenario-group validate the 3 UE VoiceGuard Random Forest."""

from __future__ import annotations

import argparse
import csv
import json
import platform
import time
from collections import Counter
from pathlib import Path

import joblib
import sklearn
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, balanced_accuracy_score, classification_report, confusion_matrix
from sklearn.model_selection import GroupKFold, cross_val_predict

from common import FEATURE_NAMES, POLICY_ORDER, POLICY_SCALES, SLA


def policy_acceptable(summary: dict[str, float]) -> bool:
    return (
        summary["sla_success_ratio"] >= 0.75
        and summary["voice_delivery_ratio_median"] >= SLA["delivery_ratio_min"]
        and summary["voice_loss_percent_median"] <= SLA["loss_percent_max"]
        and summary["voice_jitter_ms_median"] <= SLA["jitter_ms_max"]
        and summary["voice_rtt_p95_ms_median"] <= SLA["rtt_p95_ms_max"]
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--policy-results", type=Path)
    parser.add_argument("--trees", type=int, default=500)
    parser.add_argument("--seed", type=int, default=20260803)
    arguments = parser.parse_args()

    with arguments.dataset.open(newline="") as source:
        rows = list(csv.DictReader(source))
    if len(rows) < 40:
        raise SystemExit(f"dataset is too small: {len(rows)} rows (need at least 40)")
    labels = [row["label"] for row in rows]
    if len(set(labels)) < 2:
        raise SystemExit(f"dataset only contains one policy class: {set(labels)}")
    features = [[float(row[name]) for name in FEATURE_NAMES] for row in rows]
    groups = [row["scenario_id"] for row in rows]
    model = RandomForestClassifier(
        n_estimators=arguments.trees,
        max_depth=5,
        min_samples_leaf=3,
        max_features="sqrt",
        class_weight="balanced_subsample",
        random_state=arguments.seed,
        n_jobs=-1,
    )

    unique_groups = len(set(groups))
    folds = min(5, unique_groups)
    splitter = GroupKFold(n_splits=folds)
    predictions = cross_val_predict(
        model,
        features,
        labels,
        groups=groups,
        cv=splitter,
        n_jobs=-1,
    )
    probabilities = cross_val_predict(
        model,
        features,
        labels,
        groups=groups,
        cv=GroupKFold(n_splits=folds),
        n_jobs=-1,
        method="predict_proba",
    )
    matrix_labels = [name for name in POLICY_ORDER if name in set(labels)]
    report = {
        "trained_at": time.time(),
        "python_version": platform.python_version(),
        "sklearn_version": sklearn.__version__,
        "dataset": str(arguments.dataset),
        "model": str(arguments.model),
        "rows": len(rows),
        "scenarios": unique_groups,
        "class_distribution": dict(Counter(labels)),
        "group_cross_validation_folds": folds,
        "group_cross_validation_accuracy": accuracy_score(labels, predictions),
        "group_cross_validation_balanced_accuracy": balanced_accuracy_score(labels, predictions),
        "confusion_matrix_labels": matrix_labels,
        "confusion_matrix": confusion_matrix(labels, predictions, labels=matrix_labels).tolist(),
        "classification_report": classification_report(labels, predictions, output_dict=True, zero_division=0),
    }

    # Runtime makes one policy decision per call, not one independent decision
    # per row. Average held-out probabilities across all baseline samples of a
    # load pair to report a leak-free scenario-level decision.
    probability_classes = sorted(set(labels))
    group_indices: dict[str, list[int]] = {}
    for index, group in enumerate(groups):
        group_indices.setdefault(group, []).append(index)
    scenario_predictions: dict[str, str] = {}
    scenario_probabilities: dict[str, dict[str, float]] = {}
    scenario_labels: dict[str, str] = {}
    for group, indices in group_indices.items():
        averages = [
            sum(float(probabilities[index][column]) for index in indices) / len(indices)
            for column in range(len(probability_classes))
        ]
        scenario_probabilities[group] = dict(zip(probability_classes, averages))
        scenario_predictions[group] = probability_classes[max(range(len(averages)), key=averages.__getitem__)]
        scenario_labels[group] = labels[indices[0]]
    scenario_truth = [scenario_labels[group] for group in sorted(group_indices)]
    scenario_predicted = [scenario_predictions[group] for group in sorted(group_indices)]
    policy_index = {policy: index for index, policy in enumerate(POLICY_ORDER)}
    policy_distances = [
        abs(policy_index[truth] - policy_index[predicted])
        for truth, predicted in zip(scenario_truth, scenario_predicted)
    ]
    report["scenario_level"] = {
        "accuracy": accuracy_score(scenario_truth, scenario_predicted),
        "balanced_accuracy": balanced_accuracy_score(scenario_truth, scenario_predicted),
        "within_one_policy_level_accuracy": sum(distance <= 1 for distance in policy_distances) / len(policy_distances),
        "mean_absolute_policy_level_error": sum(policy_distances) / len(policy_distances),
        "confusion_matrix_labels": matrix_labels,
        "confusion_matrix": confusion_matrix(
            scenario_truth, scenario_predicted, labels=matrix_labels
        ).tolist(),
    }

    policy_results_path = arguments.policy_results or arguments.dataset.parent / "policy_results.json"
    if policy_results_path.is_file():
        measured = json.loads(policy_results_path.read_text())
        by_group = {item["scenario_id"]: item for item in measured.get("scenarios", [])}
        evaluated = [group for group in sorted(group_indices) if group in by_group]
        selected_ok = [
            policy_acceptable(by_group[group]["outcomes"][scenario_predictions[group]])
            for group in evaluated
        ]
        equal_ok = [
            policy_acceptable(by_group[group]["outcomes"]["EQUAL_100"])
            for group in evaluated
        ]
        strong_ok = [
            policy_acceptable(by_group[group]["outcomes"]["STRONG_40"])
            for group in evaluated
        ]
        decision_quantile = 0.8
        risk_predictions: dict[str, str] = {}
        for group in evaluated:
            cumulative = 0.0
            for policy in POLICY_ORDER:
                cumulative += scenario_probabilities[group].get(policy, 0.0)
                if cumulative >= decision_quantile:
                    risk_predictions[group] = policy
                    break
            else:
                risk_predictions[group] = POLICY_ORDER[-1]
        risk_ok = [
            policy_acceptable(by_group[group]["outcomes"][risk_predictions[group]])
            for group in evaluated
        ]
        report["counterfactual_policy_evaluation"] = {
            "scenarios": len(evaluated),
            "rf_selected_sla_success_rate": sum(selected_ok) / len(selected_ok),
            "equal_100_sla_success_rate": sum(equal_ok) / len(equal_ok),
            "always_strong_40_sla_success_rate": sum(strong_ok) / len(strong_ok),
            "rf_mean_video_scale": sum(POLICY_SCALES[scenario_predictions[group]] for group in evaluated) / len(evaluated),
            "always_strong_mean_video_scale": POLICY_SCALES["STRONG_40"],
            "under_protection_rate": sum(
                policy_index[scenario_predictions[group]] < policy_index[scenario_labels[group]]
                for group in evaluated
            ) / len(evaluated),
            "over_protection_rate": sum(
                policy_index[scenario_predictions[group]] > policy_index[scenario_labels[group]]
                for group in evaluated
            ) / len(evaluated),
            "risk_quantile": decision_quantile,
            "risk_aware_sla_success_rate": sum(risk_ok) / len(risk_ok),
            "risk_aware_mean_video_scale": sum(
                POLICY_SCALES[risk_predictions[group]] for group in evaluated
            ) / len(evaluated),
            "risk_aware_policy_distribution": dict(Counter(risk_predictions.values())),
            "note": "Held-out RF choices are scored against policy windows measured on the same load pair; runtime safety escalation is not included.",
        }

    model.fit(features, labels)
    arguments.model.parent.mkdir(parents=True, exist_ok=True)
    joblib.dump(
        {
            "artifact_version": 1,
            "model": model,
            "feature_names": list(FEATURE_NAMES),
            "policy_order": list(POLICY_ORDER),
            "trained_at": time.time(),
            "dataset": str(arguments.dataset),
            "scenario": "3ue_2video_1voice",
            "sla": SLA,
            "policy_scales": POLICY_SCALES,
            "input_window_seconds": 3,
            "decision_quantile": 0.8,
        },
        arguments.model,
    )
    report["feature_importance"] = dict(
        sorted(zip(FEATURE_NAMES, model.feature_importances_), key=lambda item: item[1], reverse=True)
    )
    report["parameters"] = model.get_params()
    arguments.report.parent.mkdir(parents=True, exist_ok=True)
    arguments.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
