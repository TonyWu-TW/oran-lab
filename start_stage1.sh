#!/usr/bin/env bash
set -euo pipefail

LAB="${LAB:-/home/zju/Desktop/oran-lab}"
LOG_DIR="$LAB/logs/stage1"
RUN_DIR="$LAB/run/stage1"

FLEXRIC_DIR="$LAB/src/flexric"
RIC_BIN="$FLEXRIC_DIR/build/examples/ric/nearRT-RIC"
XAPP_BIN="${XAPP_BIN:-$FLEXRIC_DIR/build/examples/xApp/c/monitor/xapp_kpm_moni}"

GNB_BIN="$LAB/src/ocudu/build/apps/gnb/gnb"
GNB_CONFIG="${GNB_CONFIG:-$LAB/config/ocudu/gnb-fdd-srsue-zmq-open5gs-flexric.yml}"
GNB_FALLBACK_CONFIG="$LAB/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml"

SRSUE_BIN="$LAB/src/srsRAN_4G/build/srsue/src/srsue"
SRSUE_CONFIG="${SRSUE_CONFIG:-$LAB/config/srsue/ue-zmq-open5gs.conf}"
RUN_UE="${RUN_UE:-1}"

mkdir -p "$LOG_DIR" "$RUN_DIR"
rm -f "$LOG_DIR"/*.log "$RUN_DIR"/*.pid

require_file() {
  local path="$1"
  local label="$2"
  if [ ! -e "$path" ]; then
    echo "[ERROR] Missing $label: $path"
    exit 1
  fi
}

require_exec() {
  local path="$1"
  local label="$2"
  if [ ! -x "$path" ]; then
    echo "[ERROR] Missing executable $label: $path"
    exit 1
  fi
}

require_exec "$RIC_BIN" "nearRT-RIC"
require_exec "$GNB_BIN" "OCUDU gNB"
require_exec "$XAPP_BIN" "xApp"

if [ ! -f "$GNB_CONFIG" ]; then
  echo "[WARN] FlexRIC-enabled gNB config not found:"
  echo "       $GNB_CONFIG"
  echo "[WARN] Falling back to:"
  echo "       $GNB_FALLBACK_CONFIG"
  echo "[WARN] gNB/UE/Open5GS can still run, but xApp may not receive OCUDU KPM until E2 is enabled."
  GNB_CONFIG="$GNB_FALLBACK_CONFIG"
fi

require_file "$GNB_CONFIG" "gNB config"

if [ "$RUN_UE" = "1" ]; then
  require_exec "$SRSUE_BIN" "srsUE"
  require_file "$SRSUE_CONFIG" "srsUE config"
fi

start_bg() {
  local name="$1"
  shift
  local log="$LOG_DIR/$name.log"
  echo "[INFO] Starting $name"
  echo "[INFO]   log: $log"
  (
    echo "===== $name started at $(date -Is) ====="
    exec "$@"
  ) >"$log" 2>&1 &
  echo "$!" > "$RUN_DIR/$name.pid"
}

echo "[INFO] Refreshing sudo timestamp..."
sudo -v

start_bg open5gs-log sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd -f -l

start_bg nearRT-RIC bash -lc "cd '$FLEXRIC_DIR' && exec '$RIC_BIN'"
sleep 2

start_bg gnb bash -lc "cd '$(dirname "$GNB_BIN")' && exec sudo '$GNB_BIN' -c '$GNB_CONFIG'"
sleep 4

start_bg xapp bash -lc "cd '$FLEXRIC_DIR' && exec '$XAPP_BIN'"
sleep 3

if [ "$RUN_UE" = "1" ]; then
  start_bg srsue bash -lc "cd '$(dirname "$SRSUE_BIN")' && exec sudo '$SRSUE_BIN' '$SRSUE_CONFIG'"
fi

echo
echo "[INFO] Stage1 processes started."
echo "[INFO] Logs:"
echo "       $LOG_DIR"
echo
echo "[INFO] Check status with:"
echo "       $LAB/status_stage1.sh"
echo
echo "[INFO] Watch logs with examples:"
echo "       tail -f $LOG_DIR/gnb.log"
echo "       tail -f $LOG_DIR/xapp.log"
echo "       tail -f $LOG_DIR/srsue.log"
echo
echo "[INFO] Stop all with:"
echo "       $LAB/stop_stage1.sh"
