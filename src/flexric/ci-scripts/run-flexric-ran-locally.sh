#!/bin/bash
set -euo pipefail

# =========================================================
# USAGE
# =========================================================
#
# This script MUST be executed from the ROOT of the repository.
#
# PARAMETERS:
#
# 1) Default execution (current FlexRIC branch + RAN develop):
#    $0
#
# 2) Custom branches:
#    $0 -f <flexric-branch> -r <ran-branch>
#
# Example:
#    $0 flexric-feature ran-feature
#
# 3) Help:
#    $0 -h
# =========================================================

# -----------------------------
# Input parameters (with defaults)
# -----------------------------

# Defaults
FLEXRIC_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
RAN_BRANCH="develop"

# Parse options
while getopts ":f:r:h" opt; do
  case "$opt" in
    f)
      FLEXRIC_BRANCH="$OPTARG"
      ;;
    r)
      RAN_BRANCH="$OPTARG"
      ;;
    h)
      echo "Usage: $0 [-f flexric-branch] [-r ran-branch]"
      exit 0
      ;;
    \?)
      echo "[ERROR] Invalid option: -$OPTARG"
      exit 1
      ;;
    :)
      echo "[ERROR] Option -$OPTARG requires an argument"
      exit 1
      ;;
  esac
done

RAN_REPOSITORY="https://gitlab.eurecom.fr/oai/openairinterface5g.git"

WORKDIR="$PWD"
ARCHIVES="$WORKDIR/archives"

echo "[INFO] Using FlexRIC branch: $FLEXRIC_BRANCH"
echo "[INFO] Using RAN branch    : $RAN_BRANCH"

# Warn if detached HEAD
if [ "$FLEXRIC_BRANCH" = "HEAD" ]; then
  echo "[WARNING] Detached HEAD state detected for FlexRIC repo"
fi

# -----------------------------
# Cleanup & setup
# -----------------------------
echo "[STEP] Cleanup workspace..."
rm -rf "$ARCHIVES"
mkdir -p "$ARCHIVES" "$ARCHIVES/oai5g-flexric" "$ARCHIVES/oai-cn5g"

# -----------------------------
# Docker Compose files
# -----------------------------
CN5G_COMPOSE="./ci-scripts/oai-cn5g-resources/docker-compose.yaml"
RAN_COMPOSE="./ci-scripts/oai-5g-ran-flexric-resources/docker-compose.yml"

# -----------------------------
# Verify RAN commit
# -----------------------------
echo "[STEP] Checking RAN branch commit..."
RAN_COMMIT=$(git ls-remote "$RAN_REPOSITORY" refs/heads/"$RAN_BRANCH" | awk '{print $1}')

if [ -z "$RAN_COMMIT" ]; then
  echo "[ERROR] Could not find branch $RAN_BRANCH in RAN repo"
  exit 1
fi

echo "RAN Commit: $RAN_COMMIT"

# -----------------------------
# Prepare source code
# -----------------------------
echo "[STEP] Preparing FlexRIC source..."
git checkout "$FLEXRIC_BRANCH"
git submodule update --init --recursive

CORE_SERVICES=(
  mysql oai-nrf oai-udr oai-udm oai-ausf
  oai-amf oai-smf oai-upf oai-ext-dn
)

# -----------------------------
# Pull images
# -----------------------------

echo "[STEP] Ensuring CN5G images are up-to-date"
docker compose -f "$CN5G_COMPOSE" pull --quiet

echo "[STEP] Ensuring gNB and nrUE images are up-to-date"
docker compose -f "$RAN_COMPOSE" pull oai-gnb oai-nr-ue oai-nr-ue2 --quiet || true

# -----------------------------
# Cleanup
# -----------------------------

cleanup() {
  set +e

  echo "[CLEANUP] Disconnecting OAI NR UEs"

  docker stop rfsim5g-oai-nr-ue2 || true
  docker stop rfsim5g-oai-nr-ue || true

  local SERVICES=(
    xapp-rc-moni
    xapp-kpm-moni
    xapp-kpm-rc
    xapp-gtp-mac-rlc-pdcp-moni
    oai-nr-ue
    oai-nr-ue2
    oai-gnb
    nearRT-RIC
  )

  echo "[CLEANUP] Stopping the services and collecting logs"

  for s in "${SERVICES[@]}"; do
    docker compose -f "$RAN_COMPOSE" stop --timeout 60 "$s" || true

    sleep 5

    docker compose -f "$RAN_COMPOSE" logs "$s" --no-log-prefix \
      > "$ARCHIVES/oai5g-flexric/${s}.log" 2>&1 || true
  done

  echo "[CLEANUP] Stopping 5G Core services and collecting logs"

  docker compose -f "$CN5G_COMPOSE" stop --timeout 60

  for s in "${CORE_SERVICES[@]}"; do
    docker compose -f "$CN5G_COMPOSE" logs "$s" --no-log-prefix \
      > "$ARCHIVES/oai-cn5g/${s}.log" 2>&1 || true
  done

  echo "[CLEANUP] Removing containers and networks"

  docker compose -f "$RAN_COMPOSE" down --remove-orphans || true
  docker compose -f "$CN5G_COMPOSE" down --remove-orphans || true

  echo "[DONE] Logs saved in: $ARCHIVES"
}

