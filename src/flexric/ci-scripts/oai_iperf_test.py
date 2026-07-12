#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import subprocess
import os
import logging
import time

# =========================
# Logging Setup
# =========================

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)

log = logging.getLogger(__name__)
CYAN_BG = "\033[46m"
RESET = "\033[0m"

# =========================
# Config
# =========================

DN = "oai-ext-dn"

UES = [
    {"container": "rfsim5g-oai-nr-ue", "name": "rfsim5g_ue", "port": 5002},
    {"container": "rfsim5g-oai-nr-ue2", "name": "rfsim5g_ue2", "port": 5003},
]

LOG_DIR = "archives/oai5g-flexric"
os.makedirs(LOG_DIR, exist_ok=True)

def log_file(direction, ue_name):
    return f"{LOG_DIR}/iperf3_{direction}_{ue_name}.log"

# =========================
# Helpers
# =========================

def run(cmd):
    log.debug(f"Running command: {cmd}")
    return subprocess.check_output(cmd, shell=True, text=True).strip()

def run_bg(cmd):
    log.debug(f"Running in background: {cmd}")
    return subprocess.Popen(cmd, shell=True)

def get_ip(container, iface):
    cmd = f"docker exec {container} ip -4 -o addr show {iface} | awk '{{print $4}}' | cut -d/ -f1"
    ip = run(cmd)
    log.info(f"{container} ({iface}) IP: {ip}")
    return ip

def print_iperf_summary(direction):
    for ue in UES:
        logfile = log_file(direction, ue["name"])

        if os.path.exists(logfile) and os.path.getsize(logfile) > 0:
            with open(logfile, "r") as f:
                lines = f.readlines()

            # Filter lines
            summary = [
                line.strip()
                for line in lines
                if "receiver" in line or "sender" in line
            ][-2:]  # take last 2

            log.info(f"{CYAN_BG}iperf3 {direction.upper()} result for UE {ue['name']}{RESET}")
            for line in summary:
                log.info(f"{CYAN_BG}    {line}{RESET}")
        else:
            log.warning(f"{direction.upper()} log not found for UE {ue['name']}: {logfile}")

# =========================
# Setup
# =========================

log.info("Getting IP addresses")
subprocess.call(f"docker exec {DN} ip a show dev eth0", shell=True)
dn_ip = get_ip(DN, "eth0")

# Add IPs into UES dynamically
for ue in UES:
    ue["ip"] = get_ip(ue["container"], "oaitun_ue1")

log.info("Listing relevant containers...")
subprocess.call(
    'docker ps --filter "name=oai-ext-dn|oai-gnb|rfsim5g-oai-nr-ue|rfsim5g-oai-nr-ue2" '
    '--format "table {{.Names}}\t{{.Image}}\t{{.Status}}"',
    shell=True
)

# =========================
# Start iperf servers
# =========================

log.info("Starting iperf3 servers")

for ue in UES:
    subprocess.Popen(
        f"docker exec -d {DN} iperf3 -s -B {dn_ip} -p {ue['port']}",
        shell=True
    )

log.info("Waiting for iperf3 servers to be ready")
time.sleep(5)

# =========================
# DL Test (parallel)
# =========================

log.info("Running DL tests in parallel")

dl_processes = []

for ue in UES:
    logfile = log_file("dl", ue["name"])
    log.info(f"DL log for {ue['name']}: {logfile}")

    cmd = (
        f"docker exec {ue['container']} "
        f"iperf3 -c {dn_ip} -B {ue['ip']} -p {ue['port']} -t 20 -R "
        f"> {logfile} 2>&1"
    )

    dl_processes.append(run_bg(cmd))

# Wait for all DL
for p in dl_processes:
    p.wait()

log.info("DL tests completed.")

# =========================
# UL Test (parallel)
# =========================

log.info("Running UL tests in parallel")

ul_processes = []

for ue in UES:
    logfile = log_file("ul", ue["name"])
    log.info(f"UL log for {ue['name']}: {logfile}")

    cmd = (
        f"docker exec {ue['container']} "
        f"iperf3 -c {dn_ip} -B {ue['ip']} -p {ue['port']} -t 20 "
        f"> {logfile} 2>&1"
    )

    ul_processes.append(run_bg(cmd))

# Wait for all UL
for p in ul_processes:
    p.wait()

log.info("UL tests completed.")
log.info("iperf3 DL summary")
print_iperf_summary("dl")

log.info("iperf3 UL summary")
print_iperf_summary("ul")

# =========================
# Cleanup
# =========================

log.info("Cleaning up iperf3 servers...")
subprocess.call(f"docker exec {DN} pkill iperf3 || true", shell=True)

log.info(f"All tests completed. Logs saved in: {LOG_DIR}")
