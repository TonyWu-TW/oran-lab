#!/usr/bin/env python3
"""VoiceGuard RF V2 near-RT xApp.

The Random Forest selects the least restrictive video pacing policy. A small
deterministic safety layer can only make the model's choice more protective
when the measured voice SLA remains violated for three consecutive samples.
"""

from __future__ import annotations

import argparse
import json
import os
import signal
import subprocess
import time
from pathlib import Path
from typing import Any
from urllib.request import Request, urlopen

import joblib

from common import (
    FEATURE_NAMES,
    POLICY_ORDER,
    POLICY_SCALES,
    VIDEO_UES,
    VOICE_UES,
    atomic_write_json,
    extract_features,
    feature_vector,
    voice_sla_ok,
    write_traffic_scale,
)


stop_requested = False
LAB_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MODEL = Path(__file__).resolve().parent / "models" / "voiceguard_rf.joblib"
DEFAULT_RC_BRIDGE = (
    LAB_ROOT
    / "src"
    / "flexric"
    / "build"
    / "examples"
    / "xApp"
    / "c"
    / "voiceguard_rc"
    / "voiceguard_rc"
)


def request_stop(_: int, __: object) -> None:
    global stop_requested
    stop_requested = True


def get_json(url: str) -> Any:
    request = Request(url, headers={"Accept": "application/json"})
    with urlopen(request, timeout=4) as response:
        return json.loads(response.read())


def metric_values(manager_url: str, run_id: str, metric: str) -> dict[str, float]:
    payload = get_json(
        f"{manager_url}/api/runs/{run_id}/metrics/query?metric={metric}"
    )
    return {
        str(item.get("metric", {}).get("ue")): float(item["value"][1])
        for item in payload.get("data", {}).get("result", [])
        if item.get("metric", {}).get("ue") and item.get("value")
    }


def append_event(state: dict[str, Any], event_type: str, message: str) -> None:
    state.setdefault("events", []).append(
        {"timestamp": time.time(), "type": event_type, "message": message}
    )
    state["events"] = state["events"][-60:]


def policy_string(policies: list[dict[str, int]]) -> str:
    return ",".join(
        f"{item['ue_id']}:{item['minimum']}:{item['maximum']}:{item['dedicated']}"
        for item in policies
    )


