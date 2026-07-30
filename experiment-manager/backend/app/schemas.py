from __future__ import annotations

from datetime import datetime
from typing import Any

from pydantic import BaseModel, ConfigDict, Field


class ORMModel(BaseModel):
    model_config = ConfigDict(from_attributes=True)


class ChannelProfile(BaseModel):
    enabled: bool = False
    awgn_enabled: bool = False
    awgn_snr: float = 30.0
    awgn_signal_power: float = 0.0
    fading_enabled: bool = False
    fading_model: str = "none"
    delay_enabled: bool = False
    delay_minimum_us: float = 0.0
    delay_maximum_us: float = 0.0
    delay_period_s: float = 1.0
    delay_init_time_s: float = 0.0
    rlf_enabled: bool = False
    rlf_t_on_ms: int = 1000
    rlf_t_off_ms: int = 1000
    hst_enabled: bool = False
    hst_fd_hz: float = 0.0
    hst_period_s: float = 1.0
    hst_init_time_s: float = 0.0


class UECreate(BaseModel):
    slot: int = Field(ge=1, le=10)
    display_name: str = Field(min_length=1, max_length=80)
    enabled: bool = True
    imsi: str = Field(pattern=r"^[0-9]{14,15}$")
    imei: str = ""
    credential_profile: str = "lab-default"
    apn: str = "internet"
    sst: int = Field(default=1, ge=0, le=255)
    sd: str = "000001"
    rx_port: int = Field(ge=1024, le=65535)
    tx_port: int = Field(ge=1024, le=65535)
    namespace: str
    path_loss_db: float = Field(default=0.0, ge=0, le=200)
    channel: ChannelProfile = Field(default_factory=ChannelProfile)
    traffic_defaults: dict[str, Any] = Field(default_factory=dict)


class UEUpdate(BaseModel):
    display_name: str | None = None
    enabled: bool | None = None
    apn: str | None = None
    path_loss_db: float | None = Field(default=None, ge=0, le=200)
    channel: ChannelProfile | None = None
    traffic_defaults: dict[str, Any] | None = None


class UEOut(ORMModel):
    id: str
    experiment_id: str
    slot: int
    display_name: str
    enabled: bool
    imsi: str
    imei: str
    credential_profile: str
    apn: str
    sst: int
    sd: str
    rx_port: int
    tx_port: int
    namespace: str
    path_loss_db: float
    channel: dict[str, Any]
    traffic_defaults: dict[str, Any]


class ExperimentCreate(BaseModel):
    name: str = Field(min_length=1, max_length=120)
    description: str = ""
    expected_ue_count: int = Field(default=3, ge=1, le=10)
    broker_capacity: int = Field(default=3, ge=1, le=10)
    monitoring_enabled: bool = True
    scenario: str = "clean"


class ExperimentUpdate(BaseModel):
    name: str | None = None
    description: str | None = None
    expected_ue_count: int | None = Field(default=None, ge=1, le=10)
    broker_capacity: int | None = Field(default=None, ge=1, le=10)
    monitoring_enabled: bool | None = None
    scenario: str | None = None


class ExperimentOut(ORMModel):
    id: str
    name: str
    description: str
    expected_ue_count: int
    broker_capacity: int
    monitoring_enabled: bool
    scenario: str
    revision: int
    created_at: datetime
    updated_at: datetime
    ues: list[UEOut] = []


class RunOut(ORMModel):
    id: str
    experiment_id: str
    experiment_revision: int
    state: str
    operation: str | None
    started_at: datetime | None
    stopped_at: datetime | None
    snapshot_path: str | None
    allocated_ues: dict[str, Any]
    result_summary: dict[str, Any]


class EventOut(ORMModel):
    id: str
    run_id: str
    timestamp: datetime
    component: str
    severity: str
    event_type: str
    message: str
    details: dict[str, Any]


class TrafficCreate(BaseModel):
    ue: str = Field(pattern=r"^ue(?:[1-9]|10)$")
    protocol: str = Field(pattern=r"^(ping|tcp|udp)$")
    direction: str = Field(default="UL", pattern=r"^(UL|DL)$")
    target: str = "10.45.0.1"
    duration: int = Field(default=10, ge=1, le=300)
    bitrate: str = Field(default="2M", pattern=r"^[1-9][0-9]*(?:\.[0-9]+)?[KMG]?$")


class TrafficBatchCreate(BaseModel):
    ues: list[str] = Field(min_length=1, max_length=10)


class TrafficOut(ORMModel):
    id: str
    run_id: str
    ue: str
    batch_id: str | None
    traffic_type: str
    application_protocol: str
    transport: str
    protocol: str
    direction: str
    target: str
    port: int
    duration: int
    duration_seconds: int | None
    run_mode: str
    bitrate: str
    parameters: dict[str, Any]
    status: str
    created_at: datetime
    started_at: datetime | None
    finished_at: datetime | None
    result: dict[str, Any]


class ConfigUpdate(BaseModel):
    content: str = Field(min_length=1, max_length=512_000)


class VoiceGuardConfig(BaseModel):
    algorithm: str = Field(default="random_forest", pattern=r"^(rules|random_forest)$")
    model_path: str | None = Field(default=None, max_length=1024)
    sample_interval_seconds: float = Field(default=1.0, ge=0.5, le=10.0)
    congestion_threshold_mbps: float = Field(default=1.2, ge=0.01, le=100.0)
    voice_loss_threshold_percent: float = Field(default=2.0, ge=0.0, le=100.0)
    voice_latency_threshold_ms: float = Field(default=120.0, ge=1.0, le=5000.0)
    voice_jitter_threshold_ms: float = Field(default=30.0, ge=0.0, le=5000.0)
    consecutive_samples: int = Field(default=3, ge=1, le=30)
    recovery_samples: int = Field(default=5, ge=1, le=60)
    cooldown_seconds: int = Field(default=5, ge=1, le=300)
    restore_step_seconds: float = Field(default=3.0, ge=1.0, le=60.0)
    video_offered_scale_percent: int = Field(default=60, ge=10, le=100)
    video_max_prb_percent: int = Field(default=100, ge=1, le=100)
    voice_min_prb_percent: int = Field(default=0, ge=0, le=100)
    voice_dedicated_prb_percent: int = Field(default=0, ge=0, le=100)
    rc_timeout_seconds: float = Field(default=10.0, ge=2.0, le=30.0)
    ue1_f1ap_id: int = Field(default=0, ge=0)
    ue2_f1ap_id: int = Field(default=1, ge=0)
    ue3_f1ap_id: int = Field(default=2, ge=0)
    sst: int = Field(default=1, ge=0, le=255)
    sd: str = Field(default="ffffff", pattern=r"^[0-9a-fA-F]{6}$")


class VoiceGuardStart(BaseModel):
    mode: str = Field(default="observe_only", pattern=r"^(observe_only|closed_loop)$")
    config: VoiceGuardConfig = Field(default_factory=VoiceGuardConfig)
