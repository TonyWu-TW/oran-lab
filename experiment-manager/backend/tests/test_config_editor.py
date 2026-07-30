from __future__ import annotations

import pytest

from app.config_generator import (
    BASE_GNB,
    merge_sensitive,
    redact_sensitive,
    render_broker,
    validate_gnb_config,
)


def test_redacted_round_trip_preserves_secrets():
    original = "[usim]\nopc = 00112233445566778899aabbccddeeff\nk = ffeeddccbbaa99887766554433221100\nimsi = 999700000000001\n"
    browser = redact_sensitive(original).replace("imsi = 999700000000001", "imsi = 999700000000009")
    merged = merge_sensitive(original, browser)
    assert "opc = 00112233445566778899aabbccddeeff" in merged
    assert "k = ffeeddccbbaa99887766554433221100" in merged
    assert "imsi = 999700000000009" in merged
    assert "(redacted)" not in merged


def test_browser_cannot_replace_secret():
    original = "[usim]\nk = ffeeddccbbaa99887766554433221100\n"
    with pytest.raises(ValueError, match="cannot be edited"):
        merge_sensitive(original, "[usim]\nk = 00000000000000000000000000000000\n")


def test_gnb_config_validation_accepts_managed_baseline():
    validate_gnb_config(BASE_GNB.read_text())


def test_gnb_config_validation_rejects_broken_zmq_topology():
    content = BASE_GNB.read_text().replace("tx_port=tcp://*:2000", "tx_port=tcp://*:2999")
    with pytest.raises(ValueError, match=r"tx_port=tcp://\*:2000"):
        validate_gnb_config(content)


def test_broker_generation_stages_equal_path_losses():
    content = render_broker({1: 0.0, 2: 0.0, 3: 0.0})
    assert "UE admission stage" in content
    assert "self.zmq_hwm = zmq_hwm = 10" in content
    assert "blocks_throttle_1" in content
    assert "tb.set_ue2_path_loss_db(200.0)" in content
    assert "desired_path_losses[3] if requested_stage >= 3" in content
