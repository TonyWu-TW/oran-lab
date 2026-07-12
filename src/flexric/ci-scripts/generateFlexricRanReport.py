#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import logging
import re
import os
import argparse

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


# --- Constants ---
IPERF_LOGS = {
    1: {"description": "Run DL traffic to UE 10.0.0.2", "log_file": "iperf3_dl_rfsim5g_ue.log"},
    2: {"description": "Run DL traffic to UE 10.0.0.3", "log_file": "iperf3_dl_rfsim5g_ue2.log"},
    3: {"description": "Run UL traffic to UE 10.0.0.2", "log_file": "iperf3_ul_rfsim5g_ue.log"},
    4: {"description": "Run UL traffic to UE 10.0.0.3", "log_file": "iperf3_ul_rfsim5g_ue2.log"},
}
IPERF_OPTIONS = "-t 20"
CONTAINER_EXPECTED_TEXTS = {
    "nearRT-RIC": "The nearRT-RIC run SUCCESSFULLY",
    "default": "Test xApp run SUCCESSFULLY"
}
# Ordered list of services to check in container logs
SERVICES = [
    "xapp-rc-moni",
    "xapp-kpm-moni",
    "xapp-kpm-rc",
    "xapp-gtp-mac-rlc-pdcp-moni",
    "nearRT-RIC"
]

# --- Helper Functions ---

def read_template(template_file: str) -> tuple[str, int, int, str]:
    """Read template HTML and locate the template row for replacement."""
    logging.info(f"Reading template HTML: {template_file}")
    with open(template_file, "r", encoding="utf-8") as f:
        html = f.read()

    row_start = html.find("{{ test_index }}")
    if row_start == -1:
        logging.error("Template row with '{{ test_index }}' not found.")
        raise ValueError("Template row not found")
    row_start = html.rfind("<tr", 0, row_start)
    row_end = html.find("</tr>", row_start) + 5
    template_row = html[row_start:row_end]
    return html, row_start, row_end, template_row


def process_iperf_log(log_path: str) -> tuple[str, str, str]:
    """Reads iperf3 log and returns status_class, status_text, and info_content HTML."""

    status_class = "bg-status-fail"
    status_text = "KO"
    info_content = "<pre class='stats-data'>Service not detected</pre>"

    if os.path.exists(log_path):

        with open(log_path, "r", encoding="utf-8") as f:
            log_text = f.read()

        sender_match = re.search(
            r"\[\s*\d+\]\s+[\d\.\-]+\s+sec\s+[\d\.]+\s+\w+Bytes\s+([\d\.]+\s+\w+/sec).*sender",
            log_text
        )

        receiver_match = re.search(
            r"\[\s*\d+\]\s+[\d\.\-]+\s+sec\s+[\d\.]+\s+\w+Bytes\s+([\d\.]+\s+\w+/sec).*receiver",
            log_text
        )

        sender_bw = sender_match.group(1) if sender_match else "N/A"
        receiver_bw = receiver_match.group(1) if receiver_match else "N/A"

        if sender_match or receiver_match:
            status_class = "bg-status-ok"
            status_text = "OK"

        info_content = (
            f"<pre class='stats-data'>"
            f"Sender Throughput   : {sender_bw}\n"
            f"Receiver Throughput : {receiver_bw}"
            f"</pre>"
        )

    return status_class, status_text, info_content


def process_container_log(container: str, container_logs_dir: str, exit_code: str, message: str) -> tuple[str, str, str]:
    """Process container log and return status_class, status_text, info_html."""
    log_path = os.path.join(container_logs_dir, f"{container}.log")
    if os.path.exists(log_path):
        with open(log_path, "r", encoding="utf-8") as f:
            lines = [line.strip() for line in f.readlines() if line.strip()]
            last_line = lines[-1] if lines else "No log content found."
    else:
        last_line = "Service not detected"

    expected_text = CONTAINER_EXPECTED_TEXTS.get(container, CONTAINER_EXPECTED_TEXTS["default"])
    status_class = "bg-status-ok" if expected_text in last_line else "bg-status-fail"
    status_text = "OK" if status_class == "bg-status-ok" else "KO"

    info_html = f"{last_line}<hr>Exit Code: {exit_code}<br>Message: {message}"

    return status_class, status_text, info_html


