#!/usr/bin/env bash
set -euo pipefail

SESSION="${SESSION:-oran-stage1}"
LAB="${LAB:-/home/zju/Desktop/oran-lab}"

FLEXRIC_DIR="$LAB/src/flexric"
RIC_BIN="$FLEXRIC_DIR/build/examples/ric/nearRT-RIC"
XAPP_BIN="${XAPP_BIN:-$FLEXRIC_DIR/build/examples/xApp/c/monitor/xapp_kpm_moni}"

GNB_BIN="$LAB/src/ocudu/build/apps/gnb/gnb"
GNB_CONFIG="${GNB_CONFIG:-$LAB/config/ocudu/gnb-fdd-srsue-zmq-open5gs-flexric.yml}"
GNB_FALLBACK_CONFIG="$LAB/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml"

SRSUE_BIN="$LAB/src/srsRAN_4G/build/srsue/src/srsue"
SRSUE_CONFIG="${SRSUE_CONFIG:-$LAB/config/srsue/ue-zmq-open5gs.conf}"
RUN_UE="${RUN_UE:-1}"

if ! command -v tmux >/dev/null 2>&1; then
  echo "[ERROR] tmux not found. Install it with: sudo apt install -y tmux"
  exit 1
fi

if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "[INFO] tmux session '$SESSION' already exists. Attaching..."
  tmux attach -t "$SESSION"
  exit 0
fi

if [ ! -x "$RIC_BIN" ]; then
  echo "[ERROR] nearRT-RIC not found or not executable: $RIC_BIN"
  exit 1
fi

if [ ! -x "$GNB_BIN" ]; then
  echo "[ERROR] gNB not found or not executable: $GNB_BIN"
  exit 1
fi

if [ ! -x "$XAPP_BIN" ]; then
  echo "[ERROR] xApp not found or not executable: $XAPP_BIN"
  echo "[HINT] List xApps with:"
  echo "       find $FLEXRIC_DIR/build/examples/xApp -type f -executable | sort"
  exit 1
fi

if [ ! -f "$GNB_CONFIG" ]; then
  echo "[WARN] FlexRIC-enabled gNB config not found:"
  echo "       $GNB_CONFIG"
  echo "[WARN] Falling back to:"
  echo "       $GNB_FALLBACK_CONFIG"
  echo "[WARN] This can start Open5GS/gNB/UE, but xApp may not receive OCUDU KPM until E2 is enabled."
  GNB_CONFIG="$GNB_FALLBACK_CONFIG"
fi

if [ ! -f "$GNB_CONFIG" ]; then
  echo "[ERROR] gNB config not found: $GNB_CONFIG"
  exit 1
fi

if [ "$RUN_UE" = "1" ]; then
  if [ ! -x "$SRSUE_BIN" ]; then
    echo "[ERROR] srsUE not found or not executable: $SRSUE_BIN"
    exit 1
  fi
  if [ ! -f "$SRSUE_CONFIG" ]; then
    echo "[ERROR] srsUE config not found: $SRSUE_CONFIG"
    exit 1
  fi
fi

echo "[INFO] Refreshing sudo timestamp..."
sudo -v

tmux new-session -d -s "$SESSION" -n "stage1"

tmux send-keys -t "$SESSION:0.0" \
  "echo '[Open5GS logs]'; sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd -f -l" C-m

tmux split-window -h -t "$SESSION:0.0"
tmux send-keys -t "$SESSION:0.1" \
  "cd '$FLEXRIC_DIR' && echo '[nearRT-RIC]' && '$RIC_BIN'; exec bash" C-m

tmux split-window -v -t "$SESSION:0.0"
tmux send-keys -t "$SESSION:0.2" \
  "sleep 2; cd '$(dirname "$GNB_BIN")' && echo '[OCUDU gNB] config=$GNB_CONFIG' && sudo '$GNB_BIN' -c '$GNB_CONFIG'; exec bash" C-m

tmux split-window -v -t "$SESSION:0.1"
tmux send-keys -t "$SESSION:0.3" \
  "sleep 5; cd '$FLEXRIC_DIR' && echo '[xApp] $XAPP_BIN' && '$XAPP_BIN'; exec bash" C-m

if [ "$RUN_UE" = "1" ]; then
  tmux split-window -v -t "$SESSION:0.2"
  tmux send-keys -t "$SESSION:0.4" \
    "sleep 7; cd '$(dirname "$SRSUE_BIN")' && echo '[srsUE] config=$SRSUE_CONFIG' && sudo '$SRSUE_BIN' '$SRSUE_CONFIG'; exec bash" C-m
fi

tmux select-layout -t "$SESSION:0" tiled
tmux attach -t "$SESSION"
