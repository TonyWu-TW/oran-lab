from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from common import (  # noqa: E402
    FEATURE_NAMES,
    POLICY_ORDER,
    POLICY_SCALES,
    extract_features,
    feature_vector,
    voice_sla_ok,
)


def job(ue: str, traffic_type: str, progress: dict[str, float]) -> dict:
    return {
        "ue": ue,
        "status": "RUNNING",
        "traffic_type": traffic_type,
        "parameters": {"offered_load_mbps": 1.0},
        "result": {"progress": progress},
    }


def test_extracts_prometheus_video_delivery_and_worst_voice_quality():
    jobs = [
        job(
            "ue1",
            "short_video",
            {
                "offered_bps": 800_000,
                "base_offered_bps": 1_000_000,
                "shaping_factor": 0.8,
            },
        ),
        job(
            "ue9",
            "rtp_voice",
            {
                "offered_bps": 96_000,
                "received_bps": 95_000,
                "delivery_ratio": 0.99,
                "loss_percent": 1.0,
                "jitter_rolling_ms": 12,
                "rtt_p95_rolling_ms": 70,
            },
        ),
        job(
            "ue10",
            "rtp_voice",
            {
                "offered_bps": 96_000,
                "received_bps": 90_000,
                "delivery_ratio": 0.96,
                "loss_percent": 1.8,
                "jitter_rolling_ms": 20,
                "rtt_p95_rolling_ms": 110,
            },
        ),
    ]
    features, metrics = extract_features(jobs, {"ue1": 700_000})

    assert features["video_base_demand_mbps"] == 1.0
    assert features["video_offered_mbps"] == 0.8
    assert features["video_delivered_mbps"] == 0.7
    assert features["active_voice_count"] == 2
    assert features["voice_delivery_ratio"] == 0.96
    assert features["voice_loss_percent"] == 1.8
    assert features["voice_jitter_ms"] == 20
    assert features["voice_rtt_p95_ms"] == 110
    assert metrics["ue1"]["delivery_ratio"] == 0.875
    assert voice_sla_ok(features)


def test_voice_sla_rejects_rtt_above_10ue_threshold():
    features = {name: 0.0 for name in FEATURE_NAMES}
    features.update(
        {
            "active_voice_count": 1,
            "voice_delivery_ratio": 1.0,
            "voice_loss_percent": 0.0,
            "voice_jitter_ms": 10.0,
            "voice_rtt_p95_ms": 120.1,
        }
    )
    assert not voice_sla_ok(features)
    assert len(feature_vector(features)) == len(FEATURE_NAMES)


def test_policy_order_is_monotonically_more_protective():
    scales = [POLICY_SCALES[name] for name in POLICY_ORDER]
    assert scales == sorted(scales, reverse=True)
    assert scales == [1.0, 0.85, 0.7, 0.4]
