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
    losses = {slot: 0.0 for slot in range(1, 11)}
    content = render_broker(losses)
    assert "UE admission stage" in content
    assert "ACTIVE_UE_SLOTS = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]" in content
    assert "CONFIGURED_PATH_LOSSES = {1: 0.0" in content
    assert "ZMQ_HIGH_WATER_MARK = 10" in content
    assert "ul_sum = blocks.add_vcc(1)" in content
    assert "MUTED_PATH_LOSS_DB = 200.0" in content
    assert "position <= stage" in content
