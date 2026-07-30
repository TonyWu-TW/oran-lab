from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path
from typing import Any


LAB_ROOT = Path(os.environ.get("ORAN_LAB_ROOT", "/home/zju/Desktop/oran-lab"))
CONTROL = LAB_ROOT / "scripts" / "oranlabctl.py"


class ControlError(RuntimeError):
    pass


def invoke(action: str, privileged: bool = False, timeout: int = 120) -> dict[str, Any]:
    if action not in {"status", "preflight", "start", "stop"}:
        raise ValueError("unsupported control action")
    command = [str(CONTROL), action, "--json"]
    if privileged and os.geteuid() != 0:
        command = ["sudo", "-n", *command]
    result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
    output = result.stdout.strip() or result.stderr.strip()
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        raise ControlError(output or f"controller exited {result.returncode}") from exc
    if result.returncode != 0:
        raise ControlError(payload.get("error", output))
    return payload


def recover_ue(ue: str, timeout: int = 90) -> dict[str, Any]:
    if ue not in {"ue1", "ue2", "ue3"}:
        raise ValueError("unsupported UE")
    command = [str(CONTROL), "recover-ue", "--ue", ue, "--json"]
    if os.geteuid() != 0:
        command = ["sudo", "-n", *command]
    result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
    output = result.stdout.strip() or result.stderr.strip()
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        raise ControlError(output or f"controller exited {result.returncode}") from exc
    if result.returncode != 0:
        raise ControlError(payload.get("error", output))
    return payload
