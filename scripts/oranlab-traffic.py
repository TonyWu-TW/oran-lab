#!/usr/bin/env python3
"""Privileged, allow-listed traffic runner for O-RAN UE namespaces."""

from __future__ import annotations

import argparse
from collections import deque
import http.server
import ipaddress
import json
import math
import os
import random
import re
import signal
import socket
import struct
import subprocess
import sys
import threading
import time
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlparse


active_process: subprocess.Popen[Any] | None = None
server_process: subprocess.Popen[Any] | None = None
stop_requested = threading.Event()


def fail(message: str) -> None:
    print(json.dumps({"ok": False, "error": message}))
    raise SystemExit(1)


def validate(payload: dict[str, Any]) -> dict[str, Any]:
    allowed = {
        "ue", "traffic_type", "application_protocol", "transport", "protocol",
        "direction", "target", "port", "run_mode", "duration_seconds", "params",
        "control_file",
        # Backward-compatible fields used by the original API.
        "duration", "bitrate",
    }
    if set(payload) - allowed:
        fail("unknown traffic parameters")
    ue = str(payload.get("ue", ""))
    if not re.fullmatch(r"ue[1-9][0-9]*", ue):
        fail("invalid UE namespace")
    traffic_type = str(payload.get("traffic_type") or ("ping" if payload.get("protocol") == "ping" else "iperf"))
    if traffic_type not in {"ping", "iperf", "http", "short_video", "social", "navigation", "rtp_voice"}:
        fail("unsupported traffic type")
    transport = str(payload.get("transport") or payload.get("protocol") or "udp").lower()
    if transport not in {"icmp", "tcp", "udp"}:
        fail("invalid transport")
    direction = str(payload.get("direction", "UL")).upper()
    if direction not in {"UL", "DL", "BOTH"}:
        fail("direction must be UL, DL, or BOTH")
    try:
        target = str(ipaddress.ip_address(str(payload.get("target", "10.45.0.1"))))
    except ValueError:
        fail("target must be an IP address")
    port = int(payload.get("port", 5201))
    if not 5201 <= port <= 5299:
        fail("traffic port must be 5201..5299")
    run_mode = str(payload.get("run_mode", "duration"))
    if run_mode not in {"duration", "continuous"}:
        fail("invalid run mode")
    duration_raw = payload.get("duration_seconds", payload.get("duration", 10))
    duration = None if run_mode == "continuous" else int(duration_raw)
    if duration is not None and not 1 <= duration <= 86400:
        fail("duration must be 1..86400 seconds")
    params = dict(payload.get("params") or {})
    if "bitrate" in payload:
        params.setdefault("bitrate", payload["bitrate"])
    return {
        "ue": ue,
        "traffic_type": traffic_type,
        "application_protocol": str(payload.get("application_protocol", traffic_type)),
        "transport": transport,
        "protocol": "ping" if traffic_type == "ping" else transport,
        "direction": direction,
        "target": target,
        "port": port,
        "run_mode": run_mode,
        "duration_seconds": duration,
        "params": params,
        "control_file": str(payload.get("control_file", "")),
    }


def percentile(values: list[float], ratio: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, math.ceil(len(ordered) * ratio) - 1)]


def emit_progress(**values: Any) -> None:
    """Emit a machine-readable progress sample without mixing it with the final result."""
    print(json.dumps({"event": "progress", "timestamp": time.time(), **values}), flush=True)


def shaping_factor(job: dict[str, Any]) -> float:
    path = job.get("control_file")
    if not path:
        return 1.0
    try:
        payload = json.loads(Path(path).read_text())
        factor = float(payload.get("ues", {}).get(job["ue"], 1.0))
        return min(1.0, max(0.1, factor))
    except (OSError, ValueError, TypeError, json.JSONDecodeError):
        return 1.0


