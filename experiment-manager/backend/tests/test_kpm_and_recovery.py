from __future__ import annotations

import pytest
from fastapi import HTTPException

from app import main, models
from app.database import SessionLocal


def make_run(state: str) -> str:
    with SessionLocal() as database:
        experiment = models.Experiment(name=f"recovery {state} {id(database)}")
        database.add(experiment)
        database.flush()
        run = models.ExperimentRun(experiment_id=experiment.id, experiment_revision=1, state=state)
        database.add(run)
        database.commit()
        return run.id


def run_state(run_id: str) -> str:
    with SessionLocal() as database:
        return database.get(models.ExperimentRun, run_id).state


def test_kpm_metric_selector_adds_fixed_label_matchers():
    assert main.metric_selector("ue_rx_bps", "r1") == 'oran_ue_rx_bps{run_id="r1"}'
    assert main.metric_selector("kpm_ue_thp_dl", "r1") == (
        'oran_kpm_value{run_id="r1",scope="ue",measurement="DRB.UEThpDl"}'
    )
    with pytest.raises(HTTPException):
        main.metric_selector("up", "r1")


def test_running_run_without_radio_stack_is_marked_lost(monkeypatch):
    run_id = make_run("RUNNING")
    monkeypatch.setattr(main, "invoke", lambda action, **_: {
        "state": "STOPPED", "components": {"gnb": {"pid": 1, "running": False}},
    })
    with SessionLocal() as database:
        main.reconcile_lost_runs(database)
    assert run_state(run_id) == "LOST"


def test_running_run_with_live_radio_stack_is_kept(monkeypatch):
    run_id = make_run("RUNNING")
    monkeypatch.setattr(main, "invoke", lambda action, **_: {
        "state": "RUNNING", "components": {"gnb": {"pid": 1, "running": True}},
    })
    with SessionLocal() as database:
        main.reconcile_lost_runs(database)
    assert run_state(run_id) == "RUNNING"


def test_voiceguard_pid_check_recognises_the_3ue_runtime(monkeypatch, tmp_path):
    cmdline = tmp_path / "cmdline"
    cmdline.write_bytes(b"\0".join([
        b"/usr/bin/python3", str(main.VOICEGUARD_RF_3UE_SCRIPT).encode(), b"--run-id", b"run-3ue",
    ]))
    real_path = main.Path

    def fake_path(value, *args):
        if str(value) == "/proc/4242/cmdline":
            return cmdline
        return real_path(value, *args)

    monkeypatch.setattr(main, "Path", fake_path)
    assert main.is_voiceguard_pid(4242, "run-3ue")
