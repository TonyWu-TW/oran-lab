#!/usr/bin/env python3
"""E2SM-KPM exporter for the managed O-RAN lab.

Runs the native FlexRIC collector (``oranlab_kpm``), which subscribes to the
gNB's KPM RAN Function over E2 and prints one JSON line per measurement
report.  This process keeps the latest cell-level and UE-level report, and
serves them as:

* ``/metrics``   Prometheus text format (scraped as job ``oran-kpm-exporter``)
* ``/kpm.json``  normalised snapshot used by the Experiment Manager and xApps

Every raw collector line is also appended to ``<run snapshot>/kpm.jsonl`` so
each experiment keeps its own E2 measurement record.

UE identity: OCUDU reports UEs by gNB-CU-UE-F1AP-ID.  The lifecycle controller
admits UEs strictly one after another on a freshly started gNB, so the n-th
F1AP ID belongs to the n-th admitted UE slot.  This mapping is exposed as
``ue_mapping="admission_order"``; IDs beyond the admitted UE count (for
example after a radio link failure and re-attach) are reported as ``unknown``.
"""

from __future__ import annotations

import http.server
import json
import os
import re
import signal
import subprocess
import threading
import time
from pathlib import Path
from typing import Any


LAB = Path(os.environ.get("ORAN_LAB_ROOT", "/home/zju/Desktop/oran-lab"))
COLLECTOR = Path(os.environ.get(
    "ORANLAB_KPM_COLLECTOR",
    str(LAB / "src/flexric/build/examples/xApp/c/oranlab_kpm/oranlab_kpm"),
))
ACTIVE_RUN = Path(os.environ.get("ACTIVE_RUN", str(LAB / "experiments/runs/active-run.json")))
LISTEN = os.environ.get("LISTEN", "127.0.0.1")
PORT = int(os.environ.get("PORT", "9106"))
STALE_SECONDS = float(os.environ.get("ORANLAB_KPM_STALE_SECONDS", "5"))
RAW_LOG_LIMIT_BYTES = int(os.environ.get("ORANLAB_KPM_RAW_LIMIT_BYTES", str(200 * 1024 * 1024)))

# Units exactly as produced by OCUDU's e2sm_kpm_du_meas_provider_impl.cpp.
# ``scale`` converts the raw value into the normalised unit exposed in JSON.
MEASUREMENTS: dict[str, dict[str, Any]] = {
    "DRB.UEThpDl": {"key": "thp_dl_kbps", "raw_unit": "kbps", "scale": 1.0},
    "DRB.UEThpUl": {"key": "thp_ul_kbps", "raw_unit": "kbps", "scale": 1.0},
    "DRB.RlcSduTransmittedVolumeDL": {"key": "rlc_volume_dl_kbit", "raw_unit": "kbit", "scale": 1.0},
    "DRB.RlcSduTransmittedVolumeUL": {"key": "rlc_volume_ul_kbit", "raw_unit": "kbit", "scale": 1.0},
    "DRB.RlcSduDelayDl": {"key": "rlc_sdu_delay_dl_ms", "raw_unit": "0.1ms", "scale": 0.1},
    "DRB.RlcDelayUl": {"key": "rlc_delay_ul_ms", "raw_unit": "0.1ms", "scale": 0.1},
    "DRB.AirIfDelayUl": {"key": "air_if_delay_ul_ms", "raw_unit": "0.1ms", "scale": 0.1},
    "DRB.RlcPacketDropRateDl": {"key": "rlc_drop_rate_dl_percent", "raw_unit": "percent", "scale": 1.0},
    "RRU.PrbUsedDl": {"key": "prb_used_dl", "raw_unit": "prb", "scale": 1.0},
    "RRU.PrbUsedUl": {"key": "prb_used_ul", "raw_unit": "prb", "scale": 1.0},
    "RRU.PrbAvailDl": {"key": "prb_avail_dl", "raw_unit": "prb", "scale": 1.0},
    "RRU.PrbAvailUl": {"key": "prb_avail_ul", "raw_unit": "prb", "scale": 1.0},
    "RRU.PrbTotDl": {"key": "prb_util_dl_percent", "raw_unit": "percent", "scale": 1.0},
    "RRU.PrbTotUl": {"key": "prb_util_ul_percent", "raw_unit": "percent", "scale": 1.0},
    "RACH.PreambleDedCell": {"key": "rach_preamble_ded", "raw_unit": "count", "scale": 1.0},
}
LABEL_VALUE = re.compile(r"[^A-Za-z0-9_.:\-]")

state_lock = threading.Lock()
state: dict[str, Any] = {
    "collector": {"running": False, "pid": None, "starts": 0, "last_exit_code": None, "last_error": None},
    "subscriptions": [],
    "cell": None,
    "ues": {},
    "lines": 0,
    "parse_errors": 0,
}
stopping = threading.Event()
collector_process: subprocess.Popen[str] | None = None