def ping_job(job: dict[str, Any]) -> dict[str, Any]:
    global active_process
    interval_ms = int(job["params"].get("interval_ms", 1000))
    packet_size = int(job["params"].get("packet_size", 56))
    command = [
        "ip", "netns", "exec", job["ue"], "ping", "-i", f"{interval_ms / 1000:.3f}",
        "-s", str(packet_size), "-W", "2", job["target"],
    ]
    if job["duration_seconds"] is not None:
        command[5:5] = ["-w", str(job["duration_seconds"])]
    active_process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    stdout, stderr = active_process.communicate()
    exit_code = active_process.returncode
    active_process = None
    loss_match = re.search(r"([0-9.]+)% packet loss", stdout)
    sent_match = re.search(r"(\d+) packets transmitted, (\d+) received", stdout)
    rtt_match = re.search(r"= ([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms", stdout)
    sent = int(sent_match.group(1)) if sent_match else 0
    received = int(sent_match.group(2)) if sent_match else 0
    return {
        "ok": received > 0 or stop_requested.is_set(),
        "traffic_type": "ping",
        "sent": sent,
        "received": received,
        "loss_percent": float(loss_match.group(1)) if loss_match else (100.0 if sent else None),
        "rtt_min_ms": float(rtt_match.group(1)) if rtt_match else None,
        "rtt_avg_ms": float(rtt_match.group(2)) if rtt_match else None,
        "rtt_max_ms": float(rtt_match.group(3)) if rtt_match else None,
        "exit_code": exit_code,
        "error": stderr.strip() or None,
    }


def parse_iperf_report(result: subprocess.CompletedProcess[str] | None, stdout: str, stderr: str, job: dict[str, Any]) -> dict[str, Any]:
    try:
        report = json.loads(stdout)
    except json.JSONDecodeError:
        return {
            "ok": stop_requested.is_set(), "traffic_type": "iperf", "protocol": job["transport"],
            "error": (stderr or stdout or "iperf3 returned no report")[-800:],
        }
    end = report.get("end", {})
    server_end = report.get("server_output_json", {}).get("end", {})
    sender = end.get("sum_sent") or end.get("sum") or {}
    receiver = server_end.get("sum_received") or server_end.get("sum") or end.get("sum_received") or {}
    report_error = report.get("error")
    receiver_bps = receiver.get("bits_per_second")
    receiver_seconds = receiver.get("seconds")
    receiver_bytes = receiver.get("bytes")
    if not receiver_bps and receiver_seconds and receiver_bytes:
        receiver_bps = receiver_bytes * 8.0 / receiver_seconds
    return {
        "ok": not report_error or stop_requested.is_set(),
        "traffic_type": "iperf",
        "protocol": job["transport"],
        "direction": job["direction"],
        "seconds": receiver_seconds or sender.get("seconds"),
        "sender_bps": sender.get("bits_per_second"),
        "receiver_bps": receiver_bps,
        "bytes": receiver_bytes or sender.get("bytes"),
        "retransmits": sender.get("retransmits"),
        "jitter_ms": receiver.get("jitter_ms"),
        "lost_packets": receiver.get("lost_packets"),
        "packets": receiver.get("packets"),
        "loss_percent": receiver.get("lost_percent"),
        "exit_code": result.returncode if result else None,
        "error": report_error or stderr.strip() or None,
    }


def iperf_job(job: dict[str, Any]) -> dict[str, Any]:
    global active_process, server_process
    server_process = subprocess.Popen(
        ["iperf3", "-s", "-1", "-p", str(job["port"]), "-J"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    duration = job["duration_seconds"] if job["duration_seconds"] is not None else 86400
    client = [
        "ip", "netns", "exec", job["ue"], "iperf3", "-c", job["target"],
        "-p", str(job["port"]), "-t", str(duration), "-J", "--get-server-output",
    ]
    if job["transport"] == "udp":
        client.append("-u")
    else:
        client.extend(["--set-mss", "536"])
    client.extend(["-b", str(job["params"].get("bitrate", "750K"))])
    if job["direction"] == "DL":
        client.append("-R")
    try:
        active_process = subprocess.Popen(client, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            stdout, stderr = active_process.communicate(timeout=None if job["duration_seconds"] is None else duration * 4 + 20)
            completed = subprocess.CompletedProcess(client, active_process.returncode, stdout, stderr)
        except subprocess.TimeoutExpired:
            active_process.terminate()
            stdout, stderr = active_process.communicate(timeout=5)
            return {
                "ok": False, "traffic_type": "iperf", "protocol": job["transport"],
                "error_code": "IPERF_TIMEOUT", "error": "iperf3 did not finish before the radio-aware timeout",
            }
        return parse_iperf_report(completed, stdout, stderr, job)
    finally:
        active_process = None
        if server_process and server_process.poll() is None:
            server_process.terminate()
        if server_process:
            try:
                server_process.communicate(timeout=3)
            except subprocess.TimeoutExpired:
                server_process.kill()
        server_process = None


class PayloadHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self) -> None:  # noqa: N802
        query = parse_qs(urlparse(self.path).query)
        size = min(10 * 1024 * 1024, max(1, int(query.get("size", [1024])[0])))
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(size))
        self.end_headers()
        chunk = b"0" * min(65536, size)
        remaining = size
        while remaining:
            piece = chunk[:remaining]
            self.wfile.write(piece)
            remaining -= len(piece)

    def do_POST(self) -> None:  # noqa: N802
        remaining = min(10 * 1024 * 1024, max(0, int(self.headers.get("Content-Length", "0"))))
        while remaining:
            data = self.rfile.read(min(65536, remaining))
            if not data:
                break
            remaining -= len(data)
        body = b'{"ok":true}'
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, _: str, *args: Any) -> None:
        return


