# VoiceGuard RF V2

VoiceGuard RF V2 is an adapted reproduction of the policy-selection method in
“Machine Learning-based xApp for Dynamic Resource Allocation in O-RAN
Networks” (`arXiv:2401.07643`). The paper evaluates every candidate allocation
policy for a network configuration, labels the policy with the lowest outage,
and trains a Random Forest to select that policy online.

This lab uses the same workflow with actuators and metrics that the local
O-CU-DU/FlexRIC platform can actually enforce:

1. Run all four video pacing policies on the real 10 UE radio stack.
2. Measure voice delivery, loss, jitter and RTT P95 for each policy.
3. Label the least restrictive policy that meets the shared voice SLA.
4. Train a Random Forest classifier from baseline state to that policy.
5. Run online inference in the Python xApp and enforce the selected pacing
   policy, while keeping E2SM-RC grant limits at the verified safety baseline.

This is an adapted demo reproduction, not a bit-for-bit reproduction of the
paper's Python HetNet simulator.

## Scenario

| UE | Application | Configuration |
| --- | --- | --- |
| UE1–UE8 | HTTP short video | 1.0 Mbps base, ±35%, mixed wave/random burst, 1.35 Mbps peak |
| UE9–UE10 | RTP-like voice | 96 Kbps, 20 ms packet interval, started/stopped as incoming calls |

The candidate actions are `EQUAL_100`, `LIGHT_85`, `MEDIUM_70` and
`STRONG_40`. Only UE1–UE8 are paced; voice stays at 100%.

The shared 3-second voice SLA is:

- delivery ratio ≥ 95%
- packet loss ≤ 2%
- jitter ≤ 30 ms
- RTT P95 ≤ 120 ms

The 120 ms RTT target is specific to this 10 UE software-radio topology. It is
still about 60 ms one-way, but does not classify the known single-thread
broker/radio scheduling floor as an application failure.

## Reproduce data collection and training

Start the `VoiceGuard RF · 8 Video + 2 Voice` experiment, then use its run ID:

```bash
PYTHONPATH=xapps/voiceguard_rf \
experiment-manager/backend/.venv/bin/python xapps/voiceguard_rf/collect.py \
  --run-id RUN_ID \
  --output-dir xapps/voiceguard_rf/data/RUN_ID \
  --control-file experiment-manager/backend/data/voiceguard/RUN_ID.traffic-control.json \
  --rounds 3 --warmup-seconds 3 --sample-seconds 6
```

If the SLA is adjusted, the measured policy windows can be relabelled without
rerunning the radio:

```bash
PYTHONPATH=xapps/voiceguard_rf \
experiment-manager/backend/.venv/bin/python xapps/voiceguard_rf/relabel.py \
  --raw-samples xapps/voiceguard_rf/data/RUN_ID/raw_samples.csv \
  --output-dir xapps/voiceguard_rf/dataset \
  --run-id RUN_ID
```

Train the model:

```bash
PYTHONPATH=xapps/voiceguard_rf \
experiment-manager/backend/.venv/bin/python xapps/voiceguard_rf/train.py \
  --dataset xapps/voiceguard_rf/dataset/training.csv \
  --model xapps/voiceguard_rf/models/voiceguard_rf.joblib \
  --report xapps/voiceguard_rf/models/training_report.json
```

The production dataset currently contains 216 raw measurements, 54 baseline
training rows and 9 independent radio scenarios. Scenario-group
cross-validation is used so samples from the same experiment window cannot
leak into both training and validation. Current accuracy is 55.6%; this is an
honest small-data baseline, not the paper's reported >85% result. More
independent collection rounds are required before treating the RF itself as a
production controller.

## Runtime behavior

The xApp waits for three samples after the active call combination changes,
runs one RF inference, and applies that policy. During the same call:

- three consecutive SLA failures escalate protection by one level;
- protection never relaxes while the call remains active;
- when all calls stop, video restores every three seconds:
  `40% → 70% → 85% → 100%`.

This hysteresis prevents class-probability noise from creating policy
oscillation. The deterministic layer may only make the RF choice safer.

## Code map

- `common.py`: UE roles, policies, feature order and shared SLA.
- `collect.py`: real radio policy sweep and raw measurement collection.
- `relabel.py`: deterministic rebuilding of labels from raw policy windows.
- `train.py`: Random Forest training and scenario-group validation.
- `voiceguard_rf.py`: online model inference, safety state machine, traffic
  pacing and 10 UE native E2SM-RC baseline verification.
- `dataset/`: production CSV and policy outcome evidence.
- `models/`: serialized model and machine-readable training report.

## What to look for in the UI

Start UE1–UE8 first, enable `Random Forest V2 / Closed Loop`, then start UE9,
UE10 or both. The `Offered Load vs Delivered Throughput` chart should show the
eight dashed Offered lines falling together after a protection decision. The
voice panel should show delivery/loss/jitter/RTT returning inside the SLA.
The RF card shows the recommended policy, probability confidence, inference
time and the policy held by the safety layer.

When UE9 and UE10 stop, the chart should show the three-step video recovery.
The event list/state JSON records `policy_applied`, `safety_escalation` and
`policy_restored` transitions for verification.
