#!/usr/bin/env python3
"""Prometheus exporter for all managed UE network namespaces."""

from __future__ import annotations

import concurrent.futures
import http.server
import json
import os
import re
import subprocess
import threading
import time
from pathlib import Path


UE_NAMES = tuple(item.strip() for item in os.environ.get("UE_NAMES", "ue1,ue2,ue3").split(",") if item.strip())
UE_IFACE = os.environ.get("UE_IFACE", "tun_srsue")
PING_TARGET = os.environ.get("PING_TARGET", "10.45.0.1")
LISTEN = os.environ.get("LISTEN", "127.0.0.1")
PORT = int(os.environ.get("PORT", "9105"))
ACTIVE_RUN = Path(os.environ.get("ACTIVE_RUN", "/home/zju/Desktop/oran-lab/experiments/runs/active-run.json"))

last: dict[str, dict[str, float | int | None]] = {
    ue: {"rx": None, "tx": None, "time": None} for ue in UE_NAMES
}
state_lock = threading.Lock()


def run(command: list[str], timeout: float = 2) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)


def counter(ue: str, name: str) -> int | None:
    result = run(["ip", "netns", "exec", ue, "cat", f"/sys/class/net/{UE_IFACE}/statistics/{name}"])
    if result.returncode != 0:
        return None
    try:
        return int(result.stdout.strip())
    except ValueError:
        return None


def ping_once(ue: str) -> tuple[float, float]:
    result = run(["ip", "netns", "exec", ue, "ping", "-c", "1", "-W", "1", PING_TARGET], 2)
    latency = re.search(r"time=([0-9.]+)", result.stdout)
    if result.returncode != 0 or not latency:
        return -1.0, 100.0
    return float(latency.group(1)), 0.0


def run_id() -> str:
    try:
        value = json.loads(ACTIVE_RUN.read_text()).get("run_id", "none")
        return value if re.fullmatch(r"[a-zA-Z0-9-]+", value) else "unknown"
    except (OSError, json.JSONDecodeError):
        return "none"


def collect_ue(ue: str, now: float) -> dict[str, float | int]:
    rx = counter(ue, "rx_bytes")
    tx = counter(ue, "tx_bytes")
    latency, loss = ping_once(ue) if rx is not None else (-1.0, 100.0)
    rx_rate = tx_rate = 0.0
    with state_lock:
        previous = last[ue]
        if rx is not None and tx is not None and previous["time"] is not None:
            elapsed = max(now - float(previous["time"]), 0.001)
            rx_rate = max(rx - int(previous["rx"]), 0) * 8.0 / elapsed
            tx_rate = max(tx - int(previous["tx"]), 0) * 8.0 / elapsed
        if rx is not None and tx is not None:
            previous.update(rx=rx, tx=tx, time=now)
    up = 1 if rx is not None and tx is not None else 0
    return {"rx": rx or 0, "tx": tx or 0, "rx_bps": rx_rate, "tx_bps": tx_rate,
            "latency": latency, "loss": loss, "attached": up, "pdu": up}


def build_metrics() -> bytes:
    now = time.time()
    current_run = run_id()
    with concurrent.futures.ThreadPoolExecutor(max_workers=len(UE_NAMES)) as pool:
        values = dict(zip(UE_NAMES, pool.map(lambda ue: collect_ue(ue, now), UE_NAMES)))
    definitions = (
        ("oran_ue_rx_bytes_total", "counter", "UE received bytes.", "rx"),
        ("oran_ue_tx_bytes_total", "counter", "UE transmitted bytes.", "tx"),
        ("oran_ue_rx_bps", "gauge", "UE receive throughput in bits per second.", "rx_bps"),
        ("oran_ue_tx_bps", "gauge", "UE transmit throughput in bits per second.", "tx_bps"),
        ("oran_ue_ping_latency_ms", "gauge", "UE ping latency to the UPF.", "latency"),
        ("oran_ue_ping_loss_percent", "gauge", "UE ping packet loss percentage.", "loss"),
        ("oran_ue_attached", "gauge", "UE network interface is present.", "attached"),
        ("oran_ue_pdu_session_up", "gauge", "UE PDU session interface is up.", "pdu"),
    )
    lines: list[str] = []
    for metric, metric_type, help_text, key in definitions:
        lines.extend((f"# HELP {metric} {help_text}", f"# TYPE {metric} {metric_type}"))
        for ue in UE_NAMES:
            labels = f'run_id="{current_run}",ue="{ue}",iface="{UE_IFACE}"'
            lines.append(f"{metric}{{{labels}}} {values[ue][key]}")
    lines.append("")
    return "\n".join(lines).encode()


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if self.path != "/metrics":
            self.send_response(404)
            self.end_headers()
            return
        data = build_metrics()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; version=0.0.4")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, _: str, *__: object) -> None:
        return


if __name__ == "__main__":
    print(f"Multi-UE exporter listening on http://{LISTEN}:{PORT}/metrics for {','.join(UE_NAMES)}", flush=True)
    http.server.ThreadingHTTPServer((LISTEN, PORT), Handler).serve_forever()

