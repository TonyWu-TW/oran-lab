#!/usr/bin/env bash
set -euo pipefail

LAB="${ORAN_LAB_ROOT:-/home/zju/Desktop/oran-lab}"
BUILD="$LAB/src/flexric/build/examples/xApp/c/e2_school"
CONF="$LAB/src/flexric/flexric.conf"

cmd="${1:-hello}"
case "$cmd" in
  hello) bin="$BUILD/e2_hello" ;;
  kpm) bin="$BUILD/e2_kpm_sub" ;;
  rc) bin="$BUILD/e2_rc_ctrl" ;;
  *)
    echo "usage: $0 {hello|kpm|rc}"
    exit 2
    ;;
esac

if [[ ! -x "$bin" ]]; then
  echo "missing $bin"
  echo "build first:"
  echo "  cd $LAB/src/flexric/build && cmake .. && cmake --build . --target e2_hello e2_kpm_sub e2_rc_ctrl"
  exit 1
fi

exec "$bin" -c "$CONF"
