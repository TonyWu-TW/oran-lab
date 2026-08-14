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
        "parameters": {"offered_load_mbps": 1.5},
        "result": {"progress": progress},
    }


def test_extracts_varying_three_ue_features_and_voice_quality():
    jobs = [
        job("ue1", "short_video", {"offered_bps": 900_000, "delivered_bps": 800_000}),
        job("ue2", "short_video", {"offered_bps": 600_000, "delivered_bps": 400_000}),
        job(
            "ue3",
            "rtp_voice",
            {
                "offered_bps": 96_000,
                "received_bps": 94_000,
                "delivery_ratio": 0.98,
                "loss_percent": 1.0,
                "jitter_rolling_ms": 12,
                "rtt_p95_rolling_ms": 50,
            },
        ),
    ]
    features, metrics = extract_features(jobs)
    assert features["video_offered_mbps"] == 1.5
    assert features["video_delivered_mbps"] == 1.2
    assert round(features["video_gap_mbps"], 6) == 0.3
    assert round(features["video_worst_delivery_ratio"], 6) == round(2 / 3, 6)
    assert features["video_imbalance_mbps"] == 0.3
    assert features["voice_rtt_p95_ms"] == 50
    assert metrics["ue3"]["delivery_ratio"] == 0.98
    assert voice_sla_ok(features)
    assert len(feature_vector(features)) == len(FEATURE_NAMES)


def test_policy_order_becomes_monotonically_more_protective():
    assert [POLICY_SCALES[name] for name in POLICY_ORDER] == [1.0, 0.85, 0.7, 0.4]


def test_missing_prometheus_sample_falls_back_to_job_progress():
    jobs = [
        job("ue1", "short_video", {"offered_bps": 700_000, "delivered_bps": 650_000}),
        job("ue2", "short_video", {"offered_bps": 500_000, "delivered_bps": 450_000}),
        job("ue3", "rtp_voice", {"offered_bps": 96_000, "received_bps": 96_000, "delivery_ratio": 1.0, "loss_percent": 0, "jitter_ms": 5, "rtt_p95_ms": 40}),
    ]
    features, _ = extract_features(jobs, {})
    assert features["video_delivered_mbps"] == 1.1
