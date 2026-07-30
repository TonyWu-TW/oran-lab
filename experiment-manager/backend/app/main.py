from __future__ import annotations

import json
import os
import re
import shutil
import signal
import subprocess
import sys
import uuid
from contextlib import asynccontextmanager
from datetime import datetime, timezone
from pathlib import Path
from threading import Lock, Thread
from typing import Any

import httpx
from fastapi import Depends, FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from sqlalchemy import inspect, select
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session, selectinload

from . import models, schemas
from .controller import ControlError, LAB_ROOT, invoke
from .config_generator import (
    effective_gnb_config,
    effective_ue_config,
    generate_run_configs,
    merge_sensitive,
    redact_sensitive,
    render_ue_config,
    validate_gnb_config,
    validate_ue_config,
    write_definition_config,
    write_gnb_definition_config,
)
from .database import Base, SessionLocal, engine, get_db


def seed_baseline() -> None:
    if os.environ.get("ORAN_MANAGER_SEED", "1") == "0":
        return
    with SessionLocal() as database:
        channel_defaults = schemas.ChannelProfile().model_dump()
        changed = False
        for ue in database.scalars(select(models.UEProfile)).all():
            normalized = {**channel_defaults, **(ue.channel or {})}
            if normalized != ue.channel:
                ue.channel = normalized
                changed = True
            traffic = validated_traffic_defaults(ue.traffic_defaults)
            if traffic != ue.traffic_defaults:
                ue.traffic_defaults = traffic
                changed = True
        if changed:
            database.commit()
        if database.scalar(select(models.Experiment.id).limit(1)):
            return
        experiment = models.Experiment(
            name="3 UE Clean Baseline",
            description="已通過 N2/E2、三台 UE attach、UPF 與外網連線驗收的基線。",
            expected_ue_count=3,
            broker_capacity=3,
            scenario="clean",
        )
        identities = (
            (1, "999700000000001", "353490069873319"),
            (2, "999700000000002", "353490069873327"),
            (3, "999700000000003", "353490069873335"),
        )
        for slot, imsi, imei in identities:
            experiment.ues.append(models.UEProfile(
                slot=slot,
                display_name=f"UE {slot}",
                enabled=True,
                imsi=imsi,
                imei=imei,
                rx_port=2000 + slot * 100,
                tx_port=2001 + slot * 100,
                namespace=f"ue{slot}",
                path_loss_db=float((slot - 1) * 10),
                channel=schemas.ChannelProfile().model_dump(),
                traffic_defaults=default_traffic_defaults(),
            ))
        database.add(experiment)
        database.commit()


@asynccontextmanager
async def lifespan(_: FastAPI):
    Base.metadata.create_all(engine)
    migrate_database()
    RUN_ROOT.mkdir(parents=True, exist_ok=True)
    VOICEGUARD_ROOT.mkdir(parents=True, exist_ok=True)
    seed_baseline()
    # Worker threads and their subprocess handles live in this Manager process.
    # Any active DB rows found during a fresh startup therefore belong to a
    # previous Manager instance and must not remain falsely RUNNING forever.
    with SessionLocal() as database:
        interrupted = database.scalars(select(models.TrafficJob).where(
            models.TrafficJob.status.in_(["QUEUED", "RUNNING", "STOP_REQUESTED"])
        )).all()
        for job in interrupted:
            job.status = "INTERRUPTED"
            job.finished_at = datetime.now(timezone.utc)
            job.result = {
                **(job.result or {}),
                "ok": False,
                "error_code": "MANAGER_RESTARTED",
                "error": "Traffic worker ownership was lost when Experiment Manager restarted",
            }
        if interrupted:
            database.commit()
    try:
        yield
    finally:
        with traffic_lock:
            active_traffic = list(traffic_processes.values())
        for process in active_traffic:
            if process.poll() is None:
                process.send_signal(signal.SIGINT)
        with voiceguard_lock:
            active_xapps = list(voiceguard_processes.values())
        for process in active_xapps:
            if process.poll() is None:
                process.send_signal(signal.SIGTERM)


app = FastAPI(title="O-RAN Experiment Manager", version="0.1.0", lifespan=lifespan)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

PROMETHEUS = "http://127.0.0.1:9095"
RUN_ROOT = LAB_ROOT / "experiments" / "runs"
ALLOWED_METRICS = {
    "ue_rx_bps": "oran_ue_rx_bps",
    "ue_tx_bps": "oran_ue_tx_bps",
    "ue_ping_latency": "oran_ue_ping_latency_ms",
    "ue_ping_loss": "oran_ue_ping_loss_percent",
    "ue_attached": "oran_ue_attached",
    "ue_pdu_up": "oran_ue_pdu_session_up",
}
TRAFFIC_HELPER = LAB_ROOT / "scripts" / "oranlab-traffic.py"
VOICEGUARD_SCRIPT = LAB_ROOT / "xapps" / "voiceguard" / "voiceguard.py"
VOICEGUARD_RC_BRIDGE = (
    LAB_ROOT / "src" / "flexric" / "build" / "examples" / "xApp" / "c"
    / "voiceguard_rc" / "voiceguard_rc"
)
VOICEGUARD_ROOT = Path(__file__).resolve().parents[1] / "data" / "voiceguard"
MANAGER_SELF_URL = os.environ.get("ORAN_MANAGER_SELF_URL", "http://127.0.0.1:8088")
traffic_processes: dict[str, subprocess.Popen[str]] = {}
traffic_lock = Lock()
voiceguard_processes: dict[str, subprocess.Popen[str]] = {}
voiceguard_lock = Lock()


TRAFFIC_PROFILES: dict[str, dict[str, Any]] = {
    "none": {"application_protocol": "none", "transport": "none", "direction": "UL"},
    "ping": {"application_protocol": "ping", "transport": "icmp", "direction": "UL"},
    "iperf": {"application_protocol": "iperf3", "transport": "udp", "direction": "UL"},
    "http": {"application_protocol": "http", "transport": "tcp", "direction": "DL"},
    "short_video": {"application_protocol": "http", "transport": "tcp", "direction": "DL"},
    "social": {"application_protocol": "http", "transport": "tcp", "direction": "DL"},
    "navigation": {"application_protocol": "http", "transport": "tcp", "direction": "BOTH"},
    "rtp_voice": {"application_protocol": "rtp-like", "transport": "udp", "direction": "BOTH"},
}


