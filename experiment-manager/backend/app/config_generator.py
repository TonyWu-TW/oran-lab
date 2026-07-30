from __future__ import annotations

import json
import os
import re
import subprocess
from pathlib import Path

import yaml

from .controller import LAB_ROOT
from .models import Experiment


BASE_UE = LAB_ROOT / "config" / "srsue" / "multiue"
BASE_GNB = LAB_ROOT / "config" / "ocudu" / "gnb-fdd-srsue-zmq-open5gs-multiue.yml"
BASE_BROKER = LAB_ROOT / "radio" / "broker" / "build" / "multi_ue_scenario.py"
ACTIVE_CONFIG = LAB_ROOT / "experiments" / "runs" / "active-run.json"
DEFINITIONS_ROOT = LAB_ROOT / "experiments" / "definitions"
SENSITIVE_LINE = re.compile(
    r"^(\s*)(opc|op|k|pin|password)(\s*=)(.*)$", re.IGNORECASE | re.MULTILINE
)


def bool_text(value: object) -> str:
    return "true" if bool(value) else "false"


def set_section_values(content: str, section: str, values: dict[str, object]) -> str:
    """Replace or add keys inside one srsUE INI section, preserving everything else."""
    lines = content.splitlines()
    header = f"[{section}]"
    try:
        start = lines.index(header)
    except ValueError as exc:
        raise ValueError(f"missing srsUE section {header}") from exc
    end = next((index for index in range(start + 1, len(lines)) if lines[index].startswith("[")), len(lines))
    pending = {key: str(value) for key, value in values.items()}
    output = lines[: start + 1]
    key_pattern = re.compile(r"^\s*#?\s*([A-Za-z0-9_]+)\s*=")
    for line in lines[start + 1 : end]:
        match = key_pattern.match(line)
        if match and match.group(1) in pending:
            key = match.group(1)
            output.append(f"{key:<14}= {pending.pop(key)}")
        else:
            output.append(line)
    output.extend(f"{key:<14}= {value}" for key, value in pending.items())
    output.extend(lines[end:])
    return "\n".join(output) + "\n"


def render_ue_config(source: Path, channel: dict) -> str:
    content = source.read_text()
    emulator_enabled = any(channel.get(key, False) for key in (
        "awgn_enabled", "fading_enabled", "delay_enabled", "rlf_enabled", "hst_enabled"
    ))
    for direction in ("dl", "ul"):
        prefix = f"channel.{direction}"
        content = set_section_values(content, prefix, {"enable": bool_text(emulator_enabled)})
        content = set_section_values(content, f"{prefix}.awgn", {
            "enable": bool_text(channel.get("awgn_enabled")),
            "snr": channel.get("awgn_snr", 30.0),
            "signal_power": channel.get("awgn_signal_power", 0.0),
        })
        content = set_section_values(content, f"{prefix}.fading", {
            "enable": bool_text(channel.get("fading_enabled")),
            "model": channel.get("fading_model", "none"),
        })
        content = set_section_values(content, f"{prefix}.delay", {
            "enable": bool_text(channel.get("delay_enabled")),
            "period_s": channel.get("delay_period_s", 1.0),
            "init_time_s": channel.get("delay_init_time_s", 0.0),
            "maximum_us": channel.get("delay_maximum_us", 0.0),
            "minimum_us": channel.get("delay_minimum_us", 0.0),
        })
        content = set_section_values(content, f"{prefix}.rlf", {
            "enable": bool_text(channel.get("rlf_enabled")),
            "t_on_ms": channel.get("rlf_t_on_ms", 1000),
            "t_off_ms": channel.get("rlf_t_off_ms", 1000),
        })
        content = set_section_values(content, f"{prefix}.hst", {
            "enable": bool_text(channel.get("hst_enabled")),
            "period_s": channel.get("hst_period_s", 1.0),
            "fd_hz": channel.get("hst_fd_hz", 0.0) * (1 if direction == "dl" else -1),
            "init_time_s": channel.get("hst_init_time_s", 0.0),
        })
    return content


def definition_config_path(experiment_id: str, slot: int) -> Path:
    if not re.fullmatch(r"[a-zA-Z0-9-]+", experiment_id) or slot not in range(1, 11):
        raise ValueError("invalid experiment config path")
    return DEFINITIONS_ROOT / experiment_id / f"ue{slot}.conf"


def definition_gnb_config_path(experiment_id: str) -> Path:
    if not re.fullmatch(r"[a-zA-Z0-9-]+", experiment_id):
        raise ValueError("invalid experiment config path")
    return DEFINITIONS_ROOT / experiment_id / "gnb.yml"


def effective_ue_config(experiment_id: str, slot: int, channel: dict) -> tuple[str, Path, bool]:
    custom = definition_config_path(experiment_id, slot)
    if custom.exists():
        return custom.read_text(), custom, True
    source = BASE_UE / f"ue{slot}.conf"
    return render_ue_config(source, channel), custom, False


def effective_gnb_config(experiment_id: str) -> tuple[str, Path, bool]:
    custom = definition_gnb_config_path(experiment_id)
    if custom.exists():
        return custom.read_text(), custom, True
    return BASE_GNB.read_text(), custom, False


def redact_sensitive(content: str) -> str:
    return SENSITIVE_LINE.sub(lambda match: f"{match.group(1)}{match.group(2)}{match.group(3)} ******** (redacted)", content)


