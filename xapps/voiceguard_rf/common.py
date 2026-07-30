"""Shared feature and policy definitions for VoiceGuard RF V2."""

from __future__ import annotations

import json
import os
import tempfile
import time
from pathlib import Path
from statistics import median
from typing import Any, Iterable


VIDEO_UES = tuple(f"ue{index}" for index in range(1, 9))
VOICE_UES = ("ue9", "ue10")
POLICY_SCALES = {
    "EQUAL_100": 1.00,
    "LIGHT_85": 0.85,
    "MEDIUM_70": 0.70,
    "STRONG_40": 0.40,
}
POLICY_ORDER = tuple(POLICY_SCALES)
FEATURE_NAMES = (
    "video_ue_count",
    "active_voice_count",
    "video_base_demand_mbps",
    "video_offered_mbps",
    "video_delivered_mbps",
    "video_delivery_ratio",
    "voice_delivery_ratio",
    "voice_loss_percent",
    "voice_jitter_ms",
    "voice_rtt_p95_ms",
    "ue9_active",
    "ue10_active",
)
SLA = {
    "delivery_ratio_min": 0.95,
    "loss_percent_max": 2.0,
    "jitter_ms_max": 30.0,
    "rtt_p95_ms_max": 120.0,
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


def write_traffic_scale(
    path: Path,
    scale: float,
    reason: str,
    *,
    video_ues: Iterable[str] = VIDEO_UES,
    voice_ues: Iterable[str] = VOICE_UES,
) -> None:
    atomic_write_json(
        path,
        {
            "updated_at": time.time(),
            "reason": reason,
            "ues": {
                **{ue: scale for ue in video_ues},
                **{ue: 1.0 for ue in voice_ues},
            },
        },
    )


def progress_of(job: dict[str, Any] | None) -> dict[str, Any]:
    if not job:
        return {}
    result = job.get("result") or {}
    progress = result.get("progress") or {}
    return progress if isinstance(progress, dict) else {}


def active_jobs(jobs: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    active = {"QUEUED", "RUNNING", "STOP_REQUESTED"}
    return {
        str(job["ue"]): job
        for job in jobs
        if job.get("status") in active and job.get("ue")
    }


def _number(value: Any, default: float = 0.0) -> float:
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
    video_jobs = {
        ue: by_ue[ue]
        for ue in VIDEO_UES
        if ue in by_ue and by_ue[ue].get("traffic_type") == "short_video"
    }
    voice_jobs = {
        ue: by_ue[ue]
        for ue in VOICE_UES
        if ue in by_ue and by_ue[ue].get("traffic_type") == "rtp_voice"
    }

    video_base = video_offered = video_delivered = 0.0
    ue_metrics: dict[str, dict[str, float]] = {}
    for ue, job in video_jobs.items():
        progress = progress_of(job)
        configured = _number((job.get("parameters") or {}).get("offered_load_mbps")) * 1_000_000
        offered = _number(progress.get("offered_bps"), configured)
        shaping = max(0.1, _number(progress.get("shaping_factor"), 1.0))
        base = _number(progress.get("base_offered_bps"), offered / shaping)
        delivered = (
            _number(delivered_bps_by_ue.get(ue))
            if delivered_bps_by_ue is not None
            else min(offered, _number(progress.get("delivered_bps")))
        )
        video_base += base
        video_offered += offered
        video_delivered += delivered
        ue_metrics[ue] = {
            "offered_bps": offered,
            "delivered_bps": delivered,
            "delivery_ratio": min(1.0, delivered / offered) if offered else 0.0,
        }

    voice_values: list[dict[str, float]] = []
    for ue, job in voice_jobs.items():
        progress = progress_of(job)
        offered = _number(progress.get("offered_bps"))
        received = _number(progress.get("received_bps"))
        delivery = _number(
            progress.get("delivery_ratio"),
            min(1.0, received / offered) if offered else 0.0,
        )
        values = {
            "offered_bps": offered,
            "delivered_bps": received,
            "delivery_ratio": delivery,
            "loss_percent": _number(progress.get("loss_percent"), 100.0),
            "jitter_ms": _number(
                progress.get("jitter_rolling_ms", progress.get("jitter_ms"))
            ),
            "rtt_p95_ms": _number(
                progress.get("rtt_p95_rolling_ms", progress.get("rtt_p95_ms")),
                5000.0,
            ),
        }
        voice_values.append(values)
        ue_metrics[ue] = values

    # The call is only as healthy as its worst active voice UE.
    voice_delivery = min((item["delivery_ratio"] for item in voice_values), default=1.0)
    voice_loss = max((item["loss_percent"] for item in voice_values), default=0.0)
    voice_jitter = max((item["jitter_ms"] for item in voice_values), default=0.0)
    voice_rtt = max((item["rtt_p95_ms"] for item in voice_values), default=0.0)
    features = {
        "video_ue_count": float(len(video_jobs)),
        "active_voice_count": float(len(voice_jobs)),
        "video_base_demand_mbps": video_base / 1_000_000,
        "video_offered_mbps": video_offered / 1_000_000,
        "video_delivered_mbps": video_delivered / 1_000_000,
        "video_delivery_ratio": min(1.0, video_delivered / video_offered)
        if video_offered
        else 0.0,
        "voice_delivery_ratio": voice_delivery,
        "voice_loss_percent": voice_loss,
        "voice_jitter_ms": voice_jitter,
        "voice_rtt_p95_ms": voice_rtt,
        "ue9_active": float("ue9" in voice_jobs),
        "ue10_active": float("ue10" in voice_jobs),
    }
    return features, ue_metrics


def feature_vector(features: dict[str, Any]) -> list[float]:
    return [_number(features.get(name)) for name in FEATURE_NAMES]


def voice_sla_ok(features: dict[str, Any]) -> bool:
    return (
        _number(features.get("active_voice_count")) > 0
        and _number(features.get("voice_delivery_ratio")) >= SLA["delivery_ratio_min"]
        and _number(features.get("voice_loss_percent"), 100.0) <= SLA["loss_percent_max"]
        and _number(features.get("voice_jitter_ms"), 5000.0) <= SLA["jitter_ms_max"]
        and _number(features.get("voice_rtt_p95_ms"), 5000.0) <= SLA["rtt_p95_ms_max"]
    )


def median_features(samples: list[dict[str, float]]) -> dict[str, float]:
    return {
        name: median([_number(sample.get(name)) for sample in samples])
        for name in FEATURE_NAMES
    }


def policy_for_scale(scale: float) -> str:
    return min(POLICY_SCALES, key=lambda name: abs(POLICY_SCALES[name] - scale))
