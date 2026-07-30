#!/usr/bin/env python3
"""Headless GNU Radio ZMQ broker for one gNB and up to ten srsUEs.

The gNB downlink is copied to every active UE.  The active UE uplinks are
attenuated independently and added into the single composite waveform received
by the gNB.  UE admission is staged by the lifecycle controller so that
identical simulated UEs do not attempt Random Access at the same instant.
"""

from __future__ import annotations

import math
import signal
import threading
import time
from pathlib import Path

from gnuradio import blocks, gr, zeromq


SAMPLE_RATE = 23_040_000
ZMQ_TIMEOUT_MS = 100
ZMQ_HIGH_WATER_MARK = 10
MUTED_PATH_LOSS_DB = 200.0
ADMISSION_STAGE_FILE = Path("/home/zju/Desktop/oran-lab/run/manager/ue-admission-stage")

# These two assignments are replaced in immutable run snapshots.
ACTIVE_UE_SLOTS = [1, 2, 3]
CONFIGURED_PATH_LOSSES = {1: 0.0, 2: 10.0, 3: 20.0}


def amplitude(path_loss_db: float) -> float:
    return math.pow(10.0, -path_loss_db / 20.0)


def ue_base_port(slot: int) -> int:
    # Port 3001 is Grafana in this lab, so UE10 uses the next free pair.
    return 2000 + slot * 100 if slot < 10 else 3100


class MultiUEBroker(gr.top_block):
    def __init__(self, slots: list[int], path_losses: dict[int, float]):
        super().__init__("O-RAN multi-UE ZMQ broker", catch_exceptions=True)
        if not slots or len(slots) > 10 or len(set(slots)) != len(slots):
            raise ValueError("broker requires 1 to 10 unique UE slots")
        if any(slot < 1 or slot > 10 for slot in slots):
            raise ValueError("UE slots must be in range 1..10")

        self.slots = sorted(slots)
        self.path_losses = {
            slot: float(path_losses.get(slot, 0.0)) for slot in self.slots
        }
        self.admitted_stage = 0
        self.ul_gains: dict[int, blocks.multiply_const_cc] = {}
        self.dl_gains: dict[int, blocks.multiply_const_cc] = {}

        gnb_dl_source = zeromq.req_source(
            gr.sizeof_gr_complex,
            1,
            "tcp://127.0.0.1:2000",
            ZMQ_TIMEOUT_MS,
            False,
            ZMQ_HIGH_WATER_MARK,
        )
        dl_throttle = blocks.throttle(
            gr.sizeof_gr_complex, SAMPLE_RATE, True
        )
        ul_sum = blocks.add_vcc(1)
        ul_throttle = blocks.throttle(
            gr.sizeof_gr_complex, SAMPLE_RATE, True
        )
        gnb_ul_sink = zeromq.rep_sink(
            gr.sizeof_gr_complex,
            1,
            "tcp://127.0.0.1:2001",
            ZMQ_TIMEOUT_MS,
            False,
            ZMQ_HIGH_WATER_MARK,
        )

        self.connect(gnb_dl_source, dl_throttle)
        self.connect(ul_sum, ul_throttle, gnb_ul_sink)

        for input_index, slot in enumerate(self.slots):
            base_port = ue_base_port(slot)
            ue_ul_source = zeromq.req_source(
                gr.sizeof_gr_complex,
                1,
                f"tcp://127.0.0.1:{base_port + 1}",
                ZMQ_TIMEOUT_MS,
                False,
                ZMQ_HIGH_WATER_MARK,
            )
            ue_dl_sink = zeromq.rep_sink(
                gr.sizeof_gr_complex,
                1,
                f"tcp://127.0.0.1:{base_port}",
                ZMQ_TIMEOUT_MS,
                False,
                ZMQ_HIGH_WATER_MARK,
            )
            ul_gain = blocks.multiply_const_cc(amplitude(MUTED_PATH_LOSS_DB))
            dl_gain = blocks.multiply_const_cc(amplitude(MUTED_PATH_LOSS_DB))
            self.ul_gains[slot] = ul_gain
            self.dl_gains[slot] = dl_gain

            self.connect(ue_ul_source, ul_gain)
            self.connect((ul_gain, 0), (ul_sum, input_index))
            self.connect(dl_throttle, dl_gain, ue_dl_sink)

    def set_admission_stage(self, stage: int) -> None:
        stage = max(1, min(len(self.slots), stage))
        if stage == self.admitted_stage:
            return
        for position, slot in enumerate(self.slots, 1):
            loss = self.path_losses[slot] if position <= stage else MUTED_PATH_LOSS_DB
            gain = amplitude(loss)
            self.ul_gains[slot].set_k(gain)
            self.dl_gains[slot].set_k(gain)
        self.admitted_stage = stage
        print(f"UE admission stage {stage}/{len(self.slots)}", flush=True)


def requested_admission_stage(default: int = 1) -> int:
    try:
        return int(ADMISSION_STAGE_FILE.read_text().strip())
    except (OSError, ValueError):
        return default


def main() -> None:
    broker = MultiUEBroker(ACTIVE_UE_SLOTS, CONFIGURED_PATH_LOSSES)
    stopping = threading.Event()

    def request_stop(_: int, __: object) -> None:
        stopping.set()

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    broker.set_admission_stage(requested_admission_stage())
    broker.start()
    print(f"Broker ready for UE slots {ACTIVE_UE_SLOTS}", flush=True)
    try:
        while not stopping.wait(0.25):
            broker.set_admission_stage(requested_admission_stage())
    finally:
        broker.stop()
        broker.wait()


if __name__ == "__main__":
    main()