def active_run() -> dict[str, Any]:
    try:
        data = json.loads(ACTIVE_RUN.read_text())
    except (OSError, json.JSONDecodeError):
        return {"run_id": "none", "ue_slots": []}
    run_id = str(data.get("run_id", "none"))
    if not re.fullmatch(r"[a-zA-Z0-9-]+", run_id):
        run_id = "unknown"
    slots = [int(slot) for slot in data.get("ue_slots", []) if isinstance(slot, int)]
    return {"run_id": run_id, "ue_slots": slots}


def ue_name(f1ap_id: int | None, slots: list[int]) -> str:
    if f1ap_id is None or not 0 <= f1ap_id < len(slots):
        return "unknown"
    return f"ue{slots[f1ap_id]}"


def normalise(raw: dict[str, Any]) -> dict[str, float | None]:
    values: dict[str, float | None] = {}
    for name, value in raw.items():
        spec = MEASUREMENTS.get(name)
        key = spec["key"] if spec else re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")
        if value is None:
            values[key] = None
        else:
            values[key] = round(float(value) * (spec["scale"] if spec else 1.0), 6)
    return values


def handle_line(line: str, raw_log) -> None:
    try:
        message = json.loads(line)
    except json.JSONDecodeError:
        with state_lock:
            state["parse_errors"] += 1
        return
    if raw_log is not None:
        raw_log.write(line + "\n")
        raw_log.flush()
    kind = message.get("type")
    with state_lock:
        state["lines"] += 1
        if kind == "subscription":
            state["subscriptions"].append(message)
            return
        if kind != "kpm":
            return
        periods = message.get("periods") or [{}]
        # Keep the most recent granularity period of the report.
        raw = periods[-1] if periods else {}
        entry = {
            "ts_ms": message.get("ts_ms"),
            "seq": message.get("seq"),
            "raw": raw,
            "values": normalise(raw),
        }
        if message.get("scope") == "cell":
            state["cell"] = entry
        elif message.get("scope") == "ue":
            f1ap_id = message.get("gnb_cu_ue_f1ap_id")
            key = str(f1ap_id) if f1ap_id is not None else f"type{message.get('ue_id_type')}"
            entry["gnb_cu_ue_f1ap_id"] = f1ap_id
            entry["ue_id_type"] = message.get("ue_id_type")
            state["ues"][key] = entry


class RawLog:
    """Append-only per-run JSONL writer with a size cap."""

    def __init__(self) -> None:
        self.run_id: str | None = None
        self.handle = None
        self.written = 0

    def for_run(self, run_id: str):
        if run_id == self.run_id:
            return self if self.handle else None
        self.close()
        self.run_id = run_id
        directory = LAB / "experiments" / "runs" / run_id
        if run_id in {"none", "unknown"} or not directory.is_dir():
            return None
        try:
            self.handle = (directory / "kpm.jsonl").open("a")
            self.written = self.handle.tell()
        except OSError:
            self.handle = None
        return self if self.handle else None

    def write(self, text: str) -> None:
        if self.handle is None or self.written >= RAW_LOG_LIMIT_BYTES:
            return
        self.handle.write(text)
        self.written += len(text)

    def flush(self) -> None:
        if self.handle is not None:
            self.handle.flush()

    def close(self) -> None:
        if self.handle is not None:
            self.handle.close()
        self.handle = None


