"""Offline tests for kpm_exporter.py (no RIC or gNB required).

Run: python3 -m pytest monitoring/exporters/test_kpm_exporter.py
"""

from __future__ import annotations

import importlib.util
import json
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def load_exporter(tmp_path: Path, monkeypatch, slots: list[int]):
    active = tmp_path / "active-run.json"
    active.write_text(json.dumps({"run_id": "run-1", "ue_slots": slots}))
    monkeypatch.setenv("ACTIVE_RUN", str(active))
    monkeypatch.setenv("ORAN_LAB_ROOT", str(tmp_path))
    spec = importlib.util.spec_from_file_location("kpm_exporter_under_test", HERE / "kpm_exporter.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def line(**fields) -> str:
    return json.dumps({"type": "kpm", "seq": 1, "ts_ms": int(time.time() * 1000), **fields})


# Values captured from the live OCUDU gNB on 2026-09-29.
UE_VIDEO = {"DRB.UEThpDl": 11692.0, "DRB.UEThpUl": 226.0, "DRB.RlcSduDelayDl": 26.1,
            "RRU.PrbUsedDl": 18, "RRU.PrbTotDl": 16, "DRB.RlcPacketDropRateDl": 0}
UE_VOICE = {"DRB.UEThpDl": 148.0, "DRB.UEThpUl": 149.0, "DRB.RlcSduDelayDl": 12.5,
            "RRU.PrbUsedDl": 0, "DRB.AirIfDelayUl": None}


def test_ue_reports_are_normalised_and_mapped_by_admission_order(tmp_path, monkeypatch):
    exporter = load_exporter(tmp_path, monkeypatch, [1, 2, 3])
    exporter.handle_line(line(scope="ue", ue_id_type=1, gnb_cu_ue_f1ap_id=1, periods=[UE_VIDEO]), None)
    exporter.handle_line(line(scope="ue", ue_id_type=1, gnb_cu_ue_f1ap_id=2, periods=[UE_VOICE]), None)
    exporter.handle_line(line(scope="ue", ue_id_type=1, gnb_cu_ue_f1ap_id=7, periods=[UE_VOICE]), None)

    snapshot = exporter.snapshot()
    by_id = {item["gnb_cu_ue_f1ap_id"]: item for item in snapshot["ues"]}
    assert snapshot["run_id"] == "run-1"
    assert by_id[1]["ue"] == "ue2"
    assert by_id[2]["ue"] == "ue3"
    assert by_id[7]["ue"] == "unknown"  # re-attached UE: no guessed identity
    assert by_id[1]["values"]["thp_dl_kbps"] == 11692.0
    assert abs(by_id[1]["values"]["rlc_sdu_delay_dl_ms"] - 2.61) < 1e-9  # raw unit 0.1 ms
    assert by_id[2]["values"]["air_if_delay_ul_ms"] is None  # no value stays missing, never 0
    assert not by_id[1]["stale"]


def test_prometheus_output_keeps_raw_units_and_skips_missing_values(tmp_path, monkeypatch):
    exporter = load_exporter(tmp_path, monkeypatch, [1, 2, 3])
    exporter.handle_line(line(scope="cell", periods=[{"RRU.PrbTotDl": 25, "DRB.UEThpDl": 16726.0}]), None)
    exporter.handle_line(line(scope="ue", ue_id_type=1, gnb_cu_ue_f1ap_id=2, periods=[UE_VOICE]), None)
    exporter.handle_line(json.dumps({"type": "subscription", "success": True}), None)

    text = exporter.build_metrics().decode()
    assert ('oran_kpm_value{run_id="run-1",scope="cell",ue="cell",ue_f1ap_id="",'
            'measurement="RRU.PrbTotDl",unit="percent"} 25.0') in text
    assert ('oran_kpm_value{run_id="run-1",scope="ue",ue="ue3",ue_f1ap_id="2",'
            'measurement="DRB.RlcSduDelayDl",unit="0.1ms"} 12.5') in text
    assert "DRB.AirIfDelayUl" not in text
    assert 'oran_kpm_ue_reported{run_id="run-1"} 1' in text
    assert 'oran_kpm_subscriptions_accepted{run_id="run-1"} 1' in text


def test_stale_ue_reports_are_hidden_from_prometheus(tmp_path, monkeypatch):
    exporter = load_exporter(tmp_path, monkeypatch, [1])
    old = int((time.time() - 60) * 1000)
    exporter.handle_line(json.dumps({"type": "kpm", "seq": 1, "ts_ms": old, "scope": "ue",
                                     "ue_id_type": 1, "gnb_cu_ue_f1ap_id": 0, "periods": [UE_VIDEO]}), None)
    assert exporter.snapshot()["ues"][0]["stale"] is True
    assert 'scope="ue"' not in exporter.build_metrics().decode()


def test_raw_log_is_written_into_the_run_snapshot(tmp_path, monkeypatch):
    exporter = load_exporter(tmp_path, monkeypatch, [1])
    run_dir = tmp_path / "experiments" / "runs" / "run-1"
    run_dir.mkdir(parents=True)
    raw = exporter.RawLog()
    exporter.handle_line(line(scope="ue", ue_id_type=1, gnb_cu_ue_f1ap_id=0, periods=[UE_VIDEO]),
                         raw.for_run("run-1"))
    raw.close()
    record = json.loads((run_dir / "kpm.jsonl").read_text().splitlines()[0])
    assert record["gnb_cu_ue_f1ap_id"] == 0