def default_traffic_defaults() -> dict[str, Any]:
    return {
        "version": 2,
        "flows": [{
            "type": "iperf",
            "application_protocol": "iperf3",
            "transport": "udp",
            "direction": "UL",
            "run_mode": "duration",
            "duration_seconds": 10,
            "params": {"bitrate": "750K"},
        }],
    }


def normalize_traffic_defaults(raw: dict[str, Any] | None) -> dict[str, Any]:
    raw = raw or {}
    if raw.get("version") == 2 and isinstance(raw.get("flows"), list):
        return raw
    protocol = str(raw.get("protocol", "udp"))
    traffic_type = "ping" if protocol == "ping" else "iperf"
    transport = "icmp" if protocol == "ping" else protocol if protocol in {"tcp", "udp"} else "udp"
    duration = raw.get("duration", 10)
    return {
        "version": 2,
        "flows": [{
            "type": traffic_type,
            "application_protocol": "ping" if traffic_type == "ping" else "iperf3",
            "transport": transport,
            "direction": str(raw.get("direction", "UL")),
            "run_mode": "duration",
            "duration_seconds": int(duration),
            "params": {"bitrate": str(raw.get("bitrate", "750K"))},
        }],
    }


def normalize_traffic_flow(raw: dict[str, Any]) -> dict[str, Any]:
    traffic_type = str(raw.get("type", "iperf"))
    if traffic_type not in TRAFFIC_PROFILES:
        raise ValueError(f"unsupported traffic profile: {traffic_type}")
    profile = TRAFFIC_PROFILES[traffic_type]
    transport = str(raw.get("transport", profile["transport"])).lower()
    if traffic_type == "iperf" and transport not in {"tcp", "udp"}:
        raise ValueError("iperf transport must be tcp or udp")
    if traffic_type != "iperf":
        transport = profile["transport"]
    direction = str(raw.get("direction", profile["direction"])).upper()
    allowed_directions = {"UL", "DL", "BOTH"}
    if direction not in allowed_directions:
        raise ValueError("direction must be UL, DL, or BOTH")
    if traffic_type in {"iperf", "http"} and direction == "BOTH":
        raise ValueError(f"{traffic_type} direction must be UL or DL")
    if traffic_type in {"ping"}:
        direction = "UL"
    if traffic_type in {"short_video", "social"}:
        direction = "DL"
    if traffic_type in {"navigation", "rtp_voice"}:
        direction = "BOTH"
    run_mode = str(raw.get("run_mode", "duration"))
    if run_mode not in {"duration", "continuous"}:
        raise ValueError("run_mode must be duration or continuous")
    duration = raw.get("duration_seconds")
    if run_mode == "duration":
        try:
            duration = int(duration if duration is not None else 60)
        except (TypeError, ValueError) as exc:
            raise ValueError("duration_seconds must be an integer") from exc
        if not 1 <= duration <= 86400:
            raise ValueError("duration_seconds must be 1..86400")
    else:
        duration = None
    params = dict(raw.get("params") or {})
    defaults: dict[str, Any] = {
        "ping": {"interval_ms": 1000, "packet_size": 56},
        "iperf": {"bitrate": "750K"},
        "http": {"object_size_kb": 256, "interval_ms": 1000},
        "short_video": {
            "offered_load_mbps": 0.8,
            "traffic_pattern": "wave",
            "variation_percent": 30,
            "peak_limit_mbps": 1.2,
            "random_seed": 1234,
            "pattern_period_seconds": 20,
            "segment_interval_ms": 1000,
        },
        "social": {"object_size_kb": 180, "objects_per_cycle": 4, "cycle_interval_ms": 4000},
        "navigation": {"tile_size_kb": 40, "tiles_per_cycle": 6, "update_interval_ms": 5000},
        "rtp_voice": {"packet_interval_ms": 20, "bitrate_kbps": 64},
        "none": {},
    }
    params = {**defaults[traffic_type], **params}
    if traffic_type == "iperf":
        bitrate = str(params.get("bitrate", "750K"))
        if not re.fullmatch(r"[1-9][0-9]*(?:\.[0-9]+)?[KMG]?", bitrate):
            raise ValueError("invalid bitrate")
        params["bitrate"] = bitrate
    numeric_limits = {
        "interval_ms": (20, 600000),
        "packet_size": (8, 1400),
        "object_size_kb": (1, 10240),
        "segment_size_kb": (1, 10240),
        "segment_interval_ms": (100, 600000),
        "objects_per_cycle": (1, 100),
        "cycle_interval_ms": (100, 600000),
        "tile_size_kb": (1, 10240),
        "tiles_per_cycle": (1, 100),
        "update_interval_ms": (100, 600000),
        "packet_interval_ms": (5, 1000),
        "bitrate_kbps": (8, 10000),
        "variation_percent": (0, 100),
        "random_seed": (0, 2147483647),
        "pattern_period_seconds": (2, 3600),
    }
    for key, (minimum, maximum) in numeric_limits.items():
        if key not in params:
            continue
        try:
            value = int(params[key])
        except (TypeError, ValueError) as exc:
            raise ValueError(f"{key} must be an integer") from exc
        if not minimum <= value <= maximum:
            raise ValueError(f"{key} must be {minimum}..{maximum}")
        params[key] = value
    if traffic_type == "short_video":
        pattern = str(params.get("traffic_pattern", "wave"))
        if pattern not in {"fixed", "wave", "random_burst", "adaptive"}:
            raise ValueError("traffic_pattern must be fixed, wave, random_burst, or adaptive")
        params["traffic_pattern"] = pattern
        for key, minimum, maximum in (
            ("offered_load_mbps", 0.01, 100.0),
            ("peak_limit_mbps", 0.01, 100.0),
        ):
            try:
                value = float(params[key])
            except (KeyError, TypeError, ValueError) as exc:
                raise ValueError(f"{key} must be a number") from exc
            if not minimum <= value <= maximum:
                raise ValueError(f"{key} must be {minimum}..{maximum}")
            params[key] = value
        if params["peak_limit_mbps"] < params["offered_load_mbps"]:
            raise ValueError("peak_limit_mbps cannot be lower than offered_load_mbps")
    return {
        "type": traffic_type,
        "application_protocol": profile["application_protocol"],
        "transport": transport,
        "direction": direction,
        "run_mode": run_mode,
        "duration_seconds": duration,
        "params": params,
    }


