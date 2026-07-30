# Open5GS + OCUDU + FlexRIC 第二階段：資料可視化 Demo 計畫

整理日期：2026-07-03

## 0. 本階段目標

這份 plan 只做第二階段：**把第一階段跑通的 O-RAN demo 變成看得到資料、看得到圖表、看得到流量變化的實驗平台**。

這一階段先不做自己的 xApp 演算法，也先不做自動控制。目標是：

1. 把 Prometheus 和 Grafana 加進來。
2. 讓 Open5GS 的 AMF / SMF metrics 可以被 Prometheus scrape。
3. 讓 UE namespace 裡的 `tun_srsue` 流量和 ping latency 可以進 Prometheus。
4. 用 `iperf3` 製造 UE 大流量，讓 Grafana 圖表真的會變。
5. 確認 FlexRIC / Near-RT RIC / xApp 可以取得 RAN KPM 路線。
6. 建立第三階段要做自訂 xApp 演算法前的資料基線。

第二階段完成後，你應該可以 demo：

```text
UE 開始跑 iperf3 大流量
  ↓
Grafana 看到 UE throughput 上升
  ↓
Grafana 看到 latency / packet loss 變化
  ↓
Open5GS metrics 看到 UE / session 狀態
  ↓
FlexRIC xApp 可以接 KPM 路線
```

## 1. 先釐清：Near-RT RIC 和 Prometheus 不是同一種東西

這裡很容易混亂，所以先固定概念。

| 元件 | 它做什麼 | 它不是什麼 |
| --- | --- | --- |
| FlexRIC Near-RT RIC | O-RAN E2 訂閱、控制、轉送 RAN KPM 給 xApp | 不是時序資料庫，不是 Grafana dashboard |
| xApp | 跟 RIC 訂閱 RAN 資料，之後做演算法決策 | 不是通用資料庫 |
| Prometheus | 定期 scrape HTTP `/metrics`，存成 time-series | 不會直接懂 E2 / KPM / NGAP |
| Grafana | 把 Prometheus 裡的資料畫圖 | 不負責收 E2 訊息 |

所以資料路線應該是兩條：

```text
Open5GS / UE exporter
  → HTTP /metrics
  → Prometheus
  → Grafana
```

```text
OCUDU gNB
  → E2 / KPM
  → FlexRIC Near-RT RIC
  → xApp
  → 後續再轉成 Prometheus metrics
```

第二階段先做穩第一條，並驗證第二條。第三階段再把 xApp 演算法和 KPM exporter 做漂亮。

## 2. 本階段建議收哪些資料

先收會動、容易解釋、對 demo 有意義的資料。

| 資料 | 來源 | 進 Prometheus 的方式 | 用途 |
| --- | --- | --- | --- |
| UE RX/TX bytes | `ue1` namespace 的 `tun_srsue` | 自己寫簡單 exporter | 畫 throughput |
| UE ping latency | `ue1` ping `10.45.0.1` | 自己寫簡單 exporter | 畫 latency |
| UE packet loss | `ue1` ping 結果 | 自己寫簡單 exporter | 看壅塞/丟包 |
| AMF active UE | Open5GS AMF metrics | Open5GS `/metrics` | 看 UE 是否註冊 |
| SMF session | Open5GS SMF metrics | Open5GS `/metrics` | 看 PDU session |
| RAN KPM DL/UL throughput | OCUDU E2 KPM | FlexRIC xApp | 第三階段接進 Prometheus |
| CQI / RSRP / RSRQ | OCUDU E2 KPM | FlexRIC xApp | RF / channel quality demo |

第二階段最小可交付成果：

```text
Grafana 能看到 UE throughput 和 latency 隨 iperf3 變化。
Open5GS metrics target 是 UP。
FlexRIC xApp 能看到 E2 node。
```

## 3. 建立第二階段資料夾

在 Ubuntu server 執行：