def merge_sensitive(original: str, submitted: str) -> str:
    originals = {match.group(2).lower(): match.group(0) for match in SENSITIVE_LINE.finditer(original)}
    seen: set[str] = set()

    def replace(match: re.Match[str]) -> str:
        key = match.group(2).lower()
        seen.add(key)
        if "(redacted)" not in match.group(4):
            raise ValueError(f"sensitive field {key} cannot be edited in the web UI")
        return originals.get(key, match.group(0))

    merged = SENSITIVE_LINE.sub(replace, submitted)
    missing = set(originals) - seen
    if missing:
        raise ValueError(f"sensitive fields cannot be removed: {', '.join(sorted(missing))}")
    return merged


def validate_ue_config(content: str) -> None:
    if len(content.encode()) > 512_000:
        raise ValueError("UE config is larger than 500 KiB")
    required = {"[rf]", "[usim]", "[rrc]", "[nas]", "[gw]", "[channel.dl]", "[channel.ul]"}
    missing = sorted(section for section in required if section not in content)
    if missing:
        raise ValueError(f"missing required sections: {', '.join(missing)}")


def validate_gnb_config(content: str) -> None:
    if len(content.encode()) > 512_000:
        raise ValueError("gNB config is larger than 500 KiB")
    try:
        config = yaml.safe_load(content)
    except yaml.YAMLError as exc:
        raise ValueError(f"invalid gNB YAML: {exc}") from exc
    if not isinstance(config, dict):
        raise ValueError("gNB config root must be a YAML mapping")
    required = ("cu_cp", "cu_up", "ru_sdr", "cell_cfg", "e2")
    missing = [key for key in required if not isinstance(config.get(key), dict)]
    if missing:
        raise ValueError(f"missing required gNB mappings: {', '.join(missing)}")
    ru_sdr = config["ru_sdr"]
    if ru_sdr.get("device_driver") != "zmq":
        raise ValueError("managed topology requires ru_sdr.device_driver: zmq")
    device_args = str(ru_sdr.get("device_args", ""))
    for endpoint in ("tx_port=tcp://*:2000", "rx_port=tcp://localhost:2001"):
        if endpoint not in device_args:
            raise ValueError(f"managed topology requires {endpoint}")
    cell = config["cell_cfg"]
    if not isinstance(cell.get("channel_bandwidth_MHz"), (int, float)):
        raise ValueError("cell_cfg.channel_bandwidth_MHz must be numeric")
    if not isinstance(cell.get("common_scs"), (int, float)):
        raise ValueError("cell_cfg.common_scs must be numeric")


def write_definition_config(experiment_id: str, slot: int, content: str) -> Path:
    target = definition_config_path(experiment_id, slot)
    target.parent.mkdir(parents=True, mode=0o700, exist_ok=True)
    temporary = target.with_suffix(".tmp")
    temporary.write_text(content)
    os.chmod(temporary, 0o600)
    temporary.replace(target)
    return target


def write_gnb_definition_config(experiment_id: str, content: str) -> Path:
    target = definition_gnb_config_path(experiment_id)
    target.parent.mkdir(parents=True, mode=0o700, exist_ok=True)
    temporary = target.with_suffix(".tmp")
    temporary.write_text(content)
    os.chmod(temporary, 0o600)
    temporary.replace(target)
    return target


def render_broker(path_losses: dict[int, float]) -> str:
    content = BASE_BROKER.read_text()
    slots = sorted(path_losses)
    if not slots or len(slots) > 10:
        raise ValueError("broker requires 1 to 10 enabled UEs")
    replacements = {
        r"^ACTIVE_UE_SLOTS\s*=.*$": f"ACTIVE_UE_SLOTS = {slots!r}",
        r"^CONFIGURED_PATH_LOSSES\s*=.*$": (
            f"CONFIGURED_PATH_LOSSES = "
            f"{dict((slot, float(path_losses[slot])) for slot in slots)!r}"
        ),
    }
    for pattern, replacement in replacements.items():
        content, count = re.subn(pattern, replacement, content, count=1, flags=re.MULTILINE)
        if count != 1:
            raise ValueError(f"broker template hook not found: {pattern}")
    return content


def generate_run_configs(experiment: Experiment, snapshot: Path) -> dict[str, object]:
    config_dir = snapshot / "configs"
    config_dir.mkdir(mode=0o700)
    gnb_target = config_dir / "gnb.yml"
    gnb_content, _, _ = effective_gnb_config(experiment.id)
    validate_gnb_config(gnb_content)
    gnb_target.write_text(gnb_content)
    os.chmod(gnb_target, 0o600)

    ue_paths: list[str] = []
    ue_slots: list[int] = []
    path_losses: dict[int, float] = {}
    for ue in sorted((item for item in experiment.ues if item.enabled), key=lambda item: item.slot):
        target = config_dir / f"ue{ue.slot}.conf"
        content, _, custom = effective_ue_config(experiment.id, ue.slot, ue.channel or {})
        target.write_text(content)
        os.chmod(target, 0o600)
        ue_paths.append(str(target))
        ue_slots.append(ue.slot)
        path_losses[ue.slot] = ue.path_loss_db

    broker_target = config_dir / "broker.py"
    broker_target.write_text(render_broker(path_losses))
    os.chmod(broker_target, 0o700)
    subprocess.run(["/usr/bin/python3", "-m", "py_compile", str(broker_target)], check=True)

    runtime = {
        "run_id": snapshot.name,
        "gnb_config": str(gnb_target),
        "ue_configs": ue_paths,
        "ue_slots": ue_slots,
        "broker": str(broker_target),
    }
    temporary = ACTIVE_CONFIG.with_suffix(".tmp")
    temporary.write_text(json.dumps(runtime, indent=2) + "\n")
    os.chmod(temporary, 0o600)
    temporary.replace(ACTIVE_CONFIG)
    return runtime
