#!/usr/bin/env bash
set -euo pipefail

LAB="${LAB:-/home/zju/Desktop/oran-lab}"
LOG_DIR="$LAB/logs/stage1"
RUN_DIR="$LAB/run/stage1"

echo "===== Process status ====="
for name in open5gs-log nearRT-RIC gnb xapp srsue; do
  pid_file="$RUN_DIR/$name.pid"
  if [ -f "$pid_file" ]; then
    pid="$(cat "$pid_file")"
    if kill -0 "$pid" 2>/dev/null; then
      echo "[OK]   $name running pid=$pid"
    else
      echo "[FAIL] $name pid file exists but process is not running pid=$pid"
    fi
  else
    echo "[MISS] $name pid file not found"
  fi
done

echo
echo "===== Key success messages ====="

check_log() {
  local label="$1"
  local file="$2"
  local pattern="$3"
  if [ ! -f "$file" ]; then
    echo "[MISS] $label log not found: $file"
    return
  fi
  if grep -E "$pattern" "$file" >/dev/null 2>&1; then
    echo "[OK]   $label"
    grep -E "$pattern" "$file" | tail -5
  else
    echo "[WAIT] $label not found yet"
    echo "       pattern: $pattern"
    echo "       log: $file"
  fi
  echo
}

check_log "Open5GS saw gNB / UE session" "$LOG_DIR/open5gs-log.log" "gNB-N2 accepted|UE SUPI|UPF-Sessions|SMF-Sessions"
check_log "nearRT-RIC saw E2/xApp connection" "$LOG_DIR/nearRT-RIC.log" "E2 SETUP-REQUEST rx|Registered E2 nodes|E42 SETUP"
check_log "gNB connected to AMF / started" "$LOG_DIR/gnb.log" "N2: Connection to AMF|==== gNB started|E2|RIC"
check_log "xApp subscribed / received indication" "$LOG_DIR/xapp.log" "Connected E2 nodes|Successfully subscribed|KPM ind_msg|measurement|RNTI"
check_log "srsUE attached / PDU session" "$LOG_DIR/srsue.log" "Random Access Complete|RRC Connected|PDU Session Establishment successful|IP:"

echo "===== UE network quick check ====="
if sudo ip netns exec ue1 ip addr show tun_srsue >/tmp/oran_stage1_ue_addr 2>/dev/null; then
  echo "[OK] ue1/tun_srsue exists"
  cat /tmp/oran_stage1_ue_addr | grep -E "inet "
else
  echo "[WAIT] ue1/tun_srsue not found"
fi

if sudo ip netns exec ue1 ping -c 1 -W 1 10.45.0.1 >/dev/null 2>&1; then
  echo "[OK] UE can ping UPF ogstun 10.45.0.1"
else
  echo "[WAIT] UE cannot ping 10.45.0.1 yet"
fi

if sudo ip netns exec ue1 ping -c 1 -W 1 8.8.8.8 >/dev/null 2>&1; then
  echo "[OK] UE can reach internet 8.8.8.8"
else
  echo "[WAIT] UE cannot reach internet 8.8.8.8 yet"
fi

echo
echo "===== Useful commands ====="
echo "tail -f $LOG_DIR/open5gs-log.log"
echo "tail -f $LOG_DIR/nearRT-RIC.log"
echo "tail -f $LOG_DIR/gnb.log"
echo "tail -f $LOG_DIR/xapp.log"
echo "tail -f $LOG_DIR/srsue.log"