```bash
mkdir -p /home/zju/Desktop/oran-lab/monitoring/prometheus
mkdir -p /home/zju/Desktop/oran-lab/monitoring/exporters
mkdir -p /home/zju/Desktop/oran-lab/logs/stage2
```

## 4. 安裝第二階段工具

```bash
sudo apt update
sudo apt install -y docker.io docker-compose-plugin curl jq iperf3 python3 python3-pip
sudo systemctl enable --now docker
```

確認 Docker：

```bash
docker --version
docker compose version
```

如果目前使用者不能直接跑 Docker，可以先用 `sudo docker ...`，不用現在糾結權限。

## 5. 檢查 Open5GS metrics 是否已經開啟

先查 AMF / SMF metrics：

```bash
curl -s http://127.0.0.5:9090/metrics | head -40
curl -s http://127.0.0.4:9090/metrics | head -40
```

如果有看到類似：

```text
# HELP ...
# TYPE ...
ues_active ...
process_cpu_seconds_total ...
```

代表 Open5GS metrics 已經可以用。

如果沒有輸出，先檢查設定檔：

```bash
sudo grep -nA8 -B2 "metrics" /etc/open5gs/amf.yaml
sudo grep -nA8 -B2 "metrics" /etc/open5gs/smf.yaml
```

AMF 需要類似這段，放在 `amf:` 區塊下面：

```yaml
amf:
  metrics:
    server:
      - address: 127.0.0.5
        port: 9090
```

SMF 需要類似這段，放在 `smf:` 區塊下面：

```yaml
smf:
  metrics:
    server:
      - address: 127.0.0.4
        port: 9090
```

修改前先備份：

```bash
sudo cp /etc/open5gs/amf.yaml /etc/open5gs/amf.yaml.bak.stage2.$(date +%F-%H%M%S)
sudo cp /etc/open5gs/smf.yaml /etc/open5gs/smf.yaml.bak.stage2.$(date +%F-%H%M%S)
```

修改後重啟：

```bash
sudo systemctl restart open5gs-amfd open5gs-smfd
```

再測一次：

```bash
curl -s http://127.0.0.5:9090/metrics | head -40
curl -s http://127.0.0.4:9090/metrics | head -40
```

注意：Prometheus 自己預設也會用 `9090` port，所以這份 plan 後面讓 Prometheus Web UI 跑在 `9095`，避免和 Open5GS metrics 撞 port。

## 6. 建立 Prometheus 設定檔

建立：

```bash
nano /home/zju/Desktop/oran-lab/monitoring/prometheus/prometheus.yml
```

填入：

```yaml
global:
  scrape_interval: 2s
  evaluation_interval: 2s

scrape_configs:
  - job_name: "open5gs-amf"
    static_configs:
      - targets: ["127.0.0.5:9090"]

  - job_name: "open5gs-smf"
    static_configs:
      - targets: ["127.0.0.4:9090"]

  - job_name: "oran-ue-exporter"
    static_configs:
      - targets: ["127.0.0.1:9105"]

  - job_name: "oran-kpm-exporter"
    static_configs:
      - targets: ["127.0.0.1:9106"]
```

目前 `oran-kpm-exporter` 可以先是 DOWN，第三階段再接。第二階段先讓 Open5GS 和 UE exporter 起來。

## 7. 啟動 Prometheus

```bash
sudo docker rm -f oran-prometheus 2>/dev/null || true

sudo docker run -d \
  --name oran-prometheus \
  --network host \
  -v /home/zju/Desktop/oran-lab/monitoring/prometheus/prometheus.yml:/etc/prometheus/prometheus.yml:ro \
  prom/prometheus:latest \
  --config.file=/etc/prometheus/prometheus.yml \
  --web.listen-address=0.0.0.0:9095
```

確認：

```bash
sudo docker ps | grep oran-prometheus
curl -s http://127.0.0.1:9095/-/ready
```

打開：

```text
http://<server-ip>:9095/targets
```

如果在本機瀏覽器連不到 server，可以先在 server 上看：

