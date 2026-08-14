"""Shared state, action and SLA definitions for the 3 UE VoiceGuard RF demo."""

from __future__ import annotations

import json
import os
import tempfile
import time
from pathlib import Path
from statistics import median
from typing import Any


VIDEO_UES = ("ue1", "ue2")
VOICE_UES = ("ue3",)
POLICY_SCALES = {
    "EQUAL_100": 1.00,
    "LIGHT_85": 0.85,
    "MEDIUM_70": 0.70,
    "STRONG_40": 0.40,
}
POLICY_ORDER = tuple(POLICY_SCALES)

# Counts and UE-active flags are intentionally excluded: in this fixed 3 UE
# experiment they are constants and therefore contain no predictive signal.
FEATURE_NAMES = (
    "video_offered_mbps",
    "video_delivered_mbps",
    "video_delivery_ratio",
    "video_gap_mbps",
    "video_worst_delivery_ratio",
    "video_imbalance_mbps",
    "ue1_offered_mbps",
    "ue2_offered_mbps",
    "voice_delivery_ratio",
    "voice_loss_percent",
    "voice_jitter_ms",
    "voice_rtt_p95_ms",
)

SLA = {
    "delivery_ratio_min": 0.95,
    "loss_percent_max": 2.0,
    "jitter_ms_max": 30.0,
    # A clean 3 UE software-radio call sits around 35-40 ms. 60 ms catches a
    # meaningful congestion-induced regression while retaining headroom for
    # ordinary scheduler jitter.
    "rtt_p95_ms_max": 60.0,
}


def atomic_write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w") as output:
            json.dump(payload, output, ensure_ascii=False, indent=2)
            output.write("\n")
        os.replace(temporary_name, path)
    finally:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass


def write_video_policy(
    path: Path,
    base_scales: dict[str, float],
    policy: str,
    reason: str,
) -> None:
    """Apply a scenario load and a candidate protection policy atomically."""
    policy_scale = POLICY_SCALES[policy]
    factors = {
        ue: min(1.0, max(0.1, float(base_scales.get(ue, 1.0)) * policy_scale))
        for ue in VIDEO_UES
    }
    atomic_write_json(
        path,
        {
            "updated_at": time.time(),
            "reason": reason,
            "scenario_base_scales": {ue: float(base_scales.get(ue, 1.0)) for ue in VIDEO_UES},
            "policy": policy,
            "policy_scale": policy_scale,
            "ues": {**factors, **{ue: 1.0 for ue in VOICE_UES}},
        },
    )


def write_traffic_scale(path: Path, factor: float, reason: str) -> None:
    """Compatibility actuator used by the shared RF runtime."""
    factor = min(1.0, max(0.1, float(factor)))
    policy = min(POLICY_SCALES, key=lambda name: abs(POLICY_SCALES[name] - factor))
    write_video_policy(path, {ue: 1.0 for ue in VIDEO_UES}, policy, reason)


def progress_of(job: dict[str, Any] | None) -> dict[str, Any]:
    if not job:
        return {}
    progress = (job.get("result") or {}).get("progress") or {}
    return progress if isinstance(progress, dict) else {}