def validated_traffic_defaults(raw: dict[str, Any] | None) -> dict[str, Any]:
    normalized = normalize_traffic_defaults(raw)
    flows = [normalize_traffic_flow(flow) for flow in normalized.get("flows", [])]
    if not flows:
        flows = [normalize_traffic_flow({"type": "none"})]
    if len(flows) > 5:
        raise ValueError("each UE supports at most 5 traffic flows")
    return {"version": 2, "flows": flows}


def migrate_database() -> None:
    """Add execution metadata without invalidating existing SQLite run history."""
    columns = {column["name"] for column in inspect(engine).get_columns("traffic_jobs")}
    additions = {
        "batch_id": "VARCHAR",
        "traffic_type": "VARCHAR(32) DEFAULT 'iperf'",
        "application_protocol": "VARCHAR(24) DEFAULT 'iperf3'",
        "transport": "VARCHAR(12) DEFAULT 'udp'",
        "duration_seconds": "INTEGER",
        "run_mode": "VARCHAR(16) DEFAULT 'duration'",
        "parameters": "JSON DEFAULT '{}'",
    }
    with engine.begin() as connection:
        for name, definition in additions.items():
            if name not in columns:
                connection.exec_driver_sql(f"ALTER TABLE traffic_jobs ADD COLUMN {name} {definition}")
        connection.exec_driver_sql(
            "UPDATE traffic_jobs SET duration_seconds = duration WHERE duration_seconds IS NULL AND duration > 0"
        )
        connection.exec_driver_sql(
            "UPDATE traffic_jobs SET transport = protocol WHERE batch_id IS NULL AND protocol IN ('tcp', 'udp')"
        )
        connection.exec_driver_sql(
            "UPDATE traffic_jobs SET transport = 'icmp', traffic_type = 'ping', application_protocol = 'ping' "
            "WHERE batch_id IS NULL AND protocol = 'ping'"
        )


def experiment_or_404(database: Session, experiment_id: str) -> models.Experiment:
    experiment = database.scalar(
        select(models.Experiment)
        .where(models.Experiment.id == experiment_id)
        .options(selectinload(models.Experiment.ues))
    )
    if not experiment:
        raise HTTPException(404, "experiment not found")
    return experiment


def add_event(database: Session, run_id: str, event_type: str, message: str, **extra: Any) -> None:
    database.add(models.RuntimeEvent(
        run_id=run_id,
        event_type=event_type,
        message=message,
        component=extra.pop("component", "manager"),
        severity=extra.pop("severity", "info"),
        details=extra,
    ))


def enabled_ues(experiment: models.Experiment) -> list[models.UEProfile]:
    return [ue for ue in experiment.ues if ue.enabled]


def validate_experiment(experiment: models.Experiment) -> dict[str, Any]:
    checks: list[dict[str, str]] = []

    def check(check_id: str, passed: bool, message: str) -> None:
        checks.append({"id": check_id, "status": "pass" if passed else "fail", "message": message})

    ues = enabled_ues(experiment)
    check("ue-count", len(ues) == experiment.expected_ue_count, f"enabled={len(ues)}, expected={experiment.expected_ue_count}")
    check("mvp-topology", len(ues) == 3 and experiment.broker_capacity == 3, "目前已驗收的 control layer 固定為 3 UE")
    check("unique-slot", len({ue.slot for ue in ues}) == len(ues), "UE slot 必須唯一")
    check("unique-imsi", len({ue.imsi for ue in ues}) == len(ues), "IMSI 必須唯一")
    ports = [port for ue in ues for port in (ue.rx_port, ue.tx_port)]
    check("unique-port", len(set(ports)) == len(ports), "每個 UE ZMQ port 必須唯一")
    check("capacity", len(ues) <= experiment.broker_capacity, "enabled UE 不可超過 Broker capacity")
    return {"ok": all(item["status"] == "pass" for item in checks), "checks": checks}


@app.get("/api/health")
def health() -> dict[str, str]:
    return {"status": "ok", "version": app.version}


@app.get("/api/platform/status")
def platform_status() -> dict[str, Any]:
    try:
        return invoke("status")
    except ControlError as exc:
        raise HTTPException(503, str(exc)) from exc


@app.get("/api/platform/preflight")
def platform_preflight() -> dict[str, Any]:
    try:
        return invoke("preflight")
    except ControlError as exc:
        raise HTTPException(503, str(exc)) from exc


@app.get("/api/experiments", response_model=list[schemas.ExperimentOut])
def list_experiments(database: Session = Depends(get_db)):
    return database.scalars(select(models.Experiment).options(selectinload(models.Experiment.ues))).all()


@app.post("/api/experiments", response_model=schemas.ExperimentOut, status_code=201)
def create_experiment(payload: schemas.ExperimentCreate, database: Session = Depends(get_db)):
    experiment = models.Experiment(**payload.model_dump())
    database.add(experiment)
    try:
        database.commit()
    except IntegrityError as exc:
        database.rollback()
        raise HTTPException(409, "experiment name already exists") from exc
    database.refresh(experiment)
    return experiment


@app.get("/api/experiments/{experiment_id}", response_model=schemas.ExperimentOut)
def get_experiment(experiment_id: str, database: Session = Depends(get_db)):
    return experiment_or_404(database, experiment_id)