```bash
curl -s http://127.0.0.1:9095/targets | head
```

## 8. 啟動 Grafana

```bash
sudo docker rm -f oran-grafana 2>/dev/null || true
sudo docker volume create oran-grafana-data

sudo docker run -d \
  --name oran-grafana \
  --network host \
  -e GF_SERVER_HTTP_PORT=3001 \
  -e GF_SECURITY_ADMIN_USER=admin \
  -e GF_SECURITY_ADMIN_PASSWORD=admin \
  -v oran-grafana-data:/var/lib/grafana \
  grafana/grafana-oss:latest
```

確認：

```bash
sudo docker ps | grep oran-grafana
```

打開：

```text
http://<server-ip>:3001
```

登入：

```text
user: admin
password: admin
```

進 Grafana 後新增 data source：

```text
Connections → Data sources → Add data source → Prometheus
URL: http://127.0.0.1:9095
Save & test
```

如果 Grafana 是用 host network 跑，`127.0.0.1:9095` 就是同一台 server 的 Prometheus。

## 9. 建立 UE traffic exporter

Prometheus 只能 scrape HTTP `/metrics`，但 UE 的流量在 Linux namespace 裡，所以這裡先寫一個很小的 exporter。

建立：

```bash
nano /home/zju/Desktop/oran-lab/monitoring/exporters/ue_exporter.py
```

填入：

```python
#!/usr/bin/env python3
import http.server
import os
import re
import subprocess
import time

UE_NS = os.environ.get("UE_NS", "ue1")
UE_IFACE = os.environ.get("UE_IFACE", "tun_srsue")
PING_TARGET = os.environ.get("PING_TARGET", "10.45.0.1")
LISTEN = os.environ.get("LISTEN", "127.0.0.1")
PORT = int(os.environ.get("PORT", "9105"))

last = {"rx": None, "tx": None, "time": None}

def run(cmd, timeout=2):
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True, timeout=timeout)

def ns_cat(path):
    result = run(["ip", "netns", "exec", UE_NS, "cat", path])
    if result.returncode != 0:
        return None
    try:
        return int(result.stdout.strip())
    except ValueError:
        return None

def ping_once():
    result = run(["ip", "netns", "exec", UE_NS, "ping", "-c", "1", "-W", "1", PING_TARGET])
    if result.returncode != 0:
        return -1.0, 1
    match = re.search(r"time=([0-9.]+)", result.stdout)
    if not match:
        return -1.0, 1
    return float(match.group(1)), 0

def build_metrics():
    now = time.time()
    rx = ns_cat(f"/sys/class/net/{UE_IFACE}/statistics/rx_bytes")
    tx = ns_cat(f"/sys/class/net/{UE_IFACE}/statistics/tx_bytes")
    latency_ms, ping_loss = ping_once()

    rx_rate = 0.0
    tx_rate = 0.0
    if rx is not None and tx is not None and last["time"] is not None:
        dt = max(now - last["time"], 0.001)
        rx_rate = max(rx - last["rx"], 0) * 8.0 / dt
        tx_rate = max(tx - last["tx"], 0) * 8.0 / dt

    if rx is not None and tx is not None:
        last["rx"] = rx
        last["tx"] = tx
        last["time"] = now

    labels = f'ue="{UE_NS}",iface="{UE_IFACE}"'
    lines = [
        "# HELP oran_ue_rx_bytes_total UE RX bytes on tun interface.",
        "# TYPE oran_ue_rx_bytes_total counter",
        f"oran_ue_rx_bytes_total{{{labels}}} {rx if rx is not None else 0}",
        "# HELP oran_ue_tx_bytes_total UE TX bytes on tun interface.",
        "# TYPE oran_ue_tx_bytes_total counter",
        f"oran_ue_tx_bytes_total{{{labels}}} {tx if tx is not None else 0}",
        "# HELP oran_ue_rx_bps UE RX throughput estimated by exporter.",
        "# TYPE oran_ue_rx_bps gauge",
        f"oran_ue_rx_bps{{{labels}}} {rx_rate}",
        "# HELP oran_ue_tx_bps UE TX throughput estimated by exporter.",
        "# TYPE oran_ue_tx_bps gauge",
        f"oran_ue_tx_bps{{{labels}}} {tx_rate}",
        "# HELP oran_ue_ping_latency_ms Ping latency from UE namespace.",
        "# TYPE oran_ue_ping_latency_ms gauge",
        f'oran_ue_ping_latency_ms{{ue="{UE_NS}",target="{PING_TARGET}"}} {latency_ms}',
        "# HELP oran_ue_ping_loss Ping loss indicator. 0 means success, 1 means failed.",
        "# TYPE oran_ue_ping_loss gauge",
        f'oran_ue_ping_loss{{ue="{UE_NS}",target="{PING_TARGET}"}} {ping_loss}',
        "",
    ]
    return "\n".join(lines).encode()

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/metrics":
            self.send_response(404)
            self.end_headers()
            return
        data = build_metrics()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; version=0.0.4")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, fmt, *args):
        return

if __name__ == "__main__":
    server = http.server.ThreadingHTTPServer((LISTEN, PORT), Handler)
    print(f"UE exporter listening on http://{LISTEN}:{PORT}/metrics")
    server.serve_forever()
```

