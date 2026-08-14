# VoiceGuard RF V2 — 3 UE

This is the Random Forest baseline for the fixed three-UE QoS demo:

- UE1 and UE2 generate fluctuating HTTP/TCP downlink short-video traffic.
- UE3 generates a continuous bidirectional RTP-like/UDP voice call.
- The model selects the least restrictive tested video pacing action that can
  keep UE3 within the measured voice SLA.

## State, actions, and SLA

The 12 model inputs are the most recent three-second median of offered/delivered video load, per-video-UE offered
load and imbalance, plus UE3 delivery ratio, packet loss, jitter, and RTT P95.
Constants such as UE count are intentionally excluded. Raw HTTP segment
throughput may exceed paced offered load because it is a burst rate; raw data
is preserved, while model delivery is capped at offered load before deriving
ratio and gap features.

The action space is `EQUAL_100`, `LIGHT_85`, `MEDIUM_70`, and `STRONG_40`.
The number is the percentage of UE1/UE2 offered load retained. UE3 is never
paced by this actuator.

A policy window passes when at least 75% of its samples satisfy all of:

- delivery ratio >= 95%
- packet loss <= 2%
- jitter <= 30 ms
- RTT P95 <= 60 ms

The 60 ms RTT target is deliberately tighter than the earlier 10-UE demo:
clean three-UE calls on this software-radio stack are normally around 35–40
ms. The collector tests every action against every load pair instead of
manufacturing labels from a formula.

## Reproducible pipeline

Run the collector against an already running experiment whose snapshot has
UE1/UE2 `short_video` and UE3 `rtp_voice` traffic:

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue experiment-manager/backend/.venv/bin/python \
  xapps/voiceguard_rf_3ue/collect.py \
  --run-id RUN_ID \
  --output-dir xapps/voiceguard_rf_3ue/dataset \
  --control-file experiment-manager/backend/data/voiceguard/RUN_ID.traffic-control.json \
  --rounds 3 --levels 0.15,0.25,0.40,0.55,0.75,1.00 \
  --campaign fair-base8-w3 --warmup-seconds 3 --sample-seconds 3 --resume
```

`collection_results.json` is the resumable collector checkpoint.
`relabel.py` then combines repeated rounds of the same UE1/UE2 load pair and
writes the final `policy_results.json` and `training.csv`:

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue experiment-manager/backend/.venv/bin/python \
  xapps/voiceguard_rf_3ue/relabel.py \
  --raw-samples xapps/voiceguard_rf_3ue/dataset/raw_samples.csv \
  --output-dir xapps/voiceguard_rf_3ue/dataset --run-id RUN_ID
```

Train and group-validate the model:

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue experiment-manager/backend/.venv/bin/python \
  xapps/voiceguard_rf_3ue/train.py \
  --dataset xapps/voiceguard_rf_3ue/dataset/training.csv \
  --model xapps/voiceguard_rf_3ue/models/voiceguard_rf_3ue.joblib \
  --report xapps/voiceguard_rf_3ue/models/training_report.json
```

Each training row is a three-second median, matching runtime inference.
Validation uses `GroupKFold` by offered-load pair, so repeated windows and
rounds from one load condition cannot appear in both train and test folds.
The report includes exact class accuracy and counterfactual SLA success of
the selected action. The latter is the more useful demo metric because an
adjacent class can still preserve voice quality.

## Runtime

`voiceguard_rf_3ue.py` uses the shared RF closed-loop state machine. It waits
for three UE3 samples, uses the model's 80th-percentile policy (a conservative
risk decision rather than raw class argmax), applies UE1/UE2 pacing, and
escalates one level after three consecutive SLA violations. On stop it
atomically restores 100% video load. In closed-loop mode it also verifies a
three-UE E2SM-RC baseline through the native FlexRIC bridge; traffic pacing is
the actuator that creates the visible throughput effect in this lab.

The same state snapshot and measured outcomes can later be supplied to a
local vLLM policy. Keeping state, action set, SLA, and experimental load pairs
identical makes RF-versus-LLM comparison meaningful.