# --- Main Function ---

def generate_report_with_info(
    template_file: str,
    output_file: str,
    log_file: str,
    container_logs_dir: str,
    job_name: str,
    build_id: str,
    build_url: str
):
    """Generates the full HTML report for iperf and container logs."""
    html, row_start, row_end, template_row = read_template(template_file)

    logging.info(f"Reading container exit status: {log_file}")
    with open(log_file, "r", encoding="utf-8") as f:
        status_log = f.read()
    # Extract all containers, exit codes, messages
    log_containers = re.findall(r"Container:\s*(\S+)", status_log)
    exit_codes = re.findall(r"Exit Code:\s*(\d+)", status_log)
    messages = re.findall(r"Message:\s*(.+?)\n\s*---", status_log, re.DOTALL)

    # Map container -> (exit_code, message)
    container_info = {c: (exit_codes[i] if i < len(exit_codes) else "N/A",
                          messages[i].strip() if i < len(messages) else "N/A")
                      for i, c in enumerate(log_containers)}

    new_rows = ""
    all_statuses = []

    for i in range(1, 10):
        row = template_row
        row = row.replace("{{ test_index }}", f"{i:03}")

        # --- Iperf Logs ---
        if i in IPERF_LOGS:
            iperf_info = IPERF_LOGS[i]
            description = iperf_info["description"]
            log_path = os.path.join(container_logs_dir, iperf_info["log_file"])
            status_class, status_text, info_content = process_iperf_log(log_path)
            all_statuses.append(status_text)

            row = row.replace("{{ description }}", description)
            row = row.replace("{{ options }}", IPERF_OPTIONS)
            row = row.replace("{{ status_class }}", status_class)
            row = row.replace("{{ status }}", status_text)
            row = row.replace("{{ info_content }}", info_content)

        # --- Container Logs ---
        elif i >= 5:
            idx = i - 5
            if idx < len(SERVICES):
                service = SERVICES[idx]
                exit_code, message = container_info.get(service, ("N/A", "Container exit code not detected"))
                status_class, status_text, info_html = process_container_log(service, container_logs_dir, exit_code, message)
                all_statuses.append(status_text)

                row = row.replace("{{ description }}", f"Log analysis for service {service}")
                row = row.replace("{{ options }}", f"Check last line in {service}.log")
                row = row.replace("{{ status_class }}", status_class)
                row = row.replace("{{ status }}", status_text)
                row = row.replace("{{ info_content }}", info_html)
            else:
                # Empty row if no more services
                row = row.replace("{{ description }}", "")
                row = row.replace("{{ options }}", "")
                row = row.replace("{{ status_class }}", "")
                row = row.replace("{{ status }}", "")
                row = row.replace("{{ info_content }}", "")

        new_rows += row + "\n"

    html = html[:row_start] + new_rows + html[row_end:]

    # --- Final Status ---
    final_status = "FAIL" if "KO" in all_statuses else "PASS"
    final_status_class = "bg-status-fail" if final_status == "FAIL" else "bg-status-ok"
    html = html.replace("{{ final_status }}", final_status)
    html = html.replace("{{ final_status_class }}", final_status_class)
    html = html.replace("JOB_NAME", job_name)
    html = html.replace("BUILD_ID", build_id)
    html = html.replace("BUILD_URL", build_url)
    logging.info(f"Writing final HTML report: {output_file}")
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(html)
    logging.info("Report generated successfully.")


# --- Entry Point ---

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate HTML report")

    parser.add_argument("--job-name", required=True)
    parser.add_argument("--build-id", required=True)
    parser.add_argument("--build-url", required=True)

    args = parser.parse_args()
    generate_report_with_info(
        template_file="./ci-scripts/flexric-ran-report.html",
        output_file="./test_results_oai_flexric_ran.html",
        log_file="./archives/oai5g-flexric/container_exit_status.log",
        container_logs_dir="./archives/oai5g-flexric",
        job_name=args.job_name,
        build_id=args.build_id,
        build_url=args.build_url
    )