給執行權限：

```bash
chmod +x /home/zju/Desktop/oran-lab/monitoring/exporters/ue_exporter.py
```

啟動 exporter：

```bash
sudo pkill -f ue_exporter.py 2>/dev/null || true

sudo -E nohup python3 /home/zju/Desktop/oran-lab/monitoring/exporters/ue_exporter.py \
  > /home/zju/Desktop/oran-lab/logs/stage2/ue_exporter.log 2>&1 &
```

測試：

```bash
curl -s http://127.0.0.1:9105/metrics | head -80
```

如果看到：

```text
oran_ue_rx_bytes_total ...
oran_ue_tx_bytes_total ...
oran_ue_ping_latency_ms ...
```

代表 UE exporter OK。

再去 Prometheus target 看：

```text
http://<server-ip>:9095/targets
```

`oran-ue-exporter` 應該要是 `UP`。

## 10. 啟動第一階段網路

如果你已經有第一階段腳本，用這個：

```bash
/home/zju/Desktop/oran-lab/start_stage1.sh
/home/zju/Desktop/oran-lab/status_stage1.sh
```

如果想手動啟動，至少需要：

```bash
# Terminal 1: Open5GS log
sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd -f -l
```

```bash
# Terminal 2: gNB
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
```

```bash
# Terminal 3: srsUE
cd /home/zju/Desktop/oran-lab/src/srsRAN_4G/build/srsue/src
sudo ./srsue /home/zju/Desktop/oran-lab/config/srsue/ue-zmq-open5gs.conf
```

確認 UE：

```bash
sudo ip netns exec ue1 ip addr show tun_srsue
sudo ip netns exec ue1 ip route
sudo ip netns exec ue1 ping -c 3 10.45.0.1
```

## 11. 製造 UE 流量

在 host 端開 `iperf3` server：

```bash
iperf3 -s -B 10.45.0.1 -p 5201
```

另一個 terminal 從 UE namespace 打流量：

```bash
sudo ip netns exec ue1 iperf3 -c 10.45.0.1 -p 5201 -t 60 -i 1
```

如果要 UDP 壓力：

```bash
sudo ip netns exec ue1 iperf3 -c 10.45.0.1 -p 5201 -u -b 10M -t 60 -i 1
```

如果要更大一點：

```bash
sudo ip netns exec ue1 iperf3 -c 10.45.0.1 -p 5201 -u -b 30M -t 60 -i 1
```

同時觀察 latency：

```bash
sudo ip netns exec ue1 ping -i 0.2 10.45.0.1
```

Grafana 會看到 `oran_ue_rx_bps` / `oran_ue_tx_bps` / `oran_ue_ping_latency_ms` 開始變動。

