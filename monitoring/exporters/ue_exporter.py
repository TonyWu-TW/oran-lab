#!/usr/bin/env python3
import http.server
import os
import re
import subprocess
import time

UE_NS = os.environ.get("UE_NS", "ue1")
UE_IFACE = os.environ.get("UE_IFACE", "tun_srsue")
PING_TARGET = os.environ.get("PING_TARGET", "10.45.0.1")
LISTEN = os.environ.get("LISTEN", "127.0.0.1")
PORT = int(os.environ.get("PORT", "9105"))

last = {"rx": None, "tx": None, "time": None}

def run(cmd, timeout=2):
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True, timeout=timeout)

def ns_cat(path):
    result = run(["ip", "netns", "exec", UE_NS, "cat", path])
    if result.returncode != 0:
        return None
    try:
        return int(result.stdout.strip())
    except ValueError:
        return None

def ping_once():
    result = run(["ip", "netns", "exec", UE_NS, "ping", "-c", "1", "-W", "1", PING_TARGET])
    if result.returncode != 0:
        return -1.0, 1
    match = re.search(r"time=([0-9.]+)", result.stdout)
    if not match:
        return -1.0, 1
    return float(match.group(1)), 0

def build_metrics():
    now = time.time()
    rx = ns_cat(f"/sys/class/net/{UE_IFACE}/statistics/rx_bytes")
    tx = ns_cat(f"/sys/class/net/{UE_IFACE}/statistics/tx_bytes")
    latency_ms, ping_loss = ping_once()

    rx_rate = 0.0
    tx_rate = 0.0
    if rx is not None and tx is not None and last["time"] is not None:
        dt = max(now - last["time"], 0.001)
        rx_rate = max(rx - last["rx"], 0) * 8.0 / dt
        tx_rate = max(tx - last["tx"], 0) * 8.0 / dt

    if rx is not None and tx is not None:
        last["rx"] = rx
        last["tx"] = tx
        last["time"] = now

    labels = f'ue="{UE_NS}",iface="{UE_IFACE}"'
    lines = [
        "# HELP oran_ue_rx_bytes_total UE RX bytes on tun interface.",
        "# TYPE oran_ue_rx_bytes_total counter",
        f"oran_ue_rx_bytes_total{{{labels}}} {rx if rx is not None else 0}",
        "# HELP oran_ue_tx_bytes_total UE TX bytes on tun interface.",
        "# TYPE oran_ue_tx_bytes_total counter",
        f"oran_ue_tx_bytes_total{{{labels}}} {tx if tx is not None else 0}",
        "# HELP oran_ue_rx_bps UE RX throughput estimated by exporter.",
        "# TYPE oran_ue_rx_bps gauge",
        f"oran_ue_rx_bps{{{labels}}} {rx_rate}",
        "# HELP oran_ue_tx_bps UE TX throughput estimated by exporter.",
        "# TYPE oran_ue_tx_bps gauge",
        f"oran_ue_tx_bps{{{labels}}} {tx_rate}",
        "# HELP oran_ue_ping_latency_ms Ping latency from UE namespace.",
        "# TYPE oran_ue_ping_latency_ms gauge",
        f'oran_ue_ping_latency_ms{{ue="{UE_NS}",target="{PING_TARGET}"}} {latency_ms}',
        "# HELP oran_ue_ping_loss Ping loss indicator. 0 means success, 1 means failed.",
        "# TYPE oran_ue_ping_loss gauge",
        f'oran_ue_ping_loss{{ue="{UE_NS}",target="{PING_TARGET}"}} {ping_loss}',
        "",
    ]
    return "\n".join(lines).encode()

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
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

    def log_message(self, fmt, *args):
        return

if __name__ == "__main__":
    server = http.server.ThreadingHTTPServer((LISTEN, PORT), Handler)
    print(f"UE exporter listening on http://{LISTEN}:{PORT}/metrics")
    server.serve_forever()