def collector_loop() -> None:
    global collector_process
    raw_log = RawLog()
    backoff = 2.0
    while not stopping.is_set():
        if not os.access(COLLECTOR, os.X_OK):
            with state_lock:
                state["collector"]["last_error"] = f"collector not executable: {COLLECTOR}"
            stopping.wait(5)
            continue
        with state_lock:
            state["subscriptions"] = []
            state["collector"]["starts"] += 1
        started = time.monotonic()
        process = subprocess.Popen(
            [str(COLLECTOR)],
            cwd=str(COLLECTOR.parent),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        collector_process = process
        with state_lock:
            state["collector"].update(running=True, pid=process.pid, last_error=None)
        assert process.stdout is not None
        for line in process.stdout:
            stripped = line.strip()
            if not stripped:
                continue
            if stripped.startswith("{"):
                handle_line(stripped, raw_log.for_run(active_run()["run_id"]))
            elif "ORANLAB_KPM_ERROR" in stripped:
                with state_lock:
                    state["collector"]["last_error"] = stripped
            else:
                # FlexRIC's own start-up chatter goes to the exporter log.
                print(stripped, flush=True)
        process.wait()
        with state_lock:
            state["collector"].update(running=False, pid=None, last_exit_code=process.returncode)
        collector_process = None
        if stopping.is_set():
            break
        # A collector that ran for a while is restarted quickly; one that
        # fails immediately (no RIC yet, no KPM function) backs off.
        backoff = 2.0 if time.monotonic() - started > 30 else min(30.0, backoff * 2)
        print(f"collector exited with {process.returncode}; restarting in {backoff:.0f}s", flush=True)
        stopping.wait(backoff)
    raw_log.close()


def snapshot() -> dict[str, Any]:
    run = active_run()
    now_ms = time.time() * 1000
    with state_lock:
        cell = state["cell"]
        ues = []
        for key, entry in sorted(state["ues"].items(), key=lambda item: item[0]):
            age = (now_ms - entry["ts_ms"]) / 1000 if entry.get("ts_ms") else None
            ues.append({
                "gnb_cu_ue_f1ap_id": entry.get("gnb_cu_ue_f1ap_id"),
                "ue_id_type": entry.get("ue_id_type"),
                "ue": ue_name(entry.get("gnb_cu_ue_f1ap_id"), run["ue_slots"]),
                "age_seconds": age,
                "stale": age is None or age > STALE_SECONDS,
                "values": entry["values"],
                "raw": entry["raw"],
            })
        cell_age = (now_ms - cell["ts_ms"]) / 1000 if cell and cell.get("ts_ms") else None
        return {
            "run_id": run["run_id"],
            "source": "e2sm-kpm",
            "ue_mapping": "admission_order",
            "collector": dict(state["collector"]),
            "subscriptions": list(state["subscriptions"]),
            "cell": None if cell is None else {
                "age_seconds": cell_age,
                "stale": cell_age is None or cell_age > STALE_SECONDS,
                "values": cell["values"],
                "raw": cell["raw"],
            },
            "ues": ues,
            "units": {spec["key"]: spec["raw_unit"] if spec["scale"] == 1.0 else "ms" for spec in MEASUREMENTS.values()},
            "lines": state["lines"],
            "parse_errors": state["parse_errors"],
        }


def label(value: object) -> str:
    return LABEL_VALUE.sub("_", str(value))


def build_metrics() -> bytes:
    data = snapshot()
    run_id = label(data["run_id"])
    lines = [
        "# HELP oran_kpm_value E2SM-KPM measurement exactly as reported by the E2 node (see unit label).",
        "# TYPE oran_kpm_value gauge",
    ]
    rows: list[tuple[dict[str, str], dict[str, Any]]] = []
    if data["cell"] and not data["cell"]["stale"]:
        rows.append(({"scope": "cell", "ue": "cell", "ue_f1ap_id": ""}, data["cell"]["raw"]))
    for ue in data["ues"]:
        if ue["stale"]:
            continue
        rows.append((
            {"scope": "ue", "ue": ue["ue"], "ue_f1ap_id": str(ue["gnb_cu_ue_f1ap_id"])},
            ue["raw"],
        ))
    for labels, raw in rows:
        for name, value in raw.items():
            if value is None:
                continue
            unit = MEASUREMENTS.get(name, {}).get("raw_unit", "unknown")
            text = ",".join(
                f'{key}="{label(item)}"' for key, item in (
                    ("run_id", run_id), *labels.items(), ("measurement", name), ("unit", unit),
                )
            )
            lines.append(f"oran_kpm_value{{{text}}} {float(value)}")
    lines.extend((
        "# HELP oran_kpm_ue_reported Number of UEs with a fresh E2SM-KPM report.",
        "# TYPE oran_kpm_ue_reported gauge",
        f'oran_kpm_ue_reported{{run_id="{run_id}"}} {sum(1 for ue in data["ues"] if not ue["stale"])}',
        "# HELP oran_kpm_collector_up Whether the native E2 KPM collector process is running.",
        "# TYPE oran_kpm_collector_up gauge",
        f'oran_kpm_collector_up{{run_id="{run_id}"}} {1 if data["collector"]["running"] else 0}',
        "# HELP oran_kpm_subscriptions_accepted Accepted E2 KPM subscriptions.",
        "# TYPE oran_kpm_subscriptions_accepted gauge",
        f'oran_kpm_subscriptions_accepted{{run_id="{run_id}"}} '
        f'{sum(1 for item in data["subscriptions"] if item.get("success"))}',
        "",
    ))
    return "\n".join(lines).encode()


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self) -> None:  # noqa: N802
        if self.path == "/metrics":
            body, content_type = build_metrics(), "text/plain; version=0.0.4"
        elif self.path == "/kpm.json":
            body, content_type = json.dumps(snapshot(), ensure_ascii=False).encode(), "application/json"
        else:
            self.send_response(404)
            self.end_headers()
            return
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, _: str, *__: object) -> None:
        return


class QuietServer(http.server.ThreadingHTTPServer):
    daemon_threads = True

    def handle_error(self, request, client_address) -> None:  # noqa: ANN001
        # A scraper closing its connection early is not an exporter failure.
        return


def main() -> None:
    server = QuietServer((LISTEN, PORT), Handler)

    def shutdown(_: int, __: object) -> None:
        stopping.set()
        process = collector_process
        if process is not None and process.poll() is None:
            process.send_signal(signal.SIGTERM)
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)
    worker = threading.Thread(target=collector_loop, daemon=True)
    worker.start()
    print(f"KPM exporter listening on http://{LISTEN}:{PORT}/metrics (collector {COLLECTOR})", flush=True)
    server.serve_forever()
    worker.join(timeout=8)


if __name__ == "__main__":
    main()