def curl_request(job: dict[str, Any], size_bytes: int, upload: bool) -> tuple[bool, int, float, str | None]:
    global active_process
    url = f"http://{job['target']}:{job['port']}/{'upload' if upload else 'payload'}"
    if not upload:
        url += f"?size={size_bytes}"
    command = [
        "ip", "netns", "exec", job["ue"], "curl", "--silent", "--show-error",
        "--output", "/dev/null", "--write-out", "%{http_code} %{size_download} %{size_upload} %{time_total}",
        "--max-time", "20", url,
    ]
    input_data: bytes | None = None
    if upload:
        command[-1:-1] = ["--request", "POST", "--header", "Content-Type: application/octet-stream", "--data-binary", "@-"]
        input_data = b"0" * size_bytes
    active_process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = active_process.communicate(input=input_data)
    return_code = active_process.returncode
    active_process = None
    try:
        code, downloaded, uploaded, elapsed = stdout.decode().strip().split()
        transferred = int(float(uploaded if upload else downloaded))
        return return_code == 0 and code.startswith("2"), transferred, float(elapsed) * 1000, stderr.decode().strip() or None
    except ValueError:
        return False, 0, 0.0, stderr.decode().strip() or stdout.decode(errors="replace")[-300:]


def http_job(job: dict[str, Any]) -> dict[str, Any]:
    server = http.server.ThreadingHTTPServer(("0.0.0.0", job["port"]), PayloadHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    traffic_type = job["traffic_type"]
    params = job["params"]
    if traffic_type == "short_video":
        size_kb, per_cycle, interval_ms = 0, 1, int(params.get("segment_interval_ms", 1000))
    elif traffic_type == "social":
        size_kb = int(params.get("object_size_kb", 180))
        per_cycle, interval_ms = int(params.get("objects_per_cycle", 4)), int(params.get("cycle_interval_ms", 4000))
    elif traffic_type == "navigation":
        size_kb = int(params.get("tile_size_kb", 40))
        per_cycle, interval_ms = int(params.get("tiles_per_cycle", 6)), int(params.get("update_interval_ms", 5000))
    else:
        size_kb, per_cycle, interval_ms = int(params.get("object_size_kb", 256)), 1, int(params.get("interval_ms", 1000))
    deadline = None if job["duration_seconds"] is None else time.monotonic() + job["duration_seconds"]
    successes = failures = transferred = 0
    latencies: list[float] = []
    offered_samples: list[float] = []
    delivered_samples: list[float] = []
    last_error: str | None = None
    started = time.monotonic()
    cycle_number = 0
    random_source = random.Random(int(params.get("random_seed", 1234)))
    adaptive_factor = 1.0
    try:
        while not stop_requested.is_set() and (deadline is None or time.monotonic() < deadline):
            cycle_started = time.monotonic()
            target_offered_bps = 0.0
            if traffic_type == "short_video":
                base_target_mbps = float(params.get("offered_load_mbps", 0.8))
                scale = shaping_factor(job)
                target_mbps = base_target_mbps * scale
                peak_mbps = float(
                    params.get("peak_limit_mbps", max(base_target_mbps, 1.2))
                ) * scale
                variation = int(params.get("variation_percent", 30)) / 100
                pattern = str(params.get("traffic_pattern", "wave"))
                if pattern == "wave":
                    period = max(2, int(params.get("pattern_period_seconds", 20)))
                    factor = 1 + variation * math.sin(2 * math.pi * (time.monotonic() - started) / period)
                elif pattern == "random_burst":
                    factor = 1 + random_source.uniform(-variation, variation)
                    if random_source.random() < 0.12:
                        factor = 1 + variation
                elif pattern == "adaptive":
                    factor = adaptive_factor
                else:
                    factor = 1.0
                offered_mbps = min(peak_mbps, max(0.01, target_mbps * factor))
                target_offered_bps = offered_mbps * 1_000_000
                # One segment per pacing interval makes Offered Load independent of TCP delivery time.
                size_kb = max(1, min(10240, round(target_offered_bps * interval_ms / 8_192_000)))
                offered_samples.append(target_offered_bps)
            cycle_transferred = 0
            for index in range(per_cycle):
                if stop_requested.is_set():
                    break
                upload = job["direction"] == "UL" or (traffic_type == "navigation" and index == per_cycle - 1)
                request_size = 1024 if traffic_type == "navigation" and upload else size_kb * 1024
                ok, byte_count, latency, error = curl_request(job, request_size, upload)
                successes += int(ok)
                failures += int(not ok)
                transferred += byte_count
                cycle_transferred += byte_count
                if latency:
                    latencies.append(latency)
                if error:
                    last_error = error
            cycle_elapsed = max(0.001, time.monotonic() - cycle_started)
            delivered_bps = cycle_transferred * 8 / cycle_elapsed
            delivered_samples.append(delivered_bps)
            if traffic_type == "short_video":
                if str(params.get("traffic_pattern", "wave")) == "adaptive":
                    interval_seconds = interval_ms / 1000
                    if cycle_elapsed > interval_seconds * 0.95 or failures:
                        adaptive_factor = max(0.35, adaptive_factor * 0.82)
                    else:
                        adaptive_factor = min(
                            1 + int(params.get("variation_percent", 30)) / 100,
                            adaptive_factor + 0.06,
                        )
                emit_progress(
                    traffic_type=traffic_type,
                    cycle=cycle_number,
                    offered_bps=target_offered_bps,
                    base_offered_bps=target_offered_bps / scale,
                    shaping_factor=scale,
                    delivered_bps=delivered_bps,
                    segment_size_kb=size_kb,
                    segment_latency_ms=latencies[-1] if latencies else None,
                    successful_requests=successes,
                    failed_requests=failures,
                    bytes=transferred,
                )
            cycle_number += 1
            remaining = interval_ms / 1000 - (time.monotonic() - cycle_started)
            if remaining > 0:
                stop_requested.wait(remaining)
    finally:
        server.shutdown()
        server.server_close()
    elapsed = max(0.001, time.monotonic() - started)
    return {
        "ok": successes > 0 or stop_requested.is_set(),
        "traffic_type": traffic_type,
        "requests": successes + failures,
        "successful_requests": successes,
        "failed_requests": failures,
        "bytes": transferred,
        "average_bps": transferred * 8 / elapsed,
        "offered_bps_average": sum(offered_samples) / len(offered_samples) if offered_samples else None,
        "offered_bps_peak": max(offered_samples) if offered_samples else None,
        "delivered_bps_last": delivered_samples[-1] if delivered_samples else None,
        "latency_avg_ms": sum(latencies) / len(latencies) if latencies else None,
        "latency_p95_ms": percentile(latencies, 0.95),
        "seconds": elapsed,
        "error": last_error if failures else None,
    }


def rtp_client(target: str, port: int, interval_ms: int, bitrate_kbps: int, duration: int) -> int:
    local_stop = threading.Event()

    def request_stop(_: int, __: object) -> None:
        local_stop.set()

    signal.signal(signal.SIGTERM, request_stop)
    signal.signal(signal.SIGINT, request_stop)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(0.1)
    payload_size = max(12, min(1400, int(bitrate_kbps * 1000 / 8 * interval_ms / 1000)))
    deadline = None if duration == 0 else time.monotonic() + duration
    interval_seconds = interval_ms / 1000
    sequence = sent = received = 0
    sent_window = received_window = 0
    rtts: deque[float] = deque(maxlen=5000)
    window_rtts: list[float] = []
    quality_windows: deque[tuple[int, int]] = deque(maxlen=3)
    quality_rtt_windows: deque[list[float]] = deque(maxlen=3)
    metrics_lock = threading.Lock()
    started = time.monotonic()

    def receive_echoes() -> None:
        nonlocal received, received_window
        while not local_stop.is_set():
            try:
                response, _ = sock.recvfrom(2048)
                _, timestamp = struct.unpack("!IQ", response[:12])
                rtt = (time.monotonic_ns() - timestamp) / 1_000_000
                with metrics_lock:
                    received += 1
                    received_window += 1
                    rtts.append(rtt)
                    window_rtts.append(rtt)
            except socket.timeout:
                continue
            except (OSError, struct.error):
                break

    receiver = threading.Thread(target=receive_echoes, daemon=True)
    receiver.start()
    next_send = started
    last_report = started
    next_report = started + 1.0
    try:
        while not local_stop.is_set() and (deadline is None or time.monotonic() < deadline):
            now = time.monotonic()
            if now >= next_report:
                window_seconds = max(0.001, now - last_report)
                with metrics_lock:
                    current_received = received_window
                    current_rtts = list(window_rtts)
                    received_window = 0
                    window_rtts.clear()
                current_sent = sent_window
                quality_windows.append((current_sent, current_received))
                quality_rtt_windows.append(current_rtts)
                quality_sent = sum(item[0] for item in quality_windows)
                quality_received = sum(item[1] for item in quality_windows)
                rolling_rtts = [
                    value for window in quality_rtt_windows for value in window
                ]
                # One echo can legitimately be in flight exactly at the report boundary.
                lost_window = max(0, quality_sent - quality_received - 1)
                jitter_samples = [
                    abs(current - previous)
                    for previous, current in zip(current_rtts, current_rtts[1:])
                ]
                rolling_jitter_samples = [
                    abs(current - previous)
                    for previous, current in zip(rolling_rtts, rolling_rtts[1:])
                ]
                emit_progress(
                    traffic_type="rtp_voice",
                    offered_bps=current_sent * payload_size * 8 / window_seconds,
                    received_bps=current_received * payload_size * 8 / window_seconds,
                    sent_packets_window=current_sent,
                    received_packets_window=current_received,
                    loss_percent=lost_window * 100 / quality_sent if quality_sent else None,
                    delivery_ratio=min(1.0, (quality_received + 1) / quality_sent) if quality_sent else None,
                    jitter_ms=sum(jitter_samples) / len(jitter_samples) if jitter_samples else 0.0,
                    jitter_rolling_ms=(
                        sum(rolling_jitter_samples) / len(rolling_jitter_samples)
                        if rolling_jitter_samples
                        else 0.0
                    ),
                    rtt_avg_ms=sum(current_rtts) / len(current_rtts) if current_rtts else None,
                    rtt_p95_ms=percentile(current_rtts, 0.95),
                    rtt_p95_rolling_ms=percentile(rolling_rtts, 0.95),
                    sent_packets=sent,
                    received_packets=received,
                    payload_size_bytes=payload_size,
                )
                sent_window = 0
                last_report = now
                next_report = now + 1.0
            if now >= next_send:
                packet = struct.pack("!IQ", sequence, time.monotonic_ns()) + b"0" * (payload_size - 12)
                sock.sendto(packet, (target, port))
                sequence += 1
                sent += 1
                sent_window += 1
                next_send += interval_seconds
                # Do not send a large catch-up burst after the process was descheduled.
                if next_send < now - interval_seconds * 3:
                    next_send = now + interval_seconds
            wait_for = max(0.001, min(next_send, next_report) - time.monotonic())
            local_stop.wait(min(wait_for, 0.05))
    finally:
        local_stop.set()
        try:
            sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        sock.close()
        receiver.join(timeout=1)
    elapsed = max(0.001, time.monotonic() - started)
    final_rtts = list(rtts)
    jitter_samples = [abs(current - previous) for previous, current in zip(final_rtts, final_rtts[1:])]
    result = {
        "ok": received > 0,
        "traffic_type": "rtp_voice",
        "sent_packets": sent,
        "received_packets": received,
        "lost_packets": sent - received,
        "loss_percent": (sent - received) * 100 / sent if sent else None,
        "rtt_avg_ms": sum(final_rtts) / len(final_rtts) if final_rtts else None,
        "rtt_p95_ms": percentile(final_rtts, 0.95),
        "jitter_ms": sum(jitter_samples) / len(jitter_samples) if jitter_samples else 0.0,
        "packet_interval_ms": interval_ms,
        "bitrate_kbps": bitrate_kbps,
        "offered_bps_average": sent * payload_size * 8 / elapsed,
        "received_bps_average": received * payload_size * 8 / elapsed,
        "seconds": elapsed,
    }
    print(json.dumps(result), flush=True)
    return 0 if result["ok"] else 1


def rtp_job(job: dict[str, Any]) -> dict[str, Any]:
    global active_process
    echo_stop = threading.Event()
    echo_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    echo_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    echo_socket.bind(("0.0.0.0", job["port"]))
    echo_socket.settimeout(0.2)

    def echo() -> None:
        while not echo_stop.is_set():
            try:
                payload, address = echo_socket.recvfrom(2048)
                echo_socket.sendto(payload, address)
            except socket.timeout:
                continue
            except OSError:
                break

    thread = threading.Thread(target=echo, daemon=True)
    thread.start()
    params = job["params"]
    duration = job["duration_seconds"] or 0
    command = [
        "ip", "netns", "exec", job["ue"], sys.executable, os.path.abspath(__file__),
        "rtp-client", "--target", job["target"], "--port", str(job["port"]),
        "--interval-ms", str(params.get("packet_interval_ms", 20)),
        "--bitrate-kbps", str(params.get("bitrate_kbps", 64)), "--duration", str(duration),
    ]
    try:
        active_process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert active_process.stdout is not None
        assert active_process.stderr is not None
        final_result: dict[str, Any] | None = None
        output_tail: list[str] = []
        for line in active_process.stdout:
            stripped = line.strip()
            if not stripped:
                continue
            output_tail.append(stripped)
            output_tail = output_tail[-10:]
            try:
                message = json.loads(stripped)
            except json.JSONDecodeError:
                continue
            if message.get("event") == "progress":
                print(json.dumps(message), flush=True)
            else:
                final_result = message
        active_process.wait()
        stderr = active_process.stderr.read()
        return final_result or {
            "ok": stop_requested.is_set(),
            "traffic_type": "rtp_voice",
            "error": (stderr or "\n".join(output_tail) or "RTP client returned no result")[-800:],
        }
    finally:
        active_process = None
        echo_stop.set()
        echo_socket.close()
        thread.join(timeout=1)


def terminate(_: int, __: object) -> None:
    stop_requested.set()
    if active_process and active_process.poll() is None:
        try:
            active_process.send_signal(signal.SIGINT)
        except ProcessLookupError:
            pass
    if server_process and server_process.poll() is None:
        server_process.terminate()


def run_job() -> int:
    if os.geteuid() != 0:
        fail("traffic helper must run as root")
    signal.signal(signal.SIGTERM, terminate)
    signal.signal(signal.SIGINT, terminate)
    try:
        payload = json.load(sys.stdin)
    except (json.JSONDecodeError, TypeError):
        fail("request body must be JSON")
    job = validate(payload)
    if job["traffic_type"] == "ping":
        output = ping_job(job)
    elif job["traffic_type"] == "iperf":
        output = iperf_job(job)
    elif job["traffic_type"] == "rtp_voice":
        output = rtp_job(job)
    else:
        output = http_job(job)
    if stop_requested.is_set():
        output["stopped_by_user"] = True
        output["ok"] = True
    print(json.dumps(output))
    return 0 if output.get("ok") else 1


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    run_parser = subparsers.add_parser("run")
    run_parser.add_argument("--json", action="store_true")
    rtp_parser = subparsers.add_parser("rtp-client")
    rtp_parser.add_argument("--target", required=True)
    rtp_parser.add_argument("--port", type=int, required=True)
    rtp_parser.add_argument("--interval-ms", type=int, required=True)
    rtp_parser.add_argument("--bitrate-kbps", type=int, required=True)
    rtp_parser.add_argument("--duration", type=int, required=True)
    args = parser.parse_args()
    if args.command == "rtp-client":
        return rtp_client(args.target, args.port, args.interval_ms, args.bitrate_kbps, args.duration)
    return run_job()


if __name__ == "__main__":
    raise SystemExit(main())