def active_jobs(jobs: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    active = {"QUEUED", "RUNNING", "STOP_REQUESTED"}
    return {
        str(job["ue"]): job
        for job in jobs
        if job.get("status") in active and job.get("ue")
    }


def number(value: Any, default: float = 0.0) -> float:
    try:
        parsed = float(value)
        return parsed if parsed == parsed else default
    except (TypeError, ValueError):
        return default


def extract_features(
    jobs: list[dict[str, Any]],
    delivered_bps_by_ue: dict[str, float] | None = None,
) -> tuple[dict[str, float], dict[str, dict[str, float]]]:
    by_ue = active_jobs(jobs)
    video_metrics: dict[str, dict[str, float]] = {}
    ue_metrics: dict[str, dict[str, float]] = {}

    for ue in VIDEO_UES:
        job = by_ue.get(ue)
        if not job or job.get("traffic_type") != "short_video":
            continue
        progress = progress_of(job)
        configured = number((job.get("parameters") or {}).get("offered_load_mbps")) * 1_000_000
        offered = number(progress.get("offered_bps"), configured)
        progress_delivered = number(progress.get("delivered_bps"))
        measured = (delivered_bps_by_ue or {}).get(ue)
        raw_delivered = number(measured, progress_delivered) if measured is not None else progress_delivered
        # HTTP progress and interface counters are instantaneous burst rates,
        # whereas offered_bps is a paced average. A burst may legitimately be
        # much faster than offered load; cap it before deriving delivery/gap
        # features so that the model cannot learn an impossible >100% ratio.
        delivered = min(offered, raw_delivered) if offered else raw_delivered
        delivery = min(1.0, delivered / offered) if offered else 0.0
        values = {
            "offered_bps": offered,
            "delivered_bps": delivered,
            "delivery_ratio": delivery,
        }
        video_metrics[ue] = values
        ue_metrics[ue] = values

    voice_job = by_ue.get("ue3")
    voice_progress = progress_of(voice_job) if voice_job and voice_job.get("traffic_type") == "rtp_voice" else {}
    voice_offered = number(voice_progress.get("offered_bps"))
    voice_received = number(voice_progress.get("received_bps"))
    voice_values = {
        "offered_bps": voice_offered,
        "delivered_bps": voice_received,
        "delivery_ratio": number(
            voice_progress.get("delivery_ratio"),
            min(1.0, voice_received / voice_offered) if voice_offered else 0.0,
        ),
        "loss_percent": number(voice_progress.get("loss_percent"), 100.0),
        "jitter_ms": number(
            voice_progress.get("jitter_rolling_ms", voice_progress.get("jitter_ms")),
            5000.0,
        ),
        "rtt_p95_ms": number(
            voice_progress.get("rtt_p95_rolling_ms", voice_progress.get("rtt_p95_ms")),
            5000.0,
        ),
    }
    if voice_job and voice_job.get("traffic_type") == "rtp_voice":
        ue_metrics["ue3"] = voice_values

    offered_by_ue = {ue: video_metrics.get(ue, {}).get("offered_bps", 0.0) for ue in VIDEO_UES}
    delivered_by_ue = {ue: video_metrics.get(ue, {}).get("delivered_bps", 0.0) for ue in VIDEO_UES}
    delivery_ratios = [item["delivery_ratio"] for item in video_metrics.values()]
    video_offered = sum(offered_by_ue.values())
    video_delivered = sum(delivered_by_ue.values())
    features = {
        "video_offered_mbps": video_offered / 1_000_000,
        "video_delivered_mbps": video_delivered / 1_000_000,
        "video_delivery_ratio": min(1.0, video_delivered / video_offered) if video_offered else 0.0,
        "video_gap_mbps": max(0.0, video_offered - video_delivered) / 1_000_000,
        "video_worst_delivery_ratio": min(delivery_ratios, default=0.0),
        "video_imbalance_mbps": abs(offered_by_ue["ue1"] - offered_by_ue["ue2"]) / 1_000_000,
        "ue1_offered_mbps": offered_by_ue["ue1"] / 1_000_000,
        "ue2_offered_mbps": offered_by_ue["ue2"] / 1_000_000,
        "voice_delivery_ratio": voice_values["delivery_ratio"],
        "voice_loss_percent": voice_values["loss_percent"],
        "voice_jitter_ms": voice_values["jitter_ms"],
        "voice_rtt_p95_ms": voice_values["rtt_p95_ms"],
    }
    return features, ue_metrics


def feature_vector(features: dict[str, Any]) -> list[float]:
    return [number(features.get(name)) for name in FEATURE_NAMES]


def voice_sla_ok(features: dict[str, Any]) -> bool:
    return (
        number(features.get("voice_delivery_ratio")) >= SLA["delivery_ratio_min"]
        and number(features.get("voice_loss_percent"), 100.0) <= SLA["loss_percent_max"]
        and number(features.get("voice_jitter_ms"), 5000.0) <= SLA["jitter_ms_max"]
        and number(features.get("voice_rtt_p95_ms"), 5000.0) <= SLA["rtt_p95_ms_max"]
    )


def median_features(samples: list[dict[str, Any]]) -> dict[str, float]:
    return {
        name: median([number(sample.get(name)) for sample in samples])
        for name in FEATURE_NAMES
    }
