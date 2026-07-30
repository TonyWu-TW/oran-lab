#!/usr/bin/env python3
"""VoiceGuard near-RT xApp policy process.

The policy and state machine live in Python. Metrics come from the Experiment
Manager's Prometheus/RTP facade. Standard E2SM-RC control is sent through a
small native FlexRIC bridge because this FlexRIC build has no RC Python SDK.
"""

from __future__ import annotations

import argparse
import json
import os
import signal
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


stop_requested = False
LAB_ROOT = Path(__file__).resolve().parents[2]
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
    with urlopen(request, timeout=3) as response:
        return json.loads(response.read())


def metric_values(manager_url: str, run_id: str, metric: str) -> dict[str, float]:
    payload = get_json(f"{manager_url}/api/runs/{run_id}/metrics/query?metric={metric}")
    result: dict[str, float] = {}
    for sample in payload.get("data", {}).get("result", []):
        ue = sample.get("metric", {}).get("ue")
        value = sample.get("value", [None, None])[1]
        if ue and value is not None:
            result[str(ue)] = float(value)
    return result


def write_state(path: Path, state: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", dir=path.parent
    )
    try:
        with os.fdopen(descriptor, "w") as output:
            json.dump(state, output, ensure_ascii=False, indent=2)
            output.write("\n")
        os.replace(temporary_name, path)
    finally:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass


def append_event(state: dict[str, Any], event_type: str, message: str) -> None:
    state.setdefault("events", []).append(
        {"timestamp": time.time(), "type": event_type, "message": message}
    )
    state["events"] = state["events"][-50:]


def active_job(
    jobs: list[dict[str, Any]], ue: str, traffic_type: str | None = None
) -> dict[str, Any] | None:
    for job in jobs:
        if job.get("ue") != ue or job.get("status") not in {
            "QUEUED",
            "RUNNING",
            "STOP_REQUESTED",
        }:
            continue
        if traffic_type is None or job.get("traffic_type") == traffic_type:
            return job
    return None


def offered_bps(job: dict[str, Any] | None) -> float:
    if not job:
        return 0.0
    progress = job.get("result", {}).get("progress", {})
    if progress.get("offered_bps") is not None:
        return float(progress["offered_bps"])
    parameters = job.get("parameters", {})
    return float(parameters.get("offered_load_mbps", 0.0)) * 1_000_000


def policy_string(policies: list[dict[str, int]]) -> str:
    return ",".join(
        f"{p['ue_id']}:{p['minimum']}:{p['maximum']}:{p['dedicated']}"
        for p in policies
    )


def apply_rc_policy(
    bridge: Path,
    policies: list[dict[str, int]],
    *,
    sst: int,
    sd: int,
    timeout_seconds: float,
) -> dict[str, Any]:
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
    error_lines = [
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
        "error": "; ".join(error_lines)
        or (None if success else "RC bridge did not confirm every policy"),
        "policies": policies,
    }


def parse_sd(value: Any) -> int:
    if isinstance(value, str):
        return int(value.removeprefix("0x"), 16)
    return int(value)


