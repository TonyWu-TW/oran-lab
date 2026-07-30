from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import subprocess


RUNNER_PATH = Path(__file__).resolve().parents[3] / "scripts" / "oranlab-traffic.py"
SPEC = importlib.util.spec_from_file_location("oranlab_traffic", RUNNER_PATH)
assert SPEC and SPEC.loader
runner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runner)


class DummyServer:
    def __init__(self, *args, **kwargs):
        pass

    def serve_forever(self):
        pass

    def shutdown(self):
        pass

    def server_close(self):
        pass


def test_dynamic_video_emits_offered_load_progress(monkeypatch, capsys):
    monkeypatch.setattr(runner.http.server, "ThreadingHTTPServer", DummyServer)
    monkeypatch.setattr(
        runner,
        "curl_request",
        lambda job, size_bytes, upload: (True, size_bytes, 4.0, None),
    )
    runner.stop_requested.clear()
    result = runner.http_job({
        "ue": "ue1",
        "traffic_type": "short_video",
        "direction": "DL",
        "target": "10.45.0.1",
        "port": 5299,
        "duration_seconds": 0.04,
        "params": {
            "offered_load_mbps": 0.8,
            "traffic_pattern": "wave",
            "variation_percent": 30,
            "peak_limit_mbps": 1.2,
            "random_seed": 1234,
            "pattern_period_seconds": 20,
            "segment_interval_ms": 10,
        },
    })
    progress_lines = [line for line in capsys.readouterr().out.splitlines() if '"event": "progress"' in line]
    assert len(progress_lines) >= 2
    assert 0 < result["offered_bps_average"] <= 1_200_000
    assert result["successful_requests"] >= 2


def test_udp_report_derives_receiver_rate_when_server_returns_zero():
    report = {
        "end": {"sum": {"bits_per_second": 1_000_000, "seconds": 10, "bytes": 1_250_000}},
        "server_output_json": {
            "end": {
                "sum": {
                    "bits_per_second": 0,
                    "seconds": 10,
                    "bytes": 1_250_000,
                    "lost_percent": 20,
                    "lost_packets": 20,
                    "packets": 100,
                }
            }
        },
    }
    completed = subprocess.CompletedProcess(["iperf3"], 0, "", "")
    result = runner.parse_iperf_report(
        completed,
        json.dumps(report),
        "",
        {"transport": "udp", "direction": "UL"},
    )
    assert result["receiver_bps"] == 800_000
