from __future__ import annotations

import uuid
from datetime import datetime, timezone

from sqlalchemy import Boolean, DateTime, ForeignKey, Integer, JSON, String, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from .database import Base


def now() -> datetime:
    return datetime.now(timezone.utc)


def new_id() -> str:
    return str(uuid.uuid4())


class Experiment(Base):
    __tablename__ = "experiments"

    id: Mapped[str] = mapped_column(String, primary_key=True, default=new_id)
    name: Mapped[str] = mapped_column(String(120), unique=True)
    description: Mapped[str] = mapped_column(Text, default="")
    expected_ue_count: Mapped[int] = mapped_column(Integer, default=3)
    broker_capacity: Mapped[int] = mapped_column(Integer, default=3)
    monitoring_enabled: Mapped[bool] = mapped_column(Boolean, default=True)
    scenario: Mapped[str] = mapped_column(String(80), default="clean")
    revision: Mapped[int] = mapped_column(Integer, default=1)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=now)
    updated_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=now, onupdate=now)

    ues: Mapped[list[UEProfile]] = relationship(
        back_populates="experiment", cascade="all, delete-orphan", order_by="UEProfile.slot"
    )
    runs: Mapped[list[ExperimentRun]] = relationship(back_populates="experiment")


class UEProfile(Base):
    __tablename__ = "ue_profiles"

    id: Mapped[str] = mapped_column(String, primary_key=True, default=new_id)
    experiment_id: Mapped[str] = mapped_column(ForeignKey("experiments.id", ondelete="CASCADE"))
    slot: Mapped[int] = mapped_column(Integer)
    display_name: Mapped[str] = mapped_column(String(80))
    enabled: Mapped[bool] = mapped_column(Boolean, default=True)
    imsi: Mapped[str] = mapped_column(String(24))
    imei: Mapped[str] = mapped_column(String(24), default="")
    credential_profile: Mapped[str] = mapped_column(String(80), default="lab-default")
    apn: Mapped[str] = mapped_column(String(80), default="internet")
    sst: Mapped[int] = mapped_column(Integer, default=1)
    sd: Mapped[str] = mapped_column(String(8), default="000001")
    rx_port: Mapped[int] = mapped_column(Integer)
    tx_port: Mapped[int] = mapped_column(Integer)
    namespace: Mapped[str] = mapped_column(String(32))
    path_loss_db: Mapped[float] = mapped_column(default=0.0)
    channel: Mapped[dict] = mapped_column(JSON, default=dict)
    traffic_defaults: Mapped[dict] = mapped_column(JSON, default=dict)

    experiment: Mapped[Experiment] = relationship(back_populates="ues")


class ExperimentRun(Base):
    __tablename__ = "experiment_runs"

    id: Mapped[str] = mapped_column(String, primary_key=True, default=new_id)
    experiment_id: Mapped[str] = mapped_column(ForeignKey("experiments.id"))
    experiment_revision: Mapped[int] = mapped_column(Integer)
    state: Mapped[str] = mapped_column(String(32), default="STARTING")
    operation: Mapped[str | None] = mapped_column(String(40), nullable=True)
    started_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    stopped_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    snapshot_path: Mapped[str | None] = mapped_column(Text, nullable=True)
    allocated_ues: Mapped[dict] = mapped_column(JSON, default=dict)
    result_summary: Mapped[dict] = mapped_column(JSON, default=dict)

    experiment: Mapped[Experiment] = relationship(back_populates="runs")
    events: Mapped[list[RuntimeEvent]] = relationship(
        back_populates="run", cascade="all, delete-orphan", order_by="RuntimeEvent.timestamp"
    )


class RuntimeEvent(Base):
    __tablename__ = "runtime_events"

    id: Mapped[str] = mapped_column(String, primary_key=True, default=new_id)
    run_id: Mapped[str] = mapped_column(ForeignKey("experiment_runs.id", ondelete="CASCADE"))
    timestamp: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=now)
    component: Mapped[str] = mapped_column(String(80), default="manager")
    severity: Mapped[str] = mapped_column(String(16), default="info")
    event_type: Mapped[str] = mapped_column(String(60))
    message: Mapped[str] = mapped_column(Text)
    details: Mapped[dict] = mapped_column(JSON, default=dict)

    run: Mapped[ExperimentRun] = relationship(back_populates="events")


class TrafficJob(Base):
    __tablename__ = "traffic_jobs"

    id: Mapped[str] = mapped_column(String, primary_key=True, default=new_id)
    run_id: Mapped[str] = mapped_column(ForeignKey("experiment_runs.id", ondelete="CASCADE"))
    ue: Mapped[str] = mapped_column(String(16))
    batch_id: Mapped[str | None] = mapped_column(String, nullable=True)
    traffic_type: Mapped[str] = mapped_column(String(32), default="iperf")
    application_protocol: Mapped[str] = mapped_column(String(24), default="iperf3")
    transport: Mapped[str] = mapped_column(String(12), default="udp")
    protocol: Mapped[str] = mapped_column(String(8))
    direction: Mapped[str] = mapped_column(String(4))
    target: Mapped[str] = mapped_column(String(64), default="10.45.0.1")
    port: Mapped[int] = mapped_column(Integer)
    duration: Mapped[int] = mapped_column(Integer)
    duration_seconds: Mapped[int | None] = mapped_column(Integer, nullable=True)
    run_mode: Mapped[str] = mapped_column(String(16), default="duration")
    bitrate: Mapped[str] = mapped_column(String(16), default="2M")
    parameters: Mapped[dict] = mapped_column(JSON, default=dict)
    status: Mapped[str] = mapped_column(String(24), default="QUEUED")
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=now)
    started_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    finished_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    result: Mapped[dict] = mapped_column(JSON, default=dict)