@app.patch("/api/experiments/{experiment_id}", response_model=schemas.ExperimentOut)
def update_experiment(experiment_id: str, payload: schemas.ExperimentUpdate, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    for key, value in payload.model_dump(exclude_unset=True).items():
        setattr(experiment, key, value)
    experiment.revision += 1
    database.commit()
    return experiment_or_404(database, experiment_id)


@app.delete("/api/experiments/{experiment_id}", status_code=204)
def delete_experiment(experiment_id: str, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    active = database.scalar(select(models.ExperimentRun).where(
        models.ExperimentRun.experiment_id == experiment_id,
        models.ExperimentRun.state.in_(["STARTING", "RUNNING", "STOPPING"]),
    ))
    if active:
        raise HTTPException(409, "active experiment cannot be deleted")
    database.delete(experiment)
    database.commit()


@app.post("/api/experiments/{experiment_id}/clone", response_model=schemas.ExperimentOut, status_code=201)
def clone_experiment(experiment_id: str, database: Session = Depends(get_db)):
    source = experiment_or_404(database, experiment_id)
    clone = models.Experiment(
        name=f"{source.name} copy {datetime.now().strftime('%H%M%S')}",
        description=source.description,
        expected_ue_count=source.expected_ue_count,
        broker_capacity=source.broker_capacity,
        monitoring_enabled=source.monitoring_enabled,
        scenario=source.scenario,
    )
    for ue in source.ues:
        values = {column.name: getattr(ue, column.name) for column in models.UEProfile.__table__.columns if column.name not in {"id", "experiment_id"}}
        clone.ues.append(models.UEProfile(**values))
    database.add(clone)
    database.commit()
    return experiment_or_404(database, clone.id)


@app.post("/api/experiments/{experiment_id}/validate")
def validate(experiment_id: str, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    result = validate_experiment(experiment)
    try:
        result["platform"] = invoke("preflight")
    except ControlError as exc:
        result["platform"] = {"ok": False, "error": str(exc)}
    result["ok"] = result["ok"] and result["platform"].get("ok", False)
    return result


@app.get("/api/experiments/{experiment_id}/ues", response_model=list[schemas.UEOut])
def list_ues(experiment_id: str, database: Session = Depends(get_db)):
    return experiment_or_404(database, experiment_id).ues


@app.get("/api/experiments/{experiment_id}/gnb/config")
def get_editable_gnb_config(experiment_id: str, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    content, target, custom = effective_gnb_config(experiment.id)
    return {
        "experiment_id": experiment.id,
        "component": "gnb",
        "path": str(target),
        "custom": custom,
        "redacted": False,
        "content": content,
        "applies": "next_run",
    }


@app.put("/api/experiments/{experiment_id}/gnb/config")
def save_editable_gnb_config(
    experiment_id: str,
    payload: schemas.ConfigUpdate,
    database: Session = Depends(get_db),
):
    experiment = experiment_or_404(database, experiment_id)
    try:
        validate_gnb_config(payload.content)
        target = write_gnb_definition_config(experiment.id, payload.content)
    except (OSError, ValueError) as exc:
        raise HTTPException(422, str(exc)) from exc
    experiment.revision += 1
    database.commit()
    return {
        "experiment_id": experiment.id,
        "component": "gnb",
        "path": str(target),
        "custom": True,
        "redacted": False,
        "content": payload.content,
        "applies": "next_run",
        "revision": experiment.revision,
    }


@app.post("/api/experiments/{experiment_id}/ues", response_model=schemas.UEOut, status_code=201)
def create_ue(experiment_id: str, payload: schemas.UECreate, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    if any(ue.slot == payload.slot or ue.imsi == payload.imsi for ue in experiment.ues):
        raise HTTPException(409, "UE slot or IMSI already exists in experiment")
    ue = models.UEProfile(experiment_id=experiment_id, **payload.model_dump(mode="json"))
    database.add(ue)
    experiment.revision += 1
    database.commit()
    database.refresh(ue)
    return ue


@app.patch("/api/experiments/{experiment_id}/ues/{ue_id}", response_model=schemas.UEOut)
def update_ue(experiment_id: str, ue_id: str, payload: schemas.UEUpdate, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    ue = next((item for item in experiment.ues if item.id == ue_id), None)
    if not ue:
        raise HTTPException(404, "UE not found")
    values = payload.model_dump(exclude_unset=True, mode="json")
    if payload.channel is not None:
        values["channel"] = payload.channel.model_dump(mode="json")
    if payload.traffic_defaults is not None:
        try:
            values["traffic_defaults"] = validated_traffic_defaults(payload.traffic_defaults)
        except ValueError as exc:
            raise HTTPException(422, str(exc)) from exc
    for key, value in values.items():
        setattr(ue, key, value)
    if payload.channel is not None:
        _, custom_path, is_custom = effective_ue_config(experiment.id, ue.slot, ue.channel or {})
        if is_custom:
            updated_custom = render_ue_config(custom_path, ue.channel or {})
            validate_ue_config(updated_custom)
            write_definition_config(experiment.id, ue.slot, updated_custom)
    experiment.revision += 1
    database.commit()
    database.refresh(ue)
    return ue


@app.get("/api/experiments/{experiment_id}/ues/{ue_id}/config")
def get_editable_ue_config(experiment_id: str, ue_id: str, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    ue = next((item for item in experiment.ues if item.id == ue_id), None)
    if not ue:
        raise HTTPException(404, "UE not found")
    content, target, custom = effective_ue_config(experiment.id, ue.slot, ue.channel or {})
    return {
        "experiment_id": experiment.id,
        "ue_id": ue.id,
        "ue": f"ue{ue.slot}",
        "path": str(target),
        "custom": custom,
        "redacted": True,
        "content": redact_sensitive(content),
        "applies": "next_run",
    }


@app.put("/api/experiments/{experiment_id}/ues/{ue_id}/config")
def save_editable_ue_config(
    experiment_id: str,
    ue_id: str,
    payload: schemas.ConfigUpdate,
    database: Session = Depends(get_db),
):
    experiment = experiment_or_404(database, experiment_id)
    ue = next((item for item in experiment.ues if item.id == ue_id), None)
    if not ue:
        raise HTTPException(404, "UE not found")
    original, _, _ = effective_ue_config(experiment.id, ue.slot, ue.channel or {})
    try:
        merged = merge_sensitive(original, payload.content)
        validate_ue_config(merged)
        target = write_definition_config(experiment.id, ue.slot, merged)
    except (OSError, ValueError) as exc:
        raise HTTPException(422, str(exc)) from exc
    experiment.revision += 1
    database.commit()
    return {
        "experiment_id": experiment.id,
        "ue_id": ue.id,
        "ue": f"ue{ue.slot}",
        "path": str(target),
        "custom": True,
        "redacted": True,
        "content": redact_sensitive(merged),
        "applies": "next_run",
        "revision": experiment.revision,
    }


@app.delete("/api/experiments/{experiment_id}/ues/{ue_id}", status_code=204)
def delete_ue(experiment_id: str, ue_id: str, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    ue = next((item for item in experiment.ues if item.id == ue_id), None)
    if not ue:
        raise HTTPException(404, "UE not found")
    database.delete(ue)
    experiment.revision += 1
    database.commit()


def run_start_worker(run_id: str) -> None:
    with SessionLocal() as database:
        run = database.get(models.ExperimentRun, run_id)
        if not run:
            return
        try:
            add_event(database, run_id, "start_requested", "正在啟動 O-RAN 實驗")
            database.commit()
            status_payload = invoke("start", privileged=True)
            run.state = "RUNNING"
            run.started_at = datetime.now(timezone.utc)
            run.result_summary = {"platform": status_payload}
            add_event(database, run_id, "started", "RIC、gNB、Broker 與三台 UE 已就緒")
        except Exception as exc:
            run.state = "START_FAILED"
            add_event(database, run_id, "start_failed", str(exc), severity="error")
        database.commit()


@app.post("/api/experiments/{experiment_id}/runs", response_model=schemas.RunOut, status_code=202)
def start_run(experiment_id: str, database: Session = Depends(get_db)):
    experiment = experiment_or_404(database, experiment_id)
    validation = validate_experiment(experiment)
    if not validation["ok"]:
        raise HTTPException(422, validation)
    active = database.scalar(select(models.ExperimentRun).where(
        models.ExperimentRun.state.in_(["STARTING", "RUNNING", "STOPPING"])
    ))
    if active:
        raise HTTPException(409, f"run {active.id} is already {active.state}")
    run = models.ExperimentRun(
        experiment_id=experiment.id,
        experiment_revision=experiment.revision,
        state="STARTING",
        operation="start",
    )
    database.add(run)
    database.flush()
    snapshot = RUN_ROOT / run.id
    try:
        snapshot.mkdir(parents=True, mode=0o700, exist_ok=False)
        runtime_config = generate_run_configs(experiment, snapshot)
        manifest = {
            "run_id": run.id,
            "experiment": schemas.ExperimentOut.model_validate(experiment).model_dump(mode="json"),
            "runtime_config": runtime_config,
        }
        (snapshot / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
        run.snapshot_path = str(snapshot)
    except OSError as exc:
        database.rollback()
        shutil.rmtree(snapshot, ignore_errors=True)
        raise HTTPException(500, f"cannot create run snapshot: {exc}") from exc
    database.commit()
    Thread(target=run_start_worker, args=(run.id,), daemon=True).start()
    return run


@app.get("/api/runs", response_model=list[schemas.RunOut])
def list_runs(database: Session = Depends(get_db)):
    return database.scalars(select(models.ExperimentRun).order_by(models.ExperimentRun.started_at.desc())).all()


@app.get("/api/runs/{run_id}", response_model=schemas.RunOut)
def get_run(run_id: str, database: Session = Depends(get_db)):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    return run


def run_stop_worker(run_id: str) -> None:
    with SessionLocal() as database:
        run = database.get(models.ExperimentRun, run_id)
        if not run:
            return
        try:
            add_event(database, run_id, "stop_requested", "正在停止 O-RAN 實驗")
            active_jobs = database.scalars(select(models.TrafficJob).where(
                models.TrafficJob.run_id == run_id,
                models.TrafficJob.status.in_(["QUEUED", "RUNNING"]),
            )).all()
            for job in active_jobs:
                job.status = "STOP_REQUESTED"
            database.commit()
            with traffic_lock:
                active_traffic = list(traffic_processes.values())
            for process in active_traffic:
                process.send_signal(signal.SIGINT)
            voiceguard = voiceguard_processes.get(run_id)
            if voiceguard and voiceguard.poll() is None:
                voiceguard.send_signal(signal.SIGTERM)
            payload = invoke("stop", privileged=True)
            run.state = "STOPPED"
            run.stopped_at = datetime.now(timezone.utc)
            run.result_summary = {**run.result_summary, "stop": payload}
            add_event(database, run_id, "stopped", "實驗已停止，受管程序與 ZMQ ports 已清理")
        except Exception as exc:
            run.state = "STOP_FAILED"
            add_event(database, run_id, "stop_failed", str(exc), severity="error")
        database.commit()


@app.post("/api/runs/{run_id}/stop", response_model=schemas.RunOut, status_code=202)
def stop_run(run_id: str, database: Session = Depends(get_db)):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    if run.state not in {"RUNNING", "START_FAILED", "DEGRADED"}:
        raise HTTPException(409, f"run cannot stop from {run.state}")
    run.state = "STOPPING"
    run.operation = "stop"
    database.commit()
    Thread(target=run_stop_worker, args=(run.id,), daemon=True).start()
    return run


@app.get("/api/runs/{run_id}/events", response_model=list[schemas.EventOut])
def run_events(run_id: str, database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    return database.scalars(select(models.RuntimeEvent).where(models.RuntimeEvent.run_id == run_id).order_by(models.RuntimeEvent.timestamp)).all()


ACTIVE_TRAFFIC_STATUSES = {"QUEUED", "RUNNING", "STOP_REQUESTED"}


def run_traffic_configuration(run: models.ExperimentRun) -> list[dict[str, Any]]:
    if not run.snapshot_path:
        return []
    manifest_path = Path(run.snapshot_path) / "manifest.json"
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, json.JSONDecodeError):
        return []
    ues = manifest.get("experiment", {}).get("ues", [])
    configured: list[dict[str, Any]] = []
    for ue in ues:
        if not ue.get("enabled", True):
            continue
        name = str(ue.get("namespace") or f"ue{ue.get('slot')}")
        try:
            traffic = validated_traffic_defaults(ue.get("traffic_defaults"))
        except ValueError:
            traffic = default_traffic_defaults()
        configured.append({
            "ue": name,
            "display_name": ue.get("display_name", name.upper()),
            "enabled": bool(ue.get("enabled", True)),
            "traffic": traffic,
        })
    return configured


def next_traffic_port(database: Session, run_id: str, preferred: int) -> int:
    used = set(database.scalars(select(models.TrafficJob.port).where(
        models.TrafficJob.run_id == run_id,
        models.TrafficJob.status.in_(ACTIVE_TRAFFIC_STATUSES),
    )).all())
    for port in [preferred, *range(5201, 5300)]:
        if port not in used:
            return port
    raise HTTPException(409, "no traffic ports are available")


def create_traffic_job(
    database: Session,
    run: models.ExperimentRun,
    ue: str,
    flow: dict[str, Any],
    batch_id: str,
    flow_index: int = 0,
) -> tuple[models.TrafficJob, dict[str, Any]]:
    slot_match = re.fullmatch(r"ue([1-9][0-9]*)", ue)
    if not slot_match:
        raise HTTPException(422, f"invalid UE namespace: {ue}")
    slot = int(slot_match.group(1))
    port = next_traffic_port(database, run.id, 5200 + slot + flow_index * 10)
    traffic_type = flow["type"]
    transport = flow["transport"]
    protocol = "ping" if traffic_type == "ping" else transport
    duration_seconds = flow["duration_seconds"]
    params = flow["params"]
    job = models.TrafficJob(
        run_id=run.id,
        ue=ue,
        batch_id=batch_id,
        traffic_type=traffic_type,
        application_protocol=flow["application_protocol"],
        transport=transport,
        protocol=protocol,
        direction=flow["direction"],
        target="10.45.0.1",
        port=port,
        duration=duration_seconds or 0,
        duration_seconds=duration_seconds,
        run_mode=flow["run_mode"],
        bitrate=str(params.get("bitrate", f"{params.get('bitrate_kbps', 0)}K")),
        parameters=params,
    )
    database.add(job)
    database.flush()
    worker_payload = {
        "ue": ue,
        "traffic_type": traffic_type,
        "application_protocol": flow["application_protocol"],
        "transport": transport,
        "protocol": protocol,
        "direction": flow["direction"],
        "target": job.target,
        "port": port,
        "run_mode": flow["run_mode"],
        "duration_seconds": duration_seconds,
        "params": params,
        "control_file": str(VOICEGUARD_ROOT / f"{run.id}.traffic-control.json"),
    }
    return job, worker_payload


def traffic_worker(job_id: str, payload: dict[str, Any]) -> None:
    with SessionLocal() as database:
        job = database.get(models.TrafficJob, job_id)
        if not job:
            return
        if job.status == "STOP_REQUESTED":
            job.status = "STOPPED"
            job.finished_at = datetime.now(timezone.utc)
            job.result = {"ok": True, "stopped_by_user": True, "started": False}
            database.commit()
            return
        job.status = "RUNNING"
        job.started_at = datetime.now(timezone.utc)
        add_event(database, job.run_id, "traffic_started", f"{job.ue} {job.traffic_type} {job.direction} 已啟動", component=job.ue, job_id=job.id)
        database.commit()
    process = subprocess.Popen(
        ["sudo", "-n", str(TRAFFIC_HELPER), "run", "--json"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, start_new_session=True,
    )
    with traffic_lock:
        traffic_processes[job_id] = process
    assert process.stdin is not None
    assert process.stdout is not None
    assert process.stderr is not None
    process.stdin.write(json.dumps(payload))
    process.stdin.close()
    final_result: dict[str, Any] | None = None
    output_tail: list[str] = []
    for line in process.stdout:
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
            with SessionLocal() as progress_database:
                progress_job = progress_database.get(models.TrafficJob, job_id)
                if progress_job and progress_job.status in {"RUNNING", "STOP_REQUESTED"}:
                    progress_job.result = {
                        **(progress_job.result or {}),
                        "progress": {key: value for key, value in message.items() if key != "event"},
                    }
                    progress_database.commit()
        else:
            final_result = message
    process.wait()
    stderr = process.stderr.read()
    with traffic_lock:
        traffic_processes.pop(job_id, None)
    result = final_result or {
        "ok": False,
        "error": (stderr or "\n".join(output_tail) or "traffic helper failed")[-1000:],
    }
    with SessionLocal() as database:
        job = database.get(models.TrafficJob, job_id)
        if not job:
            return
        stopped = job.status == "STOP_REQUESTED" or process.returncode in {-15, 143}
        if stopped:
            result = {**result, "ok": True, "stopped_by_user": True}
        job.status = "STOPPED" if stopped else ("COMPLETED" if result.get("ok") else "FAILED")
        job.finished_at = datetime.now(timezone.utc)
        job.result = result
        add_event(database, job.run_id, "traffic_finished", f"{job.ue} traffic {job.status.lower()}",
                  component=job.ue, severity="info" if job.status in {"COMPLETED", "STOPPED"} else "warning",
                  job_id=job.id, result=result)
        database.commit()


@app.get("/api/runs/{run_id}/traffic", response_model=list[schemas.TrafficOut])
def list_traffic(run_id: str, database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    return database.scalars(select(models.TrafficJob).where(
        models.TrafficJob.run_id == run_id
    ).order_by(models.TrafficJob.created_at.desc())).all()


@app.get("/api/runs/{run_id}/traffic/config")
def get_traffic_configuration(run_id: str, database: Session = Depends(get_db)):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    return {"run_id": run_id, "ues": run_traffic_configuration(run)}


@app.post("/api/runs/{run_id}/traffic/batch", response_model=list[schemas.TrafficOut], status_code=202)
def start_traffic_batch(run_id: str, payload: schemas.TrafficBatchCreate, database: Session = Depends(get_db)):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    if run.state != "RUNNING":
        raise HTTPException(409, "traffic requires a RUNNING experiment")
    requested = list(dict.fromkeys(payload.ues))
    if any(not re.fullmatch(r"ue[1-9][0-9]*", ue) for ue in requested):
        raise HTTPException(422, "invalid UE namespace")
    configurations = {item["ue"]: item for item in run_traffic_configuration(run)}
    missing = [ue for ue in requested if ue not in configurations]
    if missing:
        raise HTTPException(422, f"UE has no run snapshot configuration: {', '.join(missing)}")
    duplicates = database.scalars(select(models.TrafficJob).where(
        models.TrafficJob.run_id == run_id,
        models.TrafficJob.ue.in_(requested),
        models.TrafficJob.status.in_(ACTIVE_TRAFFIC_STATUSES),
    )).all()
    if duplicates:
        names = ", ".join(sorted({job.ue for job in duplicates}))
        raise HTTPException(409, f"active traffic already exists for {names}")
    batch_id = str(uuid.uuid4())
    created: list[tuple[models.TrafficJob, dict[str, Any]]] = []
    for ue in requested:
        flows = configurations[ue]["traffic"]["flows"]
        for index, flow in enumerate(flows):
            if flow["type"] == "none":
                continue
            created.append(create_traffic_job(database, run, ue, flow, batch_id, index))
    if not created:
        raise HTTPException(422, "selected UEs have no enabled traffic flow")
    database.commit()
    for job, worker_payload in created:
        Thread(target=traffic_worker, args=(job.id, worker_payload), daemon=True).start()
    return [job for job, _ in created]


@app.post("/api/runs/{run_id}/traffic", response_model=schemas.TrafficOut, status_code=202)
def start_traffic(run_id: str, payload: schemas.TrafficCreate, database: Session = Depends(get_db)):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    if run.state != "RUNNING":
        raise HTTPException(409, "traffic requires a RUNNING experiment")
    duplicate = database.scalar(select(models.TrafficJob).where(
        models.TrafficJob.run_id == run_id,
        models.TrafficJob.ue == payload.ue,
        models.TrafficJob.status.in_(ACTIVE_TRAFFIC_STATUSES),
    ))
    if duplicate:
        raise HTTPException(409, f"{payload.ue} already has active traffic")
    flow = normalize_traffic_flow({
        "type": "ping" if payload.protocol == "ping" else "iperf",
        "transport": "icmp" if payload.protocol == "ping" else payload.protocol,
        "direction": payload.direction,
        "run_mode": "duration",
        "duration_seconds": payload.duration,
        "params": {"bitrate": payload.bitrate},
    })
    job, worker_payload = create_traffic_job(database, run, payload.ue, flow, str(uuid.uuid4()))
    database.commit()
    Thread(target=traffic_worker, args=(job.id, worker_payload), daemon=True).start()
    return job


@app.delete("/api/runs/{run_id}/traffic/{job_id}", response_model=schemas.TrafficOut, status_code=202)
def stop_traffic(run_id: str, job_id: str, database: Session = Depends(get_db)):
    job = database.get(models.TrafficJob, job_id)
    if not job or job.run_id != run_id:
        raise HTTPException(404, "traffic job not found")
    if job.status not in {"QUEUED", "RUNNING"}:
        raise HTTPException(409, f"traffic job is already {job.status}")
    job.status = "STOP_REQUESTED"
    database.commit()
    with traffic_lock:
        process = traffic_processes.get(job_id)
    if process:
        process.send_signal(signal.SIGINT)
    return job


@app.delete("/api/runs/{run_id}/traffic", response_model=list[schemas.TrafficOut], status_code=202)
def stop_all_traffic(run_id: str, database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    jobs = database.scalars(select(models.TrafficJob).where(
        models.TrafficJob.run_id == run_id,
        models.TrafficJob.status.in_(["QUEUED", "RUNNING"]),
    )).all()
    for job in jobs:
        job.status = "STOP_REQUESTED"
    database.commit()
    with traffic_lock:
        processes = [(job.id, traffic_processes.get(job.id)) for job in jobs]
    for _, process in processes:
        if process:
            process.send_signal(signal.SIGINT)
    return jobs


def voiceguard_state_path(run_id: str) -> Path:
    return VOICEGUARD_ROOT / f"{run_id}.json"


def is_voiceguard_pid(pid: int, run_id: str) -> bool:
    try:
        command = Path(f"/proc/{pid}/cmdline").read_bytes().replace(b"\0", b" ").decode(errors="replace")
    except OSError:
        return False
    return str(VOICEGUARD_SCRIPT) in command and run_id in command


def read_voiceguard_state(run_id: str) -> dict[str, Any]:
    path = voiceguard_state_path(run_id)
    try:
        state = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        state = {
            "run_id": run_id,
            "running": False,
            "state": "OFF",
            "mode": "observe_only",
            "e2_adapter": "unavailable",
            "native_control": False,
            "last_decision": "尚未啟動 VoiceGuard",
            "events": [],
            "ues": {},
        }
    process = voiceguard_processes.get(run_id)
    if process is not None and process.poll() is not None:
        state["running"] = False
        if state.get("state") not in {"OFF", "ERROR"}:
            state["state"] = "ERROR"
            state["last_decision"] = f"VoiceGuard process exited with code {process.returncode}"
    elif process is not None:
        state["running"] = True
        state["pid"] = process.pid
    else:
        pid = state.get("pid")
        state["running"] = isinstance(pid, int) and pid > 1 and is_voiceguard_pid(pid, run_id)
    try:
        control = json.loads(
            (VOICEGUARD_ROOT / f"{run_id}.traffic-control.json").read_text()
        )
        state["traffic_shaping_factor"] = float(
            control.get("ues", {}).get("ue1", 1.0)
        )
    except (OSError, ValueError, TypeError, json.JSONDecodeError):
        pass
    return state


@app.get("/api/runs/{run_id}/xapps/voiceguard")
def voiceguard_status(run_id: str, database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    return read_voiceguard_state(run_id)


@app.post("/api/runs/{run_id}/xapps/voiceguard/start", status_code=202)
def start_voiceguard(
    run_id: str,
    payload: schemas.VoiceGuardStart,
    database: Session = Depends(get_db),
):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    if run.state != "RUNNING":
        raise HTTPException(409, "VoiceGuard requires a RUNNING experiment")
    if not VOICEGUARD_SCRIPT.exists():
        raise HTTPException(500, f"VoiceGuard script not found: {VOICEGUARD_SCRIPT}")
    if payload.mode == "closed_loop" and not os.access(VOICEGUARD_RC_BRIDGE, os.X_OK):
        raise HTTPException(503, f"VoiceGuard RC bridge is not executable: {VOICEGUARD_RC_BRIDGE}")
    config = payload.config.model_dump()
    config["traffic_control_file"] = str(
        VOICEGUARD_ROOT / f"{run_id}.traffic-control.json"
    )
    if config["voice_dedicated_prb_percent"] > config["voice_min_prb_percent"]:
        raise HTTPException(422, "voice_dedicated_prb_percent must be <= voice_min_prb_percent")
    if len({config["ue1_f1ap_id"], config["ue2_f1ap_id"], config["ue3_f1ap_id"]}) != 3:
        raise HTTPException(422, "UE F1AP IDs must be unique")
    with voiceguard_lock:
        current = voiceguard_processes.get(run_id)
        if current and current.poll() is None:
            raise HTTPException(409, "VoiceGuard is already running")
        state = read_voiceguard_state(run_id)
        if state.get("running"):
            raise HTTPException(409, "VoiceGuard is already running")
        state_path = voiceguard_state_path(run_id)
        command = [
            sys.executable,
            str(VOICEGUARD_SCRIPT),
            "--run-id", run_id,
            "--manager-url", MANAGER_SELF_URL,
            "--state-file", str(state_path),
            "--mode", payload.mode,
            "--config-json", json.dumps(config),
        ]
        process = subprocess.Popen(
            command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True,
        )
        voiceguard_processes[run_id] = process
    add_event(
        database, run_id, "voiceguard_started",
        f"VoiceGuard xApp 已用 {payload.mode} 模式啟動",
        component="voiceguard", mode=payload.mode, pid=process.pid,
    )
    database.commit()
    return {
        "run_id": run_id,
        "running": True,
        "state": "STARTING",
        "mode": payload.mode,
        "pid": process.pid,
        "native_control": payload.mode == "closed_loop",
    }


@app.post("/api/runs/{run_id}/xapps/voiceguard/stop", status_code=202)
def stop_voiceguard(run_id: str, database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    state = read_voiceguard_state(run_id)
    process = voiceguard_processes.get(run_id)
    pid = process.pid if process and process.poll() is None else state.get("pid")
    if not state.get("running") or not isinstance(pid, int):
        raise HTTPException(409, "VoiceGuard is not running")
    if not (process and process.poll() is None) and not is_voiceguard_pid(pid, run_id):
        raise HTTPException(409, "VoiceGuard state is stale; refusing to signal an unrelated process")
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    add_event(
        database, run_id, "voiceguard_stop_requested",
        "正在停止 VoiceGuard；Closed Loop 會先恢復 UE1/UE2/UE3 基線 PRB policy",
        component="voiceguard",
    )
    database.commit()
    return {**state, "running": False, "state": "STOPPING"}


@app.get("/api/runs/{run_id}/components")
def run_components(run_id: str, database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    return platform_status()


@app.get("/api/runs/{run_id}/ues/{ue}/config")
def run_ue_config(run_id: str, ue: str, database: Session = Depends(get_db)):
    run = database.get(models.ExperimentRun, run_id)
    if not run:
        raise HTTPException(404, "run not found")
    if ue not in {"ue1", "ue2", "ue3"} or not run.snapshot_path:
        raise HTTPException(404, "UE config not found")
    snapshot = Path(run.snapshot_path).resolve()
    config_path = (snapshot / "configs" / f"{ue}.conf").resolve()
    try:
        config_path.relative_to(snapshot)
        content = config_path.read_text()
    except (ValueError, OSError) as exc:
        raise HTTPException(404, "UE config not found") from exc
    sensitive = re.compile(r"^(\s*(?:k|opc|op|pin|password)\s*=).*$", re.IGNORECASE | re.MULTILINE)
    redacted = sensitive.sub(r"\1 ******** (redacted)", content)
    return {"run_id": run_id, "ue": ue, "path": str(config_path), "redacted": True, "content": redacted}


@app.get("/api/runs/{run_id}/metrics/query")
def metrics_query(run_id: str, metric: str = Query(pattern="^[a-z_]+$"), database: Session = Depends(get_db)):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    metric_name = ALLOWED_METRICS.get(metric)
    if not metric_name:
        raise HTTPException(400, "metric is not allow-listed")
    query = f'{metric_name}{{run_id="{run_id}"}}'
    try:
        with httpx.Client(trust_env=False, timeout=3) as client:
            response = client.get(f"{PROMETHEUS}/api/v1/query", params={"query": query})
        response.raise_for_status()
        return response.json()
    except (httpx.HTTPError, ValueError) as exc:
        raise HTTPException(502, f"Prometheus unavailable: {exc}") from exc


@app.get("/api/runs/{run_id}/metrics/range")
def metrics_range(
    run_id: str,
    metric: str = Query(pattern="^[a-z_]+$"),
    start: float = Query(),
    end: float = Query(),
    step: str = Query(default="2s", pattern=r"^[1-9][0-9]*[smh]$"),
    database: Session = Depends(get_db),
):
    if not database.get(models.ExperimentRun, run_id):
        raise HTTPException(404, "run not found")
    metric_name = ALLOWED_METRICS.get(metric)
    if not metric_name:
        raise HTTPException(400, "metric is not allow-listed")
    if end <= start or end - start > 86400:
        raise HTTPException(400, "range must be positive and no longer than 24 hours")
    query = f'{metric_name}{{run_id="{run_id}"}}'
    try:
        with httpx.Client(trust_env=False, timeout=5) as client:
            response = client.get(f"{PROMETHEUS}/api/v1/query_range", params={
                "query": query, "start": start, "end": end, "step": step,
            })
        response.raise_for_status()
        return response.json()
    except (httpx.HTTPError, ValueError) as exc:
        raise HTTPException(502, f"Prometheus unavailable: {exc}") from exc


FRONTEND_DIST = Path(__file__).resolve().parents[2] / "frontend" / "dist"
if FRONTEND_DIST.exists():
    app.mount("/", StaticFiles(directory=FRONTEND_DIST, html=True), name="frontend")
