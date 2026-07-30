from __future__ import annotations

import json

from fastapi.testclient import TestClient

from app import config_generator, main, models
from app.database import Base, SessionLocal, engine
from app.main import app


def setup_module():
    Base.metadata.drop_all(engine)
    Base.metadata.create_all(engine)


client = TestClient(app)


def test_health():
    response = client.get("/api/health")
    assert response.status_code == 200
    assert response.json()["status"] == "ok"


def test_experiment_and_ue_crud():
    experiment = client.post("/api/experiments", json={"name": "pytest experiment"})
    assert experiment.status_code == 201
    experiment_id = experiment.json()["id"]

    ue = client.post(f"/api/experiments/{experiment_id}/ues", json={
        "slot": 1,
        "display_name": "UE 1",
        "imsi": "001010123456780",
        "rx_port": 2100,
        "tx_port": 2101,
        "namespace": "ue1",
    })
    assert ue.status_code == 201
    assert ue.json()["channel"]["awgn_snr"] == 30.0

    listed = client.get("/api/experiments").json()
    assert len(listed) == 1
    assert listed[0]["ues"][0]["imsi"] == "001010123456780"


def test_metric_allowlist():
    response = client.get("/api/runs/not-real/metrics/query", params={"metric": "up"})
    assert response.status_code == 404


def test_ue_traffic_profile_and_snapshot_batch(monkeypatch, tmp_path):
    experiment = client.get("/api/experiments").json()[0]
    ue = experiment["ues"][0]
    traffic = {
        "version": 2,
        "flows": [{
            "type": "short_video",
            "application_protocol": "http",
            "transport": "tcp",
            "direction": "DL",
            "run_mode": "continuous",
            "duration_seconds": None,
            "params": {"segment_size_kb": 512, "segment_interval_ms": 2000},
        }],
    }
    updated = client.patch(
        f"/api/experiments/{experiment['id']}/ues/{ue['id']}",
        json={"traffic_defaults": traffic},
    )
    assert updated.status_code == 200
    assert updated.json()["traffic_defaults"]["flows"][0]["run_mode"] == "continuous"

    snapshot = tmp_path / "run"
    snapshot.mkdir()
    manifest = {
        "experiment": {
            "ues": [{
                "slot": 1, "display_name": "UE 1", "namespace": "ue1",
                "enabled": True, "traffic_defaults": traffic,
            }],
        },
    }
    (snapshot / "manifest.json").write_text(json.dumps(manifest))
    with SessionLocal() as database:
        run = models.ExperimentRun(
            experiment_id=experiment["id"], experiment_revision=1,
            state="RUNNING", snapshot_path=str(snapshot),
        )
        database.add(run)
        database.commit()
        run_id = run.id

    class NoopThread:
        def __init__(self, *args, **kwargs):
            pass

        def start(self):
            pass

    monkeypatch.setattr(main, "Thread", NoopThread)
    configuration = client.get(f"/api/runs/{run_id}/traffic/config")
    assert configuration.status_code == 200
    assert configuration.json()["ues"][0]["traffic"]["flows"][0]["type"] == "short_video"
    launched = client.post(f"/api/runs/{run_id}/traffic/batch", json={"ues": ["ue1"]})
    assert launched.status_code == 202
    job = launched.json()[0]
    assert job["traffic_type"] == "short_video"
    assert job["duration_seconds"] is None
    assert job["run_mode"] == "continuous"
    stopped = client.delete(f"/api/runs/{run_id}/traffic/{job['id']}")
    assert stopped.status_code == 202
    assert stopped.json()["status"] == "STOP_REQUESTED"


def test_gnb_config_editor_validates_and_persists(monkeypatch, tmp_path):
    monkeypatch.setattr(config_generator, "DEFINITIONS_ROOT", tmp_path)
    experiment = client.post("/api/experiments", json={"name": "pytest gnb config"})
    assert experiment.status_code == 201
    experiment_id = experiment.json()["id"]

    loaded = client.get(f"/api/experiments/{experiment_id}/gnb/config")
    assert loaded.status_code == 200
    assert loaded.json()["component"] == "gnb"
    assert loaded.json()["custom"] is False

    edited = loaded.json()["content"].replace("all_level: info", "all_level: warning")
    saved = client.put(f"/api/experiments/{experiment_id}/gnb/config", json={"content": edited})
    assert saved.status_code == 200
    assert saved.json()["custom"] is True
    assert "all_level: warning" in saved.json()["content"]

    broken = edited.replace("rx_port=tcp://localhost:2001", "rx_port=tcp://localhost:2999")
    rejected = client.put(f"/api/experiments/{experiment_id}/gnb/config", json={"content": broken})
    assert rejected.status_code == 422


def test_dynamic_video_offered_load_validation():
    experiment = client.post("/api/experiments", json={"name": "dynamic video profile"})
    assert experiment.status_code == 201
    experiment_id = experiment.json()["id"]
    ue = client.post(f"/api/experiments/{experiment_id}/ues", json={
        "slot": 1,
        "display_name": "Video UE",
        "imsi": "001010123456789",
        "rx_port": 2500,
        "tx_port": 2501,
        "namespace": "ue1",
    }).json()
    flow = {
        "version": 2,
        "flows": [{
            "type": "short_video",
            "run_mode": "continuous",
            "params": {
                "offered_load_mbps": 0.8,
                "traffic_pattern": "random_burst",
                "variation_percent": 30,
                "peak_limit_mbps": 1.2,
                "random_seed": 1234,
                "pattern_period_seconds": 20,
                "segment_interval_ms": 1000,
            },
        }],
    }
    saved = client.patch(
        f"/api/experiments/{experiment_id}/ues/{ue['id']}",
        json={"traffic_defaults": flow},
    )
    assert saved.status_code == 200
    params = saved.json()["traffic_defaults"]["flows"][0]["params"]
    assert params["offered_load_mbps"] == 0.8
    assert params["traffic_pattern"] == "random_burst"

    flow["flows"][0]["params"]["peak_limit_mbps"] = 0.4
    rejected = client.patch(
        f"/api/experiments/{experiment_id}/ues/{ue['id']}",
        json={"traffic_defaults": flow},
    )
    assert rejected.status_code == 422
    assert "peak_limit_mbps" in rejected.json()["detail"]
