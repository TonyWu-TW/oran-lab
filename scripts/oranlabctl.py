#!/usr/bin/env python3
"""Headless lifecycle controller for the local O-RAN lab.

Only exposes fixed, allow-listed operations. Start/stop must run as root;
status and preflight are read-only.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any


LAB = Path(os.environ.get("ORAN_LAB_ROOT", "/home/zju/Desktop/oran-lab")).resolve()
RUN_DIR = LAB / "run" / "manager"
LOG_DIR = LAB / "logs" / "manager"
PID_FILE = RUN_DIR / "processes.json"
ADMISSION_STAGE_FILE = RUN_DIR / "ue-admission-stage"

RIC = LAB / "src/flexric/build/examples/ric/nearRT-RIC"
GNB = LAB / "src/ocudu/build/apps/gnb/gnb"
GNB_CONFIG = LAB / "config/ocudu/gnb-fdd-srsue-zmq-open5gs-multiue.yml"
SRSUE = LAB / "src/srsRAN_4G/build/srsue/src/srsue"
UE_CONFIG_DIR = LAB / "config/srsue/multiue"
BROKER = LAB / "radio/broker/build/multi_ue_scenario.py"
EXPERIMENTS_ROOT = LAB / "experiments" / "runs"
ACTIVE_CONFIG = EXPERIMENTS_ROOT / "active-run.json"

MAX_UES = 10
UE_SLOTS = tuple(range(1, MAX_UES + 1))
UE_NAMES = tuple(f"ue{slot}" for slot in UE_SLOTS)


def ue_base_port(slot: int) -> int:
    # 3001 is reserved by the local Grafana service.
    return 2000 + slot * 100 if slot < 10 else 3100


ZMQ_PORTS = (2000, 2001, *(
    port
    for slot in UE_SLOTS
    for port in (ue_base_port(slot), ue_base_port(slot) + 1)
))


@dataclass(frozen=True)
class Component:
    name: str
    command: list[str]
    cwd: Path
    log: Path
    stdin_fifo: Path | None = None
    run_as_user: str | None = None
    environment: dict[str, str] | None = None


def emit(data: Any, as_json: bool) -> None:
    if as_json:
        print(json.dumps(data, ensure_ascii=False, indent=2))
    elif isinstance(data, dict):
        for key, value in data.items():
            print(f"{key}: {value}")
    else:
        print(data)


def require_root() -> None:
    if os.geteuid() != 0:
        raise PermissionError("This operation must run as root")


def read_registry() -> dict[str, dict[str, Any]]:
    if not PID_FILE.exists():
        return {}
    try:
        return json.loads(PID_FILE.read_text())
    except (OSError, json.JSONDecodeError):
        return {}


def write_registry(registry: dict[str, dict[str, Any]]) -> None:
    RUN_DIR.mkdir(parents=True, exist_ok=True)
    temporary = PID_FILE.with_suffix(".tmp")
    temporary.write_text(json.dumps(registry, indent=2) + "\n")
    temporary.replace(PID_FILE)


def process_info(pid: int) -> dict[str, Any]:
    proc = Path(f"/proc/{pid}")
    if not proc.exists():
        return {"pid": pid, "running": False}
    try:
        command = (proc / "cmdline").read_bytes().replace(b"\0", b" ").decode().strip()
    except OSError:
        command = ""
    return {"pid": pid, "running": True, "command": command}


def port_in_use(port: int) -> bool:
    result = subprocess.run(
        ["ss", "-lnt"], capture_output=True, text=True, check=False
    )
    return any(line.split()[3].endswith(f":{port}") for line in result.stdout.splitlines()[1:] if len(line.split()) >= 4)


def systemd_active(unit: str) -> bool:
    return subprocess.run(
        ["systemctl", "is-active", "--quiet", unit], check=False
    ).returncode == 0


def docker_running(name: str) -> bool:
    result = subprocess.run(
        ["docker", "inspect", "-f", "{{.State.Running}}", name],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode == 0 and result.stdout.strip() == "true"


def http_ready(host: str, port: int, path: str = "/") -> bool:
    try:
        with socket.create_connection((host, port), timeout=0.5) as connection:
            connection.sendall(f"GET {path} HTTP/1.0\r\nHost: {host}\r\n\r\n".encode())
            return connection.recv(32).startswith(b"HTTP/")
    except OSError:
        return False


def platform_status() -> dict[str, Any]:
    registry = read_registry()
    components = {name: process_info(int(meta["pid"])) for name, meta in registry.items()}
    return {
        "state": "RUNNING" if any(item["running"] for item in components.values()) else "STOPPED",
        "components": components,
        "services": {
            unit: systemd_active(unit)
            for unit in ("open5gs-nrfd", "open5gs-amfd", "open5gs-smfd", "open5gs-upfd")
        },
        "mongodb": docker_running("open5gs-mongodb"),
        "prometheus": http_ready("127.0.0.1", 9095, "/-/ready"),
        "grafana": http_ready("127.0.0.1", 3001, "/api/health"),
        "ports": {str(port): port_in_use(port) for port in ZMQ_PORTS},
    }


def load_runtime_config() -> dict[str, Any]:
    fallback = {
        "gnb_config": str(GNB_CONFIG),
        "ue_configs": [str(UE_CONFIG_DIR / f"ue{index}.conf") for index in UE_SLOTS],
        "ue_slots": list(UE_SLOTS),
        "broker": str(BROKER),
    }
    if not ACTIVE_CONFIG.exists():
        return fallback
    try:
        data = json.loads(ACTIVE_CONFIG.read_text())
        paths = [data["gnb_config"], data["broker"], *data["ue_configs"]]
        ue_configs = data["ue_configs"]
        ue_slots = data.get("ue_slots", list(range(1, len(ue_configs) + 1)))
        if not 1 <= len(ue_configs) <= MAX_UES:
            raise ValueError("controller requires 1 to 10 UE configs")
        if (
            len(ue_slots) != len(ue_configs)
            or len(set(ue_slots)) != len(ue_slots)
            or any(slot not in UE_SLOTS for slot in ue_slots)
        ):
            raise ValueError("ue_slots must contain unique values in range 1..10")
        root = EXPERIMENTS_ROOT.resolve()
        for raw_path in paths:
            Path(raw_path).resolve().relative_to(root)
        data["ue_slots"] = ue_slots
        return data
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"invalid active-run configuration: {exc}") from exc


def preflight(config: dict[str, Any] | None = None) -> dict[str, Any]:
    config = config or load_runtime_config()
    checks: list[dict[str, Any]] = []

    def add(check_id: str, ok: bool, message: str) -> None:
        checks.append({"id": check_id, "status": "pass" if ok else "fail", "message": message})

    for label, path in (
        ("nearRT-RIC", RIC),
        ("gnb", GNB),
        ("gnb-config", Path(config["gnb_config"])),
        ("srsue", SRSUE),
        ("broker", Path(config["broker"])),
    ):
        add(f"file-{label}", path.exists(), str(path))
    for index, raw_path in zip(config["ue_slots"], config["ue_configs"]):
        path = Path(raw_path)
        add(f"ue{index}-config", path.exists(), str(path))
    add("mongodb", docker_running("open5gs-mongodb"), "Open5GS MongoDB")
    for unit in ("open5gs-nrfd", "open5gs-amfd", "open5gs-smfd", "open5gs-upfd"):
        add(unit, systemd_active(unit), unit)
    registry = read_registry()
    registered_running = any(process_info(int(meta["pid"]))["running"] for meta in registry.values())
    add("manager-not-running", not registered_running, "No registered experiment processes")
    required_ports = [2000, 2001]
    required_ports.extend(
        port
        for slot in config["ue_slots"]
        for port in (ue_base_port(slot), ue_base_port(slot) + 1)
    )
    busy = [port for port in required_ports if port_in_use(port)]
    add("zmq-ports", not busy, f"busy={busy}" if busy else "all free")
    free_gb = shutil.disk_usage(LAB).free / (1024**3)
    add("disk-space", free_gb >= 10, f"{free_gb:.1f} GiB free")
    return {"ok": all(item["status"] == "pass" for item in checks), "checks": checks}


def wait_for_log(
    path: Path, needles: tuple[str, ...], timeout: float, pid: int | None = None
) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            content = path.read_text(errors="replace")
        except OSError:
            content = ""
        if all(needle in content for needle in needles):
            return True
        if pid is not None and not Path(f"/proc/{pid}").exists():
            return False
        time.sleep(0.5)
    return False


def wait_for_ports(ports: tuple[int, ...], timeout: float, pid: int | None = None) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if all(port_in_use(port) for port in ports):
            return True
        if pid is not None and not Path(f"/proc/{pid}").exists():
            return False
        time.sleep(0.25)
    return False


def set_admission_stage(stage: int, ue_count: int) -> None:
    """Tell the broker which UE signals may enter the shared RF path."""
    if not 1 <= stage <= ue_count <= MAX_UES:
        raise ValueError("UE admission stage must be within the active UE count")
    RUN_DIR.mkdir(parents=True, exist_ok=True)
    temporary = ADMISSION_STAGE_FILE.with_suffix(".tmp")
    temporary.write_text(f"{stage}\n")
    os.chmod(temporary, 0o644)
    temporary.replace(ADMISSION_STAGE_FILE)


def run_checked(command: list[str], **kwargs: Any) -> None:
    subprocess.run(command, check=True, **kwargs)


def prepare_network(slots: list[int]) -> None:
    for slot in slots:
        namespace = f"ue{slot}"
        existing = subprocess.run(["ip", "netns", "list"], capture_output=True, text=True, check=True).stdout
        if namespace not in {line.split()[0] for line in existing.splitlines() if line.strip()}:
            run_checked(["ip", "netns", "add", namespace])
    run_checked(["sysctl", "-w", "net.ipv4.ip_forward=1"], stdout=subprocess.DEVNULL)
    rules = [
        (["iptables", "-t", "nat", "-C", "POSTROUTING", "-s", "10.45.0.0/16", "!", "-o", "ogstun", "-j", "MASQUERADE"],
         ["iptables", "-t", "nat", "-A", "POSTROUTING", "-s", "10.45.0.0/16", "!", "-o", "ogstun", "-j", "MASQUERADE"]),
        (["iptables", "-C", "FORWARD", "-s", "10.45.0.0/16", "-i", "ogstun", "-j", "ACCEPT"],
         ["iptables", "-I", "FORWARD", "1", "-s", "10.45.0.0/16", "-i", "ogstun", "-j", "ACCEPT"]),
        (["iptables", "-C", "FORWARD", "-d", "10.45.0.0/16", "-o", "ogstun", "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT"],
         ["iptables", "-I", "FORWARD", "2", "-d", "10.45.0.0/16", "-o", "ogstun", "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT"]),
    ]
    for check, create in rules:
        if subprocess.run(check, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False).returncode != 0:
            run_checked(create)


def start_component(component: Component, registry: dict[str, dict[str, Any]]) -> None:
    command = component.command
    if component.run_as_user:
        command = ["runuser", "-u", component.run_as_user, "--", *command]
    fifo_fd: int | None = None
    if component.stdin_fifo:
        component.stdin_fifo.unlink(missing_ok=True)
        os.mkfifo(component.stdin_fifo)
        fifo_fd = os.open(component.stdin_fifo, os.O_RDWR | os.O_NONBLOCK)
    component.log.parent.mkdir(parents=True, exist_ok=True)
    log_handle = component.log.open("wb")
    environment = os.environ.copy()
    environment.update(component.environment or {})
    process = subprocess.Popen(
        command,
        cwd=component.cwd,
        stdin=fifo_fd if fifo_fd is not None else subprocess.DEVNULL,
        stdout=log_handle,
        stderr=subprocess.STDOUT,
        env=environment,
        start_new_session=True,
        close_fds=True,
    )
    log_handle.close()
    if fifo_fd is not None:
        os.close(fifo_fd)
    registry[component.name] = {
        "pid": process.pid,
        "command": command,
        "log": str(component.log),
        "started_at": time.time(),
    }
    write_registry(registry)


def components(config: dict[str, Any]) -> list[Component]:
    line_buffered = ["/usr/bin/stdbuf", "-oL", "-eL"]
    items = [
        Component(
            "nearRT-RIC",
            [*line_buffered, str(RIC)],
            RIC.parents[3],
            LOG_DIR / "nearRT-RIC.log",
            run_as_user="zju",
        ),
        Component(
            "gnb",
            [*line_buffered, str(GNB), "-c", str(config["gnb_config"])],
            GNB.parent,
            LOG_DIR / "gnb.log",
            RUN_DIR / "gnb.stdin",
        ),
        Component(
            "broker",
            ["/usr/bin/python3", str(config["broker"])],
            LAB,
            LOG_DIR / "broker.log",
            run_as_user="zju",
            environment={"QT_QPA_PLATFORM": "offscreen"},
        ),
    ]
    for slot, ue_config in zip(config["ue_slots"], config["ue_configs"]):
        items.append(Component(
            f"ue{slot}",
            [*line_buffered, str(SRSUE), str(ue_config)],
            SRSUE.parent,
            LOG_DIR / f"ue{slot}.log",
            RUN_DIR / f"ue{slot}.stdin",
        ))
    return items


def start() -> dict[str, Any]:
    require_root()
    config = load_runtime_config()
    result = preflight(config)
    if not result["ok"]:
        raise RuntimeError(json.dumps(result, ensure_ascii=False))
    RUN_DIR.mkdir(parents=True, exist_ok=True)
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    for path in LOG_DIR.glob("*.log"):
        path.unlink()
    slots = config["ue_slots"]
    prepare_network(slots)
    registry: dict[str, dict[str, Any]] = {}
    try:
        # All UE ZMQ sources must exist for the GNU Radio add block, but
        # only one UE at a time is admitted to Random Access. The broker reads
        # this stage file and temporarily mutes the later UE RF paths.
        set_admission_stage(1, len(slots))
        for component in components(config):
            start_component(component, registry)
            if component.name == "nearRT-RIC":
                time.sleep(2)
            elif component.name == "gnb":
                # The ZMQ lower PHY needs the broker's REP endpoint before it
                # can make forward progress, so readiness is checked after the
                # broker has started.
                time.sleep(1)
            elif component.name == "broker":
                broker_ports = (2001, *(
                    ue_base_port(slot) for slot in slots
                ))
                if not wait_for_ports(
                    broker_ports,
                    10,
                    int(registry[component.name]["pid"]),
                ):
                    raise RuntimeError("Broker failed ZMQ readiness")
                if not wait_for_log(
                    LOG_DIR / "gnb.log",
                    ("N2: Connection to AMF", "E2: Connection to Near-RT-RIC"),
                    60,
                    int(registry["gnb"]["pid"]),
                ):
                    raise RuntimeError("gNB failed N2/E2 readiness after Broker startup")
            elif component.name.startswith("ue"):
                time.sleep(0.5)
        for stage, slot in enumerate(slots, 1):
            set_admission_stage(stage, len(slots))
            log = LOG_DIR / f"ue{slot}.log"
            if not wait_for_log(
                log,
                ("RRC Connected", "PDU Session Establishment successful"),
                60,
                int(registry[f"ue{slot}"]["pid"]),
            ):
                raise RuntimeError(f"ue{slot} failed attach/PDU readiness")
            run_checked(["ip", "netns", "exec", f"ue{slot}", "ip", "route", "replace", "default", "dev", "tun_srsue"])
        return platform_status()
    except Exception:
        stop()
        raise


def terminate_groups(pids: list[int], timeout: float = 4.0) -> None:
    """Stop component process groups concurrently.

    Waiting four seconds per component made a 10 UE shutdown take almost one
    minute.  All groups can use the same grace period safely.
    """
    remaining: set[int] = set()
    for pid in pids:
        try:
            os.killpg(pid, signal.SIGTERM)
            remaining.add(pid)
        except (ProcessLookupError, PermissionError):
            continue
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline and remaining:
        remaining = {pid for pid in remaining if Path(f"/proc/{pid}").exists()}
        time.sleep(0.2)
    for pid in remaining:
        try:
            os.killpg(pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass


def stop(remove_active_config: bool = True) -> dict[str, Any]:
    require_root()
    registry = read_registry()
    ue_components = sorted(
        (name for name in registry if name in UE_NAMES),
        key=lambda name: int(name[2:]),
        reverse=True,
    )
    order = ["broker", *ue_components, "gnb", "nearRT-RIC"]
    terminate_groups([
        int(registry[name]["pid"]) for name in order if name in registry
    ])
    for fifo in RUN_DIR.glob("*.stdin"):
        fifo.unlink(missing_ok=True)
    ADMISSION_STAGE_FILE.unlink(missing_ok=True)
    PID_FILE.unlink(missing_ok=True)
    if remove_active_config:
        ACTIVE_CONFIG.unlink(missing_ok=True)
    return platform_status()


def recover_ue(ue: str) -> dict[str, Any]:
    """Rebuild the radio stack to clear UE, GNU Radio and ZMQ IQ backlogs."""
    require_root()
    if ue not in UE_NAMES:
        raise ValueError("UE must be in range ue1..ue10")
    # Keep active-run.json so start() recreates the exact same immutable Run
    # snapshot. Rebuilding Broker and gNB is necessary because old UL IQ
    # samples can already be queued outside the UE process.
    load_runtime_config()
    stop(remove_active_config=False)
    recovered = start()
    component = recovered.get("components", {}).get(ue, {})
    return {"ok": True, "ue": ue, "component": component, "platform": recovered, "scope": "radio-stack"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("status", "preflight", "start", "stop", "recover-ue"))
    parser.add_argument("--ue", choices=UE_NAMES)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    try:
        if args.command == "status":
            data = platform_status()
        elif args.command == "preflight":
            data = preflight()
        elif args.command == "start":
            data = start()
        elif args.command == "stop":
            data = stop()
        else:
            if not args.ue:
                raise ValueError("recover-ue requires --ue")
            data = recover_ue(args.ue)
        emit(data, args.json)
        return 0
    except Exception as exc:
        emit({"ok": False, "error": str(exc)}, True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