## 12. Grafana 建議 dashboard

先不用匯入複雜 dashboard，直接自己新增幾個 panel。

進 Grafana：

```text
Dashboards → New → New dashboard → Add visualization → Prometheus
```

### 12.1 UE throughput

PromQL：

```promql
oran_ue_rx_bps
```

```promql
oran_ue_tx_bps
```

或用 counter 自己算：

```promql
rate(oran_ue_rx_bytes_total[10s]) * 8
```

```promql
rate(oran_ue_tx_bytes_total[10s]) * 8
```

單位：

```text
bits/sec
```

### 12.2 UE latency

PromQL：

```promql
oran_ue_ping_latency_ms
```

單位：

```text
milliseconds
```

### 12.3 UE ping loss

PromQL：

```promql
oran_ue_ping_loss
```

解讀：

```text
0 = ping 成功
1 = ping 失敗
```

### 12.4 Open5GS active UE

先在 Prometheus 查有哪些 metric：

```text
http://<server-ip>:9095/graph
```

搜尋：

```promql
ues_active
```

如果有資料，Grafana panel 用：

```promql
ues_active
```

如果沒有，先用 process 指標確認 target 正常：

```promql
process_cpu_seconds_total{job="open5gs-amf"}
```

```promql
process_open_fds{job="open5gs-smf"}
```

不同 Open5GS 版本的 metrics 名稱可能會有差異，所以先用 Prometheus UI 搜尋實際存在的名稱。

## 13. FlexRIC / KPM 路線驗證

這一步不是 Grafana，這一步是確認 O-RAN 的 E2/KPM 資料路線可以用。

### 13.1 啟動 Near-RT RIC

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/ric/nearRT-RIC
```

### 13.2 啟動 OCUDU gNB 的 E2 版本

理想上要用：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs-flexric.yml
```

如果這個檔案還沒有，要從原本通的 config 複製：

```bash
cp /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml \
   /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs-flexric.yml
```

然後在 config 末端確認有：

```yaml
e2:
  enable_du_e2: true
  e2sm_kpm_enabled: true
  e2sm_rc_enabled: true
  addr: 127.0.0.1
  port: 36421
  bind_addr: 127.0.0.1

metrics:
  layers:
    enable_rlc: true
    enable_sched: true
  periodicity:
    du_report_period: 1000
```

成功時 gNB log 應該會看到類似：

```text
Connecting to NearRT-RIC on 127.0.0.1:36421
```

或 Near-RT RIC 看到：

```text
E2 SETUP-REQUEST rx
Accepting RAN function ID 2 with def = ORAN-E2SM-KPM
```

### 13.3 啟動 xApp

先列出可用 xApp：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
find ./build/examples/xApp -type f -executable | sort
```

先測 hello world：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/xApp/c/helloworld/xapp_hw
```

如果看到：

```text
Connected E2 nodes = 1
```

代表 RIC 有 E2 node。