def set_traffic_scale(path: Path | None, factor: float, reason: str) -> None:
    if path is None:
        return
    write_state(
        path,
        {
            "updated_at": time.time(),
            "reason": reason,
            "ues": {"ue1": factor, "ue2": factor, "ue3": 1.0},
        },
    )


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

    sample_interval = float(config.get("sample_interval_seconds", 1.0))
    required_samples = int(config.get("consecutive_samples", 3))
    congestion_threshold = (
        float(config.get("congestion_threshold_mbps", 1.2)) * 1_000_000
    )
    loss_threshold = float(config.get("voice_loss_threshold_percent", 2.0))
    latency_threshold = float(config.get("voice_latency_threshold_ms", 80.0))
    jitter_threshold = float(config.get("voice_jitter_threshold_ms", 30.0))
    cooldown_seconds = int(config.get("cooldown_seconds", 5))
    bridge = Path(config.get("rc_bridge_path") or DEFAULT_RC_BRIDGE)
    rc_timeout = float(config.get("rc_timeout_seconds", 10.0))
    video_scale = int(config.get("video_offered_scale_percent", 60)) / 100
    video_maximum = int(config.get("video_max_prb_percent", 100))
    voice_minimum = int(config.get("voice_min_prb_percent", 0))
    voice_dedicated = int(config.get("voice_dedicated_prb_percent", 0))
    traffic_control_file = (
        Path(config["traffic_control_file"])
        if config.get("traffic_control_file")
        else None
    )
    ue_ids = {
        "ue1": int(config.get("ue1_f1ap_id", 0)),
        "ue2": int(config.get("ue2_f1ap_id", 1)),
        "ue3": int(config.get("ue3_f1ap_id", 2)),
    }
    sst = int(config.get("sst", 1))
    sd = parse_sd(config.get("sd", "ffffff"))
    baseline_policies = [
        {"ue_id": ue_ids[ue], "minimum": 0, "maximum": 100, "dedicated": 0}
        for ue in ("ue1", "ue2", "ue3")
    ]
    protected_policies = [
        {
            "ue_id": ue_ids["ue1"],
            "minimum": 0,
            "maximum": video_maximum,
            "dedicated": 0,
        },
        {
            "ue_id": ue_ids["ue2"],
            "minimum": 0,
            "maximum": video_maximum,
            "dedicated": 0,
        },
        {
            "ue_id": ue_ids["ue3"],
            "minimum": voice_minimum,
            "maximum": 100,
            "dedicated": voice_dedicated,
        },
    ]

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    state: dict[str, Any] = {
        "run_id": arguments.run_id,
        "pid": os.getpid(),
        "running": True,
        "state": "OBSERVING",
        "mode": arguments.mode,
        "e2_adapter": "prometheus_metrics+native_flexric_rc",
        "e2_connected": False,
        "native_control": False,
        "current_policy": "BASELINE · no RC command sent",
        "last_decision": "等待 UE3 語音通話",
        "last_sample_at": None,
        "config": config,
        "ue_mapping": ue_ids,
        "actuator": "traffic_pacing+e2sm_rc_safety_baseline",
        "traffic_shaping_factor": 1.0,
        "ues": {},
        "events": [],
    }
    append_event(
        state,
        "started",
        f"VoiceGuard {arguments.mode.replace('_', ' ').title()} 已啟動",
    )
    write_state(arguments.state_file, state)

    bad_samples = 0
    voice_was_active = False
    cooldown_until = 0.0
    previous_state = state["state"]
    protected = False
    rc_ready = False
    try:
        set_traffic_scale(traffic_control_file, 1.0, "voiceguard_start_baseline")
        if arguments.mode == "closed_loop":
            if not bridge.is_file() or not os.access(bridge, os.X_OK):
                state["state"] = "ERROR"
                state["last_error"] = f"RC bridge is not executable: {bridge}"
                state["last_decision"] = "Closed Loop 未啟動，未修改任何 PRB policy"
                append_event(state, "rc_unavailable", state["last_error"])
                write_state(arguments.state_file, state)
                return 2
            baseline_result = apply_rc_policy(
                bridge,
                baseline_policies,
                sst=sst,
                sd=sd,
                timeout_seconds=rc_timeout,
            )
            state["last_rc"] = baseline_result
            if not baseline_result["success"]:
                state["state"] = "ERROR"
                state["last_error"] = baseline_result["error"]
                state["last_decision"] = "基線 RC 驗證失敗，Closed Loop 已拒絕啟動"
                append_event(state, "rc_failed", state["last_decision"])
                write_state(arguments.state_file, state)
                return 3
            rc_ready = True
            state["e2_connected"] = True
            state["native_control"] = True
            state["current_policy"] = "BASELINE · UE1/UE2/UE3 max 100%"
            append_event(
                state,
                "rc_ready",
                f"E2SM-RC 已連線；基線策略 ACK（{baseline_result['duration_ms']:.0f} ms）",
            )
            write_state(arguments.state_file, state)

        while not stop_requested:
            sampled_at = time.time()
            try:
                jobs = get_json(
                    f"{arguments.manager_url}/api/runs/{arguments.run_id}/traffic"
                )
                rx = metric_values(arguments.manager_url, arguments.run_id, "ue_rx_bps")
                tx = metric_values(arguments.manager_url, arguments.run_id, "ue_tx_bps")
                latency = metric_values(
                    arguments.manager_url, arguments.run_id, "ue_ping_latency"
                )
                loss = metric_values(
                    arguments.manager_url, arguments.run_id, "ue_ping_loss"
                )
                video_jobs = {
                    ue: active_job(jobs, ue, "short_video")
                    for ue in ("ue1", "ue2")
                }
                voice_job = active_job(jobs, "ue3", "rtp_voice")
                voice_active = voice_job is not None
                voice_progress = (
                    voice_job.get("result", {}).get("progress", {})
                    if voice_job
                    else {}
                )
                total_offered = sum(offered_bps(job) for job in video_jobs.values())
                total_delivered = sum(
                    rx.get(ue, 0.0) + tx.get(ue, 0.0) for ue in ("ue1", "ue2")
                )
                voice_latency = float(
                    voice_progress.get(
                        "rtt_p95_rolling_ms",
                        voice_progress.get("rtt_p95_ms", latency.get("ue3", -1.0)),
                    )
                    or -1.0
                )
                voice_loss = float(
                    voice_progress.get("loss_percent", loss.get("ue3", -1.0)) or 0.0
                )
                voice_jitter = float(
                    voice_progress.get(
                        "jitter_rolling_ms", voice_progress.get("jitter_ms", 0.0)
                    )
                    or 0.0
                )
                congestion = total_offered >= congestion_threshold
                degraded = (
                    (voice_loss >= 0 and voice_loss > loss_threshold)
                    or (voice_latency >= 0 and voice_latency > latency_threshold)
                    or voice_jitter > jitter_threshold
                )
                bad_samples = (
                    bad_samples + 1
                    if voice_active and (congestion or degraded)
                    else 0
                )

                if voice_active and not voice_was_active:
                    append_event(state, "voice_started", "偵測到 UE3 RTP-like 語音通話")
                if not voice_active and voice_was_active:
                    append_event(state, "voice_stopped", "UE3 語音通話已停止")
                    cooldown_until = sampled_at + cooldown_seconds

                if bad_samples >= required_samples and not protected:
                    if arguments.mode == "closed_loop" and rc_ready:
                        state["state"] = "PROTECTING"
                        state["last_decision"] = "正在降低 UE1/UE2 Offered Load，為 UE3 騰出容量"
                        write_state(arguments.state_file, state)
                        set_traffic_scale(
                            traffic_control_file,
                            video_scale,
                            "voiceguard_protect_voice",
                        )
                        rc_result = (
                            apply_rc_policy(
                                bridge,
                                protected_policies,
                                sst=sst,
                                sd=sd,
                                timeout_seconds=rc_timeout,
                            )
                            if protected_policies != baseline_policies
                            else {
                                "success": True,
                                "timestamp": time.time(),
                                "duration_ms": 0.0,
                                "error": None,
                                "policies": baseline_policies,
                                "skipped": "RC grant limits kept at safe baseline",
                            }
                        )
                        state["last_rc"] = rc_result
                        if rc_result["success"]:
                            protected = True
                            state["traffic_shaping_factor"] = video_scale
                            state["current_policy"] = (
                                f"PROTECTED · UE1/UE2 Offered {video_scale * 100:.0f}% · "
                                "RC grant limits baseline-safe"
                            )
                            state["last_decision"] = (
                                f"UE1/UE2 需求量降至原設定的 {video_scale * 100:.0f}%；"
                                "UE3 維持完整送話能力"
                            )
                            append_event(
                                state,
                                "policy_applied",
                                f"影片 Offered Load 已調整為 {video_scale * 100:.0f}%",
                            )
                        else:
                            set_traffic_scale(
                                traffic_control_file, 1.0, "voiceguard_rc_failure"
                            )
                            rollback = apply_rc_policy(
                                bridge,
                                baseline_policies,
                                sst=sst,
                                sd=sd,
                                timeout_seconds=rc_timeout,
                            )
                            state["rollback_rc"] = rollback
                            state["state"] = "ERROR"
                            state["last_error"] = rc_result["error"]
                            state["last_decision"] = "保護策略失敗；已嘗試恢復完整基線"
                            append_event(state, "rc_failed", state["last_decision"])
                            rc_ready = False
                    else:
                        state["state"] = "WOULD_PROTECT"
                        state["last_decision"] = (
                            "目前符合保護條件；Observe Only 不發送 E2 RC Control"
                        )
                elif protected:
                    state["state"] = "PROTECTING"
                    state["last_decision"] = (
                        f"通話進行中：UE1/UE2 Offered {video_scale * 100:.0f}%，"
                        "UE3 100%"
                    )
                elif sampled_at < cooldown_until:
                    state["state"] = "COOLDOWN"
                    state["last_decision"] = "語音結束，等待 cooldown 後恢復基線"
                else:
                    state["state"] = "OBSERVING"
                    state["last_decision"] = (
                        "UE3 通話品質正常，維持 baseline"
                        if voice_active
                        else "等待 UE3 語音通話"
                    )

                if (
                    protected
                    and not voice_active
                    and sampled_at >= cooldown_until
                    and arguments.mode == "closed_loop"
                    and rc_ready
                ):
                    set_traffic_scale(
                        traffic_control_file, 1.0, "voice_call_finished"
                    )
                    restore_result = apply_rc_policy(
                        bridge,
                        baseline_policies,
                        sst=sst,
                        sd=sd,
                        timeout_seconds=rc_timeout,
                    )
                    state["last_rc"] = restore_result
                    if restore_result["success"]:
                        protected = False
                        state["traffic_shaping_factor"] = 1.0
                        state["state"] = "OBSERVING"
                        state["current_policy"] = "BASELINE · UE1/UE2/UE3 max 100%"
                        state["last_decision"] = "語音已結束，Offered Load 與 RC 已恢復基線"
                        append_event(
                            state, "policy_restored", "Traffic pacing 與 E2SM-RC 基線已恢復"
                        )
                    else:
                        state["state"] = "ERROR"
                        state["last_error"] = restore_result["error"]
                        state["last_decision"] = "恢復基線失敗，請停止實驗檢查 gNB"
                        append_event(state, "restore_failed", state["last_decision"])
                        rc_ready = False

                if state["state"] != previous_state:
                    append_event(
                        state,
                        "state_changed",
                        f"{previous_state} → {state['state']}",
                    )
                    previous_state = state["state"]

                state["ues"] = {
                    ue: {
                        "offered_bps": (
                            offered_bps(video_jobs.get(ue))
                            if ue in video_jobs
                            else float(voice_progress.get("offered_bps", 0.0) or 0.0)
                        ),
                        "delivered_bps": (
                            rx.get(ue, 0.0) + tx.get(ue, 0.0)
                            if ue in video_jobs
                            else float(
                                voice_progress.get("received_bps", 0.0) or 0.0
                            )
                        ),
                        "latency_ms": voice_latency
                        if ue == "ue3"
                        else latency.get(ue),
                        "loss_percent": voice_loss if ue == "ue3" else loss.get(ue),
                        "jitter_ms": voice_jitter if ue == "ue3" else None,
                        "delivery_ratio": voice_progress.get("delivery_ratio")
                        if ue == "ue3"
                        else None,
                    }
                    for ue in ("ue1", "ue2", "ue3")
                }
                state["voice_active"] = voice_active
                state["total_video_offered_bps"] = total_offered
                state["total_video_delivered_bps"] = total_delivered
                state["consecutive_bad_samples"] = bad_samples
                if state["state"] != "ERROR":
                    state["last_error"] = None
                voice_was_active = voice_active
            except (
                HTTPError,
                URLError,
                TimeoutError,
                ValueError,
                json.JSONDecodeError,
            ) as error:
                state["state"] = "ERROR"
                state["last_error"] = str(error)
                state["last_decision"] = (
                    "無法讀取 Manager/Prometheus 指標，未執行新的控制"
                )
            state["last_sample_at"] = sampled_at
            write_state(arguments.state_file, state)
            deadline = time.monotonic() + sample_interval
            while not stop_requested and time.monotonic() < deadline:
                time.sleep(min(0.1, deadline - time.monotonic()))
    finally:
        restore_result = None
        set_traffic_scale(traffic_control_file, 1.0, "voiceguard_stopped")
        state["traffic_shaping_factor"] = 1.0
        if arguments.mode == "closed_loop" and rc_ready:
            restore_result = apply_rc_policy(
                bridge,
                baseline_policies,
                sst=sst,
                sd=sd,
                timeout_seconds=rc_timeout,
            )
            state["last_rc"] = restore_result
        state["running"] = False
        if restore_result is None and state.get("state") == "ERROR":
            state["last_decision"] = state.get(
                "last_decision", "VoiceGuard 因錯誤停止"
            )
            append_event(state, "stopped_with_error", state["last_decision"])
        elif restore_result is not None and not restore_result["success"]:
            state["state"] = "ERROR"
            state["last_error"] = restore_result["error"]
            state["last_decision"] = "VoiceGuard 已停止，但 E2SM-RC 基線恢復失敗"
            append_event(state, "restore_failed", state["last_decision"])
        else:
            state["state"] = "OFF"
            state["current_policy"] = (
                "BASELINE · UE1/UE2/UE3 max 100%"
                if arguments.mode == "closed_loop"
                else "baseline (Observe Only never changed PRB)"
            )
            state["last_decision"] = "VoiceGuard 已停止；基線已恢復"
            append_event(state, "stopped", "VoiceGuard 已安全停止")
        write_state(arguments.state_file, state)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