trap cleanup EXIT

# -----------------------------
# Deploy core network
# -----------------------------

echo "[STEP] Deploying 5G Core"
docker compose -f "$CN5G_COMPOSE" up -d --wait "${CORE_SERVICES[@]}"
sleep 5

echo "[STATUS] 5G Core containers"
docker compose -f "$CN5G_COMPOSE" ps

# -----------------------------
# Deploy RAN + FlexRIC
# -----------------------------

echo "[STEP] Deploy nearRT-RIC"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- nearRT-RIC
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- nearRT-RIC

echo "[STEP] Deploy OAI 5G gNB in RF sim SA"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- oai-gnb
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- oai-gnb

echo "[STEP] Deploy RC Monitoring"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- xapp-rc-moni
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- xapp-rc-moni

echo "[STEP] Deploy 2 OAI 5G NR-UEs in RF sim SA"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- oai-nr-ue oai-nr-ue2
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- oai-nr-ue

echo "[STEP] Attach UEs"
docker start rfsim5g-oai-nr-ue
docker start rfsim5g-oai-nr-ue2

docker exec rfsim5g-oai-nr-ue ip a show dev oaitun_ue1
docker exec rfsim5g-oai-nr-ue2 ip a show dev oaitun_ue1

echo "[STEP] Deploy KPM Monitoring"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- xapp-kpm-moni
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- xapp-kpm-moni

echo "[STEP] Deploy KPM Monitoring and RC control"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- xapp-kpm-rc
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- xapp-kpm-rc

echo "[STEP] Deploy Custom SMs Monitoring"
docker compose -f "$RAN_COMPOSE" up -d --wait --wait-timeout 60 -- xapp-gtp-mac-rlc-pdcp-moni
sleep 5
docker compose -f "$RAN_COMPOSE" ps -- xapp-gtp-mac-rlc-pdcp-moni

# -----------------------------
# Iperf3 tests
# -----------------------------
echo "[STEP] Running iperf3 tests"

EXT_IP=$(docker exec oai-ext-dn ip -4 -o addr show eth0 | awk '{print $4}' | cut -d/ -f1)
echo "[INFO] External DN IP: $EXT_IP"
docker exec oai-ext-dn iperf3 -s -B $EXT_IP -p 5002 >> /dev/null &
docker exec oai-ext-dn iperf3 -s -B $EXT_IP -p 5003 >> /dev/null &

UE1_IP=$(docker exec rfsim5g-oai-nr-ue ip -4 -o addr show oaitun_ue1 | awk '{print $4}' | cut -d/ -f1)
UE2_IP=$(docker exec rfsim5g-oai-nr-ue2 ip -4 -o addr show oaitun_ue1 | awk '{print $4}' | cut -d/ -f1)

docker exec rfsim5g-oai-nr-ue iperf3 -B $UE1_IP -c $EXT_IP -p 5002 -t 20 -R \
    > $ARCHIVES/oai5g-flexric/iperf3_dl_ue.log 2>&1 &
PID1=$!

docker exec rfsim5g-oai-nr-ue2 iperf3 -B $UE2_IP -c $EXT_IP -p 5003 -t 20 -R \
    > $ARCHIVES/oai5g-flexric/iperf3_dl_ue2.log 2>&1 &
PID2=$!

wait $PID1
wait $PID2

docker exec rfsim5g-oai-nr-ue iperf3 -B $UE1_IP -c $EXT_IP -p 5002 -t 20 \
    > $ARCHIVES/oai5g-flexric/iperf3_ul_ue.log 2>&1 &
PID1=$!

docker exec rfsim5g-oai-nr-ue2 iperf3 -B $UE2_IP -c $EXT_IP -p 5003 -t 20 \
    > $ARCHIVES/oai5g-flexric/iperf3_ul_ue2.log 2>&1 &
PID2=$!

wait $PID1
wait $PID2