再測 KPM：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/xApp/c/monitor/xapp_kpm_moni | tee /home/zju/Desktop/oran-lab/logs/stage2/xapp_kpm_moni.log
```

如果 KPM xApp 能看到 indication / measurement，代表：

```text
OCUDU gNB → E2 → FlexRIC → xApp
```

這條 O-RAN 監控路線成立。

注意：如果看到 `mcc 505, mnc 1`，通常是 FlexRIC emulator。你要的 OCUDU/Open5GS 這套應該會接近 `mcc 999, mnc 70` 或 config 對應的 PLMN。

## 14. 第二階段先不強迫 KPM 進 Prometheus

原因：

1. FlexRIC example xApp 通常是把 KPM 印在 console 或 log。
2. Prometheus 需要 HTTP `/metrics`。
3. 所以 KPM 要進 Grafana，中間要有一個 exporter 或自己寫 xApp，把 KPM 轉成 Prometheus format。

第二階段先做到：

```text
Prometheus/Grafana：Open5GS + UE traffic 可視化
FlexRIC/xApp：KPM 路線驗證
```

第三階段再做：

```text
FlexRIC KPM xApp → Prometheus exporter → Grafana
```

這樣比較穩，不會把第二階段做爆。

## 15. 本階段成功標準

### 15.1 Prometheus

打開：

```text
http://<server-ip>:9095/targets
```

至少看到：

```text
open5gs-amf UP
open5gs-smf UP
oran-ue-exporter UP
```

`oran-kpm-exporter` 目前可以 DOWN，第三階段再處理。

### 15.2 Grafana

打開：

```text
http://<server-ip>:3001
```

至少有三張圖：

```text
UE RX/TX throughput
UE ping latency
UE ping loss
```

跑 `iperf3` 時 throughput 圖要明顯上升。

### 15.3 Open5GS

UE attach 後 Open5GS log 有：

```text
UE SUPI[imsi-999700000000001]
PDU Session
UPF-Sessions is now 1
```

### 15.4 RIC / xApp

xApp 看到：

```text
Connected E2 nodes = 1
```

如果是 OCUDU gNB 接上，RIC log 會看到 E2 setup 和 KPM service model。

## 16. 第二階段 demo 流程

正式 demo 時照這個順序：

1. 開 Grafana dashboard。
2. 啟動 Open5GS / OCUDU gNB / srsUE。
3. 確認 UE 拿到 `10.45.0.x`。
4. 確認 Prometheus targets 是 UP。
5. 開 `iperf3 -s -B 10.45.0.1`。
6. UE namespace 跑 `iperf3`。
7. Grafana 看 throughput 上升。
8. 同時看 ping latency。
9. 啟動 FlexRIC / xApp，展示 E2 node connected。
10. 說明第三階段會把 KPM exporter 和自訂 xApp control 接上。

## 17. 常見問題

### 17.1 Grafana 沒資料

先看 Prometheus target：

```text
http://<server-ip>:9095/targets
```

如果 target DOWN，先不要看 Grafana。

### 17.2 UE exporter DOWN

查：

```bash
pgrep -af ue_exporter.py
tail -n 80 /home/zju/Desktop/oran-lab/logs/stage2/ue_exporter.log
curl -s http://127.0.0.1:9105/metrics | head
```

確認 namespace 存在：

```bash
sudo ip netns list
sudo ip netns exec ue1 ip addr show tun_srsue
```

### 17.3 Open5GS metrics DOWN

查服務：

```bash
systemctl status open5gs-amfd --no-pager
systemctl status open5gs-smfd --no-pager
```

查 metrics：

```bash
curl -v http://127.0.0.5:9090/metrics
curl -v http://127.0.0.4:9090/metrics
```

### 17.4 iperf3 不通

確認 UE 先能 ping UPF：

```bash
sudo ip netns exec ue1 ping -c 3 10.45.0.1
```

確認 host 有開 server：

```bash
ss -lnpt | grep 5201
```

### 17.5 xApp 一直說 no registered nodes

代表 RIC 起來了，但沒有 E2 node。

你要二選一：

```text
測 FlexRIC 自己：啟動 emu_agent_gnb
測真正 OCUDU：啟動 gNB 的 flexric config，不能跑 emulator
```

如果是第二階段正式 demo，建議測真正 OCUDU。

## 18. 參考資料

- Open5GS Metrics with Prometheus: https://open5gs.org/open5gs/docs/tutorial/04-metrics-prometheus/
- srsRAN Project Grafana Metrics GUI: https://docs.srsran.com/projects/project/en/latest/user_manuals/source/grafana_gui.html
- srsRAN Project NearRT-RIC and xApp: https://docs.srsran.com/projects/project/en/latest/tutorials/source/near-rt-ric/source/index.html
- srsRAN Project Configuration Reference: https://docs.srsran.com/projects/project/en/latest/user_manuals/source/config_ref.html
- Prometheus: https://prometheus.io/
- Grafana: https://grafana.com/

