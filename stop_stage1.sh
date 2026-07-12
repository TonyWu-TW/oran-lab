#!/usr/bin/env bash
set -euo pipefail

LAB="${LAB:-/home/zju/Desktop/oran-lab}"
RUN_DIR="$LAB/run/stage1"

if [ ! -d "$RUN_DIR" ]; then
  echo "[INFO] No run dir found: $RUN_DIR"
  exit 0
fi

for pid_file in "$RUN_DIR"/*.pid; do
  [ -e "$pid_file" ] || continue
  name="$(basename "$pid_file" .pid)"
  pid="$(cat "$pid_file")"
  if kill -0 "$pid" 2>/dev/null; then
    echo "[INFO] Stopping $name pid=$pid"
    kill "$pid" 2>/dev/null || true
  else
    echo "[INFO] $name already stopped pid=$pid"
  fi
done

sleep 1

for pid_file in "$RUN_DIR"/*.pid; do
  [ -e "$pid_file" ] || continue
  name="$(basename "$pid_file" .pid)"
  pid="$(cat "$pid_file")"
  if kill -0 "$pid" 2>/dev/null; then
    echo "[WARN] Force stopping $name pid=$pid"
    kill -9 "$pid" 2>/dev/null || true
  fi
done

rm -f "$RUN_DIR"/*.pid

echo "[INFO] Stage1 stopped."
echo "[INFO] If a process still remains, inspect with:"
echo "       pgrep -af 'nearRT-RIC|/gnb|srsue|xapp_kpm'"