def apply_rc_baseline(
    bridge: Path,
    *,
    timeout_seconds: float,
    sst: int,
    sd: int,
) -> dict[str, Any]:
    policies = [
        {"ue_id": index, "minimum": 0, "maximum": 100, "dedicated": 0}
        for index in range(10)
    ]
    environment = {
        **os.environ,
        "VOICEGUARD_POLICIES": policy_string(policies),
        "VOICEGUARD_SST": str(sst),
        "VOICEGUARD_SD": str(sd),
    }
    started_at = time.time()
    try:
        result = subprocess.run(
            [str(bridge)],
            stdin=subprocess.DEVNULL,
            capture_output=True,
            text=True,
            env=environment,
            timeout=timeout_seconds,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        return {
            "success": False,
            "timestamp": started_at,
            "duration_ms": round((time.time() - started_at) * 1000, 1),
            "error": str(error),
            "policies": policies,
        }
    result_lines = [
        line
        for line in result.stdout.splitlines()
        if line.startswith("VOICEGUARD_RC_RESULT")
    ]
    success = (
        result.returncode == 0
        and len(result_lines) == len(policies)
        and all("success=true" in line for line in result_lines)
    )
    errors = [
        line
        for line in result.stderr.splitlines()
        if line.startswith("VOICEGUARD_RC_ERROR")
    ]
    return {
        "success": success,
        "timestamp": started_at,
        "duration_ms": round((time.time() - started_at) * 1000, 1),
        "returncode": result.returncode,
        "results": result_lines,
        "error": "; ".join(errors)
        or (None if success else "RC bridge did not acknowledge all 10 baseline policies"),
        "policies": policies,
    }


def parse_sd(value: Any) -> int:
    return int(str(value).removeprefix("0x"), 16)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--manager-url", default="http://127.0.0.1:8088")
    parser.add_argument("--state-file", type=Path, required=True)
    parser.add_argument(
        "--mode", choices=["observe_only", "closed_loop"], default="observe_only"
    )
    parser.add_argument("--config-json", default="{}")
    arguments = parser.parse_args()
    config = json.loads(arguments.config_json)
    model_path = Path(config.get("model_path") or DEFAULT_MODEL)
    traffic_control_file = Path(config["traffic_control_file"])
    interval = float(config.get("sample_interval_seconds", 1.0))
    required_samples = int(config.get("consecutive_samples", 3))
    restore_step_seconds = float(config.get("restore_step_seconds", 3.0))

    artifact = joblib.load(model_path)
    model = artifact["model"]
    artifact_features = tuple(artifact.get("feature_names") or ())
    if artifact_features != FEATURE_NAMES:
        raise SystemExit(
            f"model feature mismatch: expected {FEATURE_NAMES}, got {artifact_features}"
        )
    importance = dict(
        sorted(
            zip(FEATURE_NAMES, model.feature_importances_),
            key=lambda item: item[1],
            reverse=True,
        )
    )
    bridge = Path(config.get("rc_bridge_path") or DEFAULT_RC_BRIDGE)

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    state: dict[str, Any] = {
        "run_id": arguments.run_id,
        "pid": os.getpid(),
        "running": True,
        "state": "OBSERVING",
        "mode": arguments.mode,
        "algorithm": "random_forest",
        "model_name": "VoiceGuard RF V2",
        "model_path": str(model_path),
        "model_trained_at": artifact.get("trained_at"),
        "feature_importance": importance,
        "e2_adapter": "manager_metrics+native_flexric_rc+traffic_pacing",
        "e2_connected": False,
        "native_control": False,
        "current_policy": "EQUAL_100 · video Offered 100%",
        "predicted_policy": None,
        "prediction_confidence": None,
        "policy_probabilities": {},
        "last_decision": "等待 UE9 / UE10 語音通話",
        "last_sample_at": None,
        "actuator": "random_forest_selected_traffic_pacing+e2sm_rc_safety_baseline",
        "traffic_shaping_factor": 1.0,
        "active_voice_ues": [],
        "ues": {},
        "events": [],
        "config": config,
    }
    append_event(state, "started", f"VoiceGuard RF V2 {arguments.mode} 已啟動")
    atomic_write_json(arguments.state_file, state)

    current_policy = "EQUAL_100"
    predicted_policy = "EQUAL_100"
    previous_voice_signature: tuple[str, ...] = ()
    inference_wait = 0
    decision_initialized = False
    bad_samples = stable_samples = 0
    next_restore_at = 0.0
    try:
        write_traffic_scale(
            traffic_control_file, 1.0, "voiceguard_rf_start_baseline"
        )
        if arguments.mode == "closed_loop":
            if bridge.is_file() and os.access(bridge, os.X_OK):
                rc = apply_rc_baseline(
                    bridge,
                    timeout_seconds=float(config.get("rc_timeout_seconds", 30.0)),
                    sst=int(config.get("sst", 1)),
                    sd=parse_sd(config.get("sd", "ffffff")),
                )
                state["last_rc"] = rc
                state["e2_connected"] = bool(rc["success"])
                state["native_control"] = bool(rc["success"])
                append_event(
                    state,
                    "rc_ready" if rc["success"] else "rc_warning",
                    (
                        f"10 UE E2SM-RC baseline ACK（{rc['duration_ms']:.0f} ms）"
                        if rc["success"]
                        else f"RC baseline 未完整 ACK；RF pacing 仍可運作：{rc['error']}"
                    ),
                )
            else:
                append_event(
                    state,
                    "rc_warning",
                    "找不到 RC bridge；RF pacing 仍會運作",
                )
            atomic_write_json(arguments.state_file, state)

        while not stop_requested:
            sampled_at = time.time()
            try:
                jobs = get_json(
                    f"{arguments.manager_url}/api/runs/{arguments.run_id}/traffic"
                )
                features, ue_metrics = extract_features(
                    jobs,
                    metric_values(
                        arguments.manager_url,
                        arguments.run_id,
                        "ue_rx_bps",
                    ),
                )
                voice_signature = tuple(
                    ue
                    for ue in VOICE_UES
                    if ue_metrics.get(ue, {}).get("offered_bps", 0) > 0
                )
                voice_active = bool(voice_signature)
                healthy = voice_sla_ok(features)
                bad_samples = bad_samples + 1 if voice_active and not healthy else 0
                stable_samples = stable_samples + 1 if voice_active and healthy else 0

                if voice_signature != previous_voice_signature:
                    if voice_active:
                        inference_wait = required_samples
                        decision_initialized = False
                        bad_samples = stable_samples = 0
                        append_event(
                            state,
                            "voice_changed",
                            f"偵測到通話組合：{', '.join(ue.upper() for ue in voice_signature)}",
                        )
                    else:
                        decision_initialized = False
                        next_restore_at = sampled_at
                        append_event(state, "voice_stopped", "所有語音通話已結束")
                    previous_voice_signature = voice_signature

                if voice_active:
                    if inference_wait > 0:
                        inference_wait -= 1
                        state["state"] = "OBSERVING"
                        state["last_decision"] = (
                            f"收集最近 3 秒通話狀態（剩 {inference_wait} 筆）"
                        )
                    elif not decision_initialized:
                        started = time.perf_counter()
                        prediction = str(model.predict([feature_vector(features)])[0])
                        probabilities = {
                            str(label): float(probability)
                            for label, probability in zip(
                                model.classes_,
                                model.predict_proba([feature_vector(features)])[0],
                            )
                        }
                        inference_ms = (time.perf_counter() - started) * 1000
                        if prediction not in POLICY_SCALES:
                            prediction = "STRONG_40"
                        predicted_policy = prediction
                        selected = prediction
                        decision_initialized = True
                        state["inference_ms"] = round(inference_ms, 3)
                        state["predicted_policy"] = prediction
                        state["prediction_confidence"] = probabilities.get(prediction)
                        state["policy_probabilities"] = probabilities
                        if arguments.mode == "closed_loop" and selected != current_policy:
                            current_policy = selected
                            scale = POLICY_SCALES[current_policy]
                            write_traffic_scale(
                                traffic_control_file,
                                scale,
                                f"voiceguard_rf_{current_policy.lower()}",
                            )
                            append_event(
                                state,
                                "policy_applied",
                                f"Random Forest 套用 {current_policy}（影片 {scale * 100:.0f}%）",
                            )
                    else:
                        selected = current_policy
                        # While a call is active, the safety layer only changes one
                        # level after sustained evidence. This prevents 1 Hz policy
                        # oscillation when RF class probabilities are close.
                        if bad_samples >= required_samples:
                            selected_index = POLICY_ORDER.index(current_policy)
                            selected = POLICY_ORDER[
                                min(len(POLICY_ORDER) - 1, selected_index + 1)
                            ]
                            bad_samples = 0
                            stable_samples = 0
                            append_event(
                                state,
                                "safety_escalation",
                                f"連續 3 秒未達 SLA，安全層升級至 {selected}",
                            )
                        if arguments.mode == "closed_loop" and selected != current_policy:
                            current_policy = selected
                            scale = POLICY_SCALES[current_policy]
                            write_traffic_scale(
                                traffic_control_file,
                                scale,
                                f"voiceguard_rf_{current_policy.lower()}",
                            )
                            append_event(
                                state,
                                "policy_applied",
                                f"Random Forest 套用 {current_policy}（影片 {scale * 100:.0f}%）",
                            )
                    state["state"] = (
                        "PROTECTING"
                        if arguments.mode == "closed_loop"
                        and POLICY_SCALES[current_policy] < 1.0
                        else (
                            "WOULD_PROTECT"
                            if arguments.mode == "observe_only"
                            and POLICY_SCALES[predicted_policy] < 1.0
                            else "OBSERVING"
                        )
                    )
                    effective = (
                        current_policy
                        if arguments.mode == "closed_loop"
                        else predicted_policy
                    )
                    confidence = float(state.get("prediction_confidence") or 0.0)
                    state["last_decision"] = (
                        f"RF 建議 {predicted_policy}（信心 {confidence * 100:.1f}%）；"
                        f"安全層目前維持 {effective}"
                    )
                else:
                    if (
                        arguments.mode == "closed_loop"
                        and current_policy != "EQUAL_100"
                        and sampled_at >= next_restore_at
                    ):
                        index = POLICY_ORDER.index(current_policy)
                        current_policy = POLICY_ORDER[max(0, index - 1)]
                        scale = POLICY_SCALES[current_policy]
                        write_traffic_scale(
                            traffic_control_file,
                            scale,
                            "voiceguard_rf_gradual_restore",
                        )
                        next_restore_at = sampled_at + restore_step_seconds
                        append_event(
                            state,
                            "policy_restored",
                            f"通話結束，逐級恢復至 {current_policy}",
                        )
                    state["state"] = (
                        "COOLDOWN" if current_policy != "EQUAL_100" else "OBSERVING"
                    )
                    state["last_decision"] = (
                        f"通話已結束，逐級恢復中：{current_policy}"
                        if current_policy != "EQUAL_100"
                        else "等待 UE9 / UE10 語音通話"
                    )

                scale = POLICY_SCALES[current_policy]
                state["current_policy"] = (
                    f"{current_policy} · video Offered {scale * 100:.0f}%"
                )
                state["traffic_shaping_factor"] = scale
                state["voice_active"] = voice_active
                state["active_voice_ues"] = list(voice_signature)
                state["current_features"] = features
                state["ues"] = ue_metrics
                state["total_video_offered_bps"] = (
                    features["video_offered_mbps"] * 1_000_000
                )
                state["total_video_delivered_bps"] = (
                    features["video_delivered_mbps"] * 1_000_000
                )
                state["consecutive_bad_samples"] = bad_samples
                state["last_sample_at"] = sampled_at
                state["last_error"] = None
                atomic_write_json(arguments.state_file, state)
            except Exception as error:
                state["state"] = "ERROR"
                state["last_error"] = str(error)
                state["last_decision"] = "取樣或 RF inference 失敗；維持上一個安全策略"
                append_event(state, "sample_error", str(error))
                atomic_write_json(arguments.state_file, state)
            time.sleep(max(0.0, interval - (time.time() - sampled_at)))
    finally:
        write_traffic_scale(
            traffic_control_file, 1.0, "voiceguard_rf_stopped"
        )
        state["running"] = False
        state["state"] = "OFF"
        state["traffic_shaping_factor"] = 1.0
        state["current_policy"] = "EQUAL_100 · video Offered 100%"
        state["last_decision"] = "VoiceGuard RF V2 已停止並恢復影片基線"
        append_event(state, "stopped", "RF xApp 已安全停止")
        atomic_write_json(arguments.state_file, state)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
