from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from relabel import load_pair_id, normalized_model_sample  # noqa: E402
from train import policy_acceptable  # noqa: E402


def test_repeated_rounds_share_one_validation_group():
    first = {"ue1_base_scale": "0.15", "ue2_base_scale": "1.0", "round": "1"}
    repeated = {"ue1_base_scale": "0.150000", "ue2_base_scale": "1", "round": "4"}
    assert load_pair_id(first) == "load-u1-015-u2-100"
    assert load_pair_id(first) == load_pair_id(repeated)


def test_policy_window_requires_the_complete_voice_sla():
    passing = {
        "sla_success_ratio": 0.75,
        "voice_delivery_ratio_median": 0.95,
        "voice_loss_percent_median": 2.0,
        "voice_jitter_ms_median": 30.0,
        "voice_rtt_p95_ms_median": 60.0,
    }
    assert policy_acceptable(passing)
    assert not policy_acceptable({**passing, "voice_rtt_p95_ms_median": 60.1})
    assert not policy_acceptable({**passing, "sla_success_ratio": 0.74})


def test_http_burst_rate_is_capped_for_model_features():
    sample = {
        "video_offered_mbps": "1.5",
        "video_delivered_mbps": "8.0",
        "video_delivery_ratio": "1.0",
        "video_gap_mbps": "0",
    }
    normalized = normalized_model_sample(sample)
    assert normalized["video_delivered_mbps"] == 1.5
    assert normalized["video_delivery_ratio"] == 1.0
    assert normalized["video_gap_mbps"] == 0.0
