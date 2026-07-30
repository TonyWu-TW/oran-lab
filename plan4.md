# O-RAN Experiment Manager 建置計畫（不含 xApp 演算法）

整理日期：2026-07-15

## 2026-07-16 實作進度

目前已有可直接使用的第一版，網址為 `http://127.0.0.1:8088`：

- `oran-experiment-manager.service` 與 `oran-ue-exporter.service` 已安裝、啟用。
- headless controller 已完成 preflight、start、status、stop，三 UE 可重複啟停。
- FastAPI、SQLite、Experiment/UE/Run/Event/Traffic models 與 19 個 API paths 已完成。
- React dashboard、Experiment/Channel editor、Live Run、Traffic controller 已完成。
- 每個 Run 產生 immutable gNB/UE/Broker config snapshot；AWGN、fading、delay、
  RLF、HST 與 per-UE path loss 會實際套用。
- ping、TCP/UDP iperf3 UL/DL 可由 API/UI 啟動，結果保存於 SQLite。
- Prometheus 已可分別取得 UE1–3 的 throughput、latency、loss、attach/PDU metrics。
- sudo 權限限制為三個固定 helper command，不接受任意 shell。

仍待後續階段完成：動態 1–10 UE Broker/config 產生、Open5GS subscriber UI、
Scenario presets/timeline、單 UE runtime restart、Traffic Stop All UI、完整 Alembic
migration、Run 結果匯出與更完整 failure-injection/hardening 測試。

## 0. 計畫定位

這份計畫不再以 `Stage 1 / Stage 2 / Stage 3` 當作使用者操作概念。
前面的 plan 只是環境建置歷程；最終使用者面對的是一個完整的
**O-RAN Experiment Manager**。

使用者的標準流程應該是：

```text
建立實驗
  → 設定 gNB、UE 數量、無線與 channel 情境
  → 驗證設定
  → 一鍵啟動整套系統
  → 控制各 UE 產生流量
  → 即時監控系統與 UE
  → 停止實驗
  → 保存結果供後續比較
```

本階段只把實驗基礎設施、控制台、監控與結果保存做好，讓後續 xApp
演算法可以直接使用同一套實驗流程。

## 1. 本階段目標

完成後，使用者應能從瀏覽器完成：

1. 查看 Open5GS、MongoDB、FlexRIC、gNB、Broker、UE、Prometheus、Grafana
   的即時狀態。
2. 建立、複製、修改與刪除實驗設定。
3. 在啟動前選擇 UE 數量並設定每台 UE。
4. 自動配置 IMSI、IMEI、ZMQ ports、namespace、log 與 Open5GS subscriber。
5. 設定每台 UE 的 path loss、AWGN、fading、delay、RLF 與 HST。
6. 執行 preflight，阻止重複 IMSI、port 衝突、sample-rate 不一致等錯誤。
7. 按正確依賴順序啟動或停止整個實驗。
8. 單獨啟動、停止或重新啟動預先配置好的 UE。
9. 控制 UE 執行 ping、TCP/UDP iperf3 與排程流量。
10. 在網頁查看 throughput、latency、packet loss、jitter、UE attach、IP、RNTI、
    CPU、RAM 與元件健康狀態。
11. 保存 config snapshot、事件、log 索引、traffic 結果與 Prometheus 查詢時間範圍。
12. 在實驗結束後重新載入該次實驗的設定與結果。

## 2. 本階段明確不做

- 不開發 xApp 演算法。
- 不實作 E2SM-RC closed-loop control。
- 不宣稱可以控制 PRB、scheduler、slice 或 QoS。
- 不修改 FlexRIC/OCUDU 的 E2 control API。
- 不做模型訓練、AI router 或多專家演算法。
- 不做 Kubernetes 或多主機叢集。
- 不做公開網際網路 SaaS。
- 不允許前端輸入任意 shell command。
- 不讓 Web API 直接以 unrestricted root 身分執行。

可以保留既有 FlexRIC/KPM 元件的啟停與健康狀態，但這一階段不把 KPM
控制邏輯納入完成標準。

## 3. 現有基線

目前已驗證的環境：

| 元件 | 現況 |
| --- | --- |
| Open5GS | systemd 服務可用 |
| MongoDB | Docker container `open5gs-mongodb` 可用 |
| OCUDU gNB | Band 3、20 MHz、ZMQ、Open5GS、E2 可用 |
| GNU Radio Broker | 三 UE flowgraph 可用 |
| srsUE | 三台 UE 可同時 attach 與建立 PDU Session |
| UE namespace | `ue1`、`ue2`、`ue3` 已驗證 |
| Prometheus | `9095` 已部署 |
| Grafana | `3001` 已部署 |
| Open5GS metrics | AMF/SMF 已進 Prometheus |
| UE exporter | 現有版本主要服務單一 `ue1`，需重構為多 UE |
| KPM exporter | 尚未完成，不列入本階段必要項目 |

現有成功 config 必須保留，Experiment Manager 只能從 template 產生新檔，
不能直接覆蓋：

```text
config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
config/ocudu/gnb-fdd-srsue-zmq-open5gs-multiue.yml
config/srsue/ue-zmq-open5gs.conf
config/srsue/multiue/ue1.conf
config/srsue/multiue/ue2.conf
config/srsue/multiue/ue3.conf
radio/broker/upstream/multi_ue_scenario.grc
```

## 4. 名詞與使用者模型

### 4.1 Experiment Definition

尚未執行的實驗設定，包含：

- gNB profile
- UE profiles
- channel profiles
- traffic jobs
- monitoring options
- 元件是否啟動

### 4.2 Experiment Run

一次實際執行。即使使用同一份 Experiment Definition，每次執行都會有新的
Run ID、開始時間、config snapshot、IP 分配、事件與結果。

### 4.3 UE Slot

一個可被啟用的 UE 位置，固定管理：

- IMSI/IMEI
- ZMQ RX/TX ports
- namespace
- subscriber
- srsUE config
- channel profile
- traffic jobs

### 4.4 Scenario

一組可重複使用的 channel 與 traffic 預設，不等同於 xApp 演算法。

## 5. 目標架構

```text
Browser
  │ REST + WebSocket
  ▼
Experiment Manager API
  ├─ Experiment/UE/Scenario CRUD
  ├─ Validation / Preflight
  ├─ Runtime state machine
  ├─ Traffic controller
  ├─ Monitoring query adapter
  └─ Audit/event recorder
       │
       ├─ SQLite（控制台自己的資料）
       ├─ Open5GS MongoDB adapter（subscriber only）
       ├─ Prometheus HTTP API（metrics only）
       └─ Privileged orchestration adapter
             ├─ systemd units
             ├─ network namespaces / routes / tc
             ├─ Open5GS
             ├─ FlexRIC
             ├─ OCUDU gNB
             ├─ GNU Radio Broker
             └─ srsUE 1...N
```

必須分開三種資料：

| 資料 | 儲存位置 |
| --- | --- |
| Open5GS subscriber/SIM | Open5GS MongoDB |
| 實驗定義、UE profile、事件、結果索引 | Experiment Manager SQLite |
| 時序監控資料 | Prometheus |

Experiment Manager 不直接把自己的資料塞進 Open5GS MongoDB。

## 6. 建議技術選型

第一版採單機、單使用者優先：

| 區塊 | 選型 |
| --- | --- |
| Backend API | Python FastAPI |
| Validation | Pydantic models + 自訂 cross-field validator |
| ORM/migration | SQLAlchemy + Alembic |
| 控制台資料庫 | SQLite |
| Frontend | React + TypeScript + Vite |
| 即時狀態 | WebSocket 或 Server-Sent Events |
| Process supervisor | systemd units/template units |
| Metrics | 現有 Prometheus |
| 圖表 | 前端查 Prometheus HTTP API；Grafana 保留進階分析 |
| 測試 | pytest + frontend component/e2e tests |

理由：

- 現有 exporter 與管理工具已是 Python，後端整合成本較低。
- FastAPI 可自動產生 OpenAPI schema，方便前後端對接。
- Vite 官方提供 React TypeScript template。
- SQLite 足以處理目前單機實驗定義與事件；未來多人同時使用才遷移 PostgreSQL。
- systemd 負責啟動、停止、重啟與程序生命週期，API 不自己發明 process supervisor。
- Prometheus 已提供 `/api/v1/query` 與 `/api/v1/query_range`，前端不需要自己讀 TSDB。

## 7. 預期新增目錄

```text
oran-lab/
├─ experiment-manager/
│  ├─ backend/
│  │  ├─ app/
│  │  │  ├─ api/
│  │  │  ├─ models/
│  │  │  ├─ schemas/
│  │  │  ├─ services/
│  │  │  ├─ adapters/
│  │  │  │  ├─ open5gs.py
│  │  │  │  ├─ prometheus.py
│  │  │  │  ├─ systemd.py
│  │  │  │  └─ network.py
│  │  │  └─ main.py
│  │  ├─ migrations/
│  │  └─ tests/
│  ├─ frontend/
│  │  ├─ src/
│  │  │  ├─ pages/
│  │  │  ├─ components/
│  │  │  ├─ api/
│  │  │  └─ types/
│  │  └─ tests/
│  ├─ templates/
│  │  ├─ gnb/
│  │  ├─ ue/
│  │  ├─ broker/
│  │  └─ prometheus/
│  └─ systemd/
├─ experiments/
│  ├─ definitions/
│  └─ runs/
│     └─ <run-id>/
│        ├─ manifest.json
│        ├─ configs/
│        ├─ logs/
│        ├─ traffic/
│        └─ result.json
└─ monitoring/
   └─ exporters/
      └─ multi_ue_exporter.py
```

## 8. 資料模型

### 8.1 Experiment

- id
- name
- description
- created_at / updated_at
- gNB profile
- expected UE count
- broker capacity
- monitoring enabled
- selected scenario
- revision

### 8.2 UEProfile

- id / experiment_id
- display name
- enabled
- IMSI
- IMEI
- SIM credential profile reference
- APN/DNN
- SST/SD
- RX/TX ports
- namespace
- requested static IP（可選）
- channel profile
- path-loss dB
- traffic defaults

K/OPc 不直接回傳給一般 frontend；UI 只選 credential profile。

### 8.3 ChannelProfile

DL 與 UL 分開保存：

- channel enabled
- AWGN enabled / SNR / signal power
- fading enabled / model
- delay enabled / min/max/period/init time
- RLF enabled / on/off time
- HST enabled / Doppler/period/init time
- Broker path loss

Validation 必須以本地 srsUE parser 支援的欄位為準。特別注意目前範例註解中的
UL `n0` 與實際 parser 的 `snr`/`signal_power` 不一致，不能照抄註解產生設定。

### 8.4 TrafficJob

- UE ID
- direction（UL/DL）
- protocol（TCP/UDP/ping）
- target/server
- port
- bitrate
- duration
- packet length
- start offset
- parallel streams
- status/result

### 8.5 ExperimentRun

- run_id
- experiment revision
- state
- started_at / stopped_at
- generated config paths
- component status
- allocated UE IPs/RNTIs
- Prometheus start/end time
- result summary

### 8.6 RuntimeEvent

- timestamp
- run_id
- component
- severity
- event type
- message
- structured details

## 9. 實驗狀態機

```text
DRAFT
  → VALIDATING
  → READY
  → STARTING
  → RUNNING
  → STOPPING
  → STOPPED
```

錯誤狀態：

```text
VALIDATION_FAILED
START_FAILED
DEGRADED
STOP_FAILED
```

規則：

- 同一時間只允許一個 radio experiment 使用 `2000/2001` 與 E2/N2 資源。
- `STARTING` 時拒絕第二次 start。
- 任何步驟失敗時，後端記錄已啟動元件並反向回滾。
- 不以固定 sleep 當作唯一健康判斷。
- UI 顯示目前等待的健康條件和 timeout。

## 10. Preflight 與設定驗證

每次啟動前至少檢查：

### 10.1 系統

- Open5GS services active
- MongoDB container reachable
- gNB/srsUE/FlexRIC/GNU Radio binaries 存在
- GNU Radio 使用 `/usr/bin/python3`
- 磁碟空間足夠
- CPU governor/系統需求
- `ogstun` 存在

### 10.2 身份與 Core

- IMSI/IMEI 唯一
- subscriber 與 UE config 一致
- APN/DNN、SST/SD、PLMN、TAC 一致
- 不把 K/OPc 寫入 event/log/API response

### 10.3 Radio

- gNB、Broker、所有 UE sample rate 一致
- bandwidth/PRB/SCS 組合合法
- Broker branch 數足夠
- path-loss/channel 數值在允許範圍

### 10.4 Runtime resources

- ZMQ ports 沒被占用
- namespace 不重複
- 沒有其他 oran-lab 實驗程序
- log/PID 目錄可寫
- Prometheus/Grafana port 沒衝突

Preflight 回傳結構化結果：

```json
{
  "ok": false,
  "checks": [
    {"id": "ports", "status": "pass"},
    {"id": "subscriber-ue3", "status": "fail", "message": "subscriber missing"}
  ]
}
```

## 11. UE 與 Open5GS Subscriber 管理

### 11.1 新增 UE

後端以 transaction-like workflow 執行：

1. 鎖定 Experiment revision。
2. 分配下一個 UE index。
3. 分配 IMSI/IMEI。
4. 分配 RX/TX ports。
5. 建立 namespace 名稱與 log 名稱。
6. 驗證 Broker capacity。
7. 產生 UE config preview。
8. 寫入 Experiment database。
9. 在使用者確認或啟動實驗時 provision Open5GS subscriber。

失敗時不得留下半筆 subscriber 或半份 config。

### 11.2 刪除 UE

- RUNNING 中不得直接刪除已啟動 UE。
- 必須先停止 UE。
- UI 明確詢問是否也刪除 Open5GS subscriber。
- 實驗歷史 snapshot 不受刪除影響。

### 11.3 啟動/停止 UE

- 只有已存在 Broker branch 的 UE 才能在 RUNNING 中啟動。
- 停止 UE 後要確認 process、TUN 與 session 狀態。
- Broker 在某個 UE 未啟動時是否會阻塞，需要在實作前做專門測試。
- 如果目前 Broker 行為無法容忍缺少 branch，第一版只允許整套協調重啟。

## 12. 可變 UE 數量與 Broker 策略

目前 `.grc` 固定三個 UE branch，因此不能只靠新增 `ue4.conf` 動態增加 UE4。

第一版採「啟動前決定 UE 數量」：

1. 保留官方 `.grc` 作為參考，不在 runtime 直接編輯 GUI 檔。
2. 建立可由 UE profile list 產生的 GNU Radio Python Broker template/generator。
3. 啟動前依 N 台 UE 產生對應 Broker Python。
4. 產生後用 `/usr/bin/python3 -m py_compile` 驗證。
5. Broker 必須最後啟動。

Port 規則：

```text
gNB: 2000/2001
UE1: 2100/2101
UE2: 2200/2201
...
UE10: 3000/3001
```

第一版上限先設 10 UE，但完成標準仍以 3 UE 穩定為主。UI 必須顯示：

- 啟動前可自由改 UE 數量。
- RUNNING 中新增超出 Broker capacity 的 UE，需要重新啟動 radio stack。

## 13. Channel 與 Scenario 管理

### 13.1 可設定 channel

- Broker per-UE path loss
- DL/UL AWGN
- DL/UL fading
- DL/UL delay
- DL/UL RLF cycle
- DL/UL HST Doppler

大部分 srsUE channel config 是啟動時讀取，第一版套用變更時應顯示：

```text
需要重新啟動 UE
是否影響其他 UE
是否需要重啟 Broker/gNB
```

### 13.2 內建 Scenario Presets

- Clean channel
- Cell edge
- Urban multipath
- High-speed train
- Tunnel/RLF
- Periodic delay
- Asymmetric UL/DL impairment
- 三台 UE 不同 path loss
- UE 逐漸遠離基地台（後續 runtime 控制）

### 13.3 IP-layer impairment

另外提供 Linux `tc netem` profile：

- delay/jitter
- loss/burst loss
- reorder/duplicate
- rate limit

UI 必須明確區分：

- RF/channel impairment：可能影響 cell search、RRC、PDU Session。
- IP impairment：主要影響應用流量，不等同於 RF noise。

## 14. 啟動與停止編排

### 14.1 啟動順序

```text
1. 鎖定 experiment/run
2. 保存 config snapshot
3. 建立/確認 subscribers
4. 建立 namespaces、route、NAT/tc
5. 確認 Open5GS/MongoDB
6. 啟動 Near-RT RIC（若該實驗需要）
7. 啟動 gNB
8. 等待 N2，並視設定等待 E2
9. 啟動所有 enabled UE
10. 啟動 GNU Radio Broker
11. 等待每台 UE attach/PDU Session/TUN/IP
12. 啟動 exporter
13. Run 狀態改為 RUNNING
```

### 14.2 停止順序

```text
1. 停止 traffic jobs
2. 停止 exporter/非必要 monitor
3. 停止 Broker
4. 反向停止 UE
5. 停止 gNB
6. 停止 RIC
7. 清理 tc rules、PID/FIFO
8. 保留 namespace 或依設定清理
9. 確認 ports/process 無殘留
10. 保存 result summary
```

Open5GS、MongoDB、Prometheus、Grafana 可設定為平台常駐服務，不必每次實驗停止。

## 15. Process 與權限管理

正式版本不能沿用網頁後端直接 `sudo bash ...`。

採用：

- `oranlab-api` 專用 Linux user。
- systemd 管理 API 本身。
- gNB、Broker、RIC 使用獨立 service units。
- UE 使用 `oranlab-ue@.service` template unit。
- systemd unit 讀取當次 Run 產生的 immutable config path。
- API 透過受限制的 polkit/systemd D-Bus 或精確 sudoers allowlist 控制 units。
- 網路 namespace/tc 操作集中在固定 helper，不接受任意命令字串。

所有 privileged action 都要：

- 有 request/run ID
- 驗證 allowlist
- 寫 audit event
- 有 timeout
- 可判斷成功/失敗

## 16. Traffic Controller

### 16.1 支援操作

- ping test
- TCP UL/DL iperf3
- UDP UL/DL iperf3
- 單台啟動
- 多台同步啟動
- 定時啟動
- 停止指定 job
- 停止全部 traffic

### 16.2 iperf server 管理

本機 iperf3 server 一次只接受一個測試，因此同步多 UE 測試要分配獨立 server
ports，例如 `5201/5202/5203`，不能讓三個 client 同時搶同一 instance。

### 16.3 Traffic 結果

至少解析：

- sender/receiver bitrate
- bytes
- retransmission
- jitter
- lost/total datagrams
- packet-loss percentage
- exit code
- timeout/error

優先使用 iperf3 JSON output，不以人類文字 log regex 作為唯一資料來源。

## 17. Monitoring 計畫

### 17.1 保留現有 Prometheus/Grafana

- Prometheus 仍跑在 `9095`。
- Grafana 仍跑在 `3001`。
- Open5GS AMF/SMF metrics 保留。

### 17.2 重構多 UE exporter

現有 exporter 改成動態讀取 enabled UE list，輸出帶 label 的 metrics：

```text
oran_ue_rx_bytes_total{run_id="...",ue="ue1"}
oran_ue_tx_bytes_total{run_id="...",ue="ue1"}
oran_ue_rx_bps{run_id="...",ue="ue1"}
oran_ue_tx_bps{run_id="...",ue="ue1"}
oran_ue_ping_latency_ms{run_id="...",ue="ue1"}
oran_ue_ping_loss_percent{run_id="...",ue="ue1"}
oran_ue_attached{run_id="...",ue="ue1"}
oran_ue_pdu_session_up{run_id="...",ue="ue1"}
```

避免把 IMSI、K、OPc 等敏感/高 cardinality 資料放進 Prometheus labels。

### 17.3 元件健康 metrics

- component process up
- N2 connected
- E2 connected（只做狀態）
- Broker ZMQ connection count
- UE RRC/PDU/TUN status
- experiment state
- active traffic job count
- CPU/RAM/disk
- gNB/UE RF underflow/late/error count

### 17.4 前端圖表

前端透過 backend proxy 查 Prometheus `/api/v1/query` 與
`/api/v1/query_range`，不要把 Prometheus 任意 query endpoint 直接暴露給外網。

預設圖表：

- 每 UE DL/UL throughput
- latency/loss/jitter
- UE IP/RNTI/RRC/PDU 狀態
- traffic job timeline
- 系統 CPU/RAM
- component health timeline

Grafana 用於更深入探索，Experiment Manager 顯示主要實驗圖表。

## 18. Backend API 草案

```text
GET    /api/health
GET    /api/platform/status

GET    /api/experiments
POST   /api/experiments
GET    /api/experiments/{id}
PATCH  /api/experiments/{id}
DELETE /api/experiments/{id}
POST   /api/experiments/{id}/clone
POST   /api/experiments/{id}/validate

GET    /api/experiments/{id}/ues
POST   /api/experiments/{id}/ues
PATCH  /api/experiments/{id}/ues/{ue_id}
DELETE /api/experiments/{id}/ues/{ue_id}

POST   /api/experiments/{id}/runs
GET    /api/runs/{run_id}
POST   /api/runs/{run_id}/stop
GET    /api/runs/{run_id}/events
GET    /api/runs/{run_id}/components

POST   /api/runs/{run_id}/ues/{ue_id}/start
POST   /api/runs/{run_id}/ues/{ue_id}/stop
POST   /api/runs/{run_id}/traffic
DELETE /api/runs/{run_id}/traffic/{job_id}

GET    /api/runs/{run_id}/metrics/query
GET    /api/runs/{run_id}/metrics/range
```

長時間 start/stop 不在 HTTP request 裡同步阻塞；API 回傳 operation ID，frontend
透過 WebSocket/SSE 收事件與進度。

## 19. Frontend 頁面

### 19.1 Platform Overview

- 平台拓撲
- 常駐服務狀態
- 是否有 active experiment
- CPU/RAM/disk
- 快速進入當前 Run

### 19.2 Experiment List

- 建立
- 複製
- 修改
- 驗證
- 啟動
- 查看歷史 Run

### 19.3 Experiment Editor

分步 wizard：

1. General/gNB
2. UE 數量與身份
3. Channel/scenario
4. Traffic defaults
5. Monitoring
6. Review/preflight

### 19.4 Live Run

- 元件狀態與啟動時間軸
- UE table
- traffic controls
- 即時圖表
- log/events
- Stop Experiment

### 19.5 UE Detail

- attach/RRC/PDU/TUN/IP/RNTI
- channel 設定
- traffic jobs
- throughput/latency/loss
- start/stop/restart

### 19.6 Results

- config snapshot
- traffic summary
- metrics 時間範圍
- error/events
- 匯出 JSON/CSV

## 20. 安全與資料保護

- 第一版預設只監聽 localhost 或實驗室管理網段。
- 若開放遠端，必須登入、HTTPS、CSRF/CORS 限制。
- 不把 sudo password 存在 UI、database 或 `.env`。
- 不提供 shell terminal 功能。
- K/OPc API response 一律遮罩。
- Subscriber CRUD 寫 audit log，但 audit 不包含秘密值。
- 所有 config update 使用 schema allowlist。
- 前端不能指定任意 filesystem path。
- Run config 產物只能寫入指定 experiments/runs root。
- Stop/delete 等破壞性操作需要確認與 idempotency key。

## 21. Log 與磁碟管理

現有實驗已發生數 GB log 持續成長，因此本階段必須處理：

- 每個 Run 獨立 log 目錄
- log rotation/max size
- PCAP 預設關閉，使用者明確啟用才收
- PCAP size limit
- 保留天數
- UI 顯示 Run 佔用空間
- 刪除 Run 前確認
- 磁碟低於門檻時拒絕啟動新 Run

## 22. 實作階段

### Phase 0：凍結基線與需求測試

- [ ] 保存目前三 UE 成功 Run 的 config/log 摘要。
- [ ] 完成安全 stop，確認沒有殘留 ports/process。
- [ ] 驗證單 UE baseline 可恢復。
- [ ] 測試 Broker 缺少某個 UE branch 時是否阻塞。
- [ ] 測試 UE 單獨 restart 對其他 UE 的影響。
- [ ] 記錄 systemd、namespace、Docker、Open5GS 實際依賴。

### Phase 1：先做可靠的 headless control layer

- [ ] 將手動啟停轉為可重複的 systemd/helper 操作。
- [ ] start/stop/status 回傳結構化 JSON。
- [ ] 完成 PID、process path、port、N2/E2、UE attach health checks。
- [ ] 完成反向 stop 與殘留清理。
- [ ] 連續啟停三次三 UE 都成功。

這一階段沒有 Web UI；若 control layer 不可靠，不能繼續包網頁。

### Phase 2：Backend 與資料模型

- [ ] FastAPI 專案骨架。
- [ ] SQLite/Alembic。
- [ ] Experiment/UE/Scenario/Run/Event models。
- [ ] config template renderer。
- [ ] validation/preflight service。
- [ ] Open5GS subscriber adapter。
- [ ] systemd/network/Prometheus adapters。
- [ ] API unit/integration tests。

### Phase 3：唯讀 Dashboard

- [ ] React TypeScript frontend。
- [ ] Platform Overview。
- [ ] Active Run component/UE status。
- [ ] Prometheus charts。
- [ ] event/log viewer。
- [ ] 尚不提供 start/stop mutation。

### Phase 4：Experiment Editor 與 UE 管理

- [ ] Experiment wizard。
- [ ] UE add/edit/delete。
- [ ] IMSI/IMEI/port allocator。
- [ ] Channel profile editor。
- [ ] Scenario presets。
- [ ] config preview/diff。
- [ ] Preflight UI。

### Phase 5：安全啟停

- [ ] Start operation/state machine。
- [ ] Stop/rollback。
- [ ] WebSocket/SSE progress。
- [ ] 防止重複 start。
- [ ] timeout/error recovery。
- [ ] UI 顯示真正 READY 條件。

### Phase 6：Traffic Controller

- [ ] ping/TCP/UDP jobs。
- [ ] per-UE unique iperf server ports。
- [ ] 同步排程。
- [ ] JSON result parser。
- [ ] Stop traffic/all traffic。
- [ ] 結果圖表與匯出。

### Phase 7：多 UE Monitoring

- [ ] 重構 UE exporter。
- [ ] Prometheus labels 加 run_id/ue。
- [ ] component health exporter。
- [ ] Grafana provisioning dashboard。
- [ ] frontend Prometheus proxy/charts。
- [ ] log/PCAP rotation。

### Phase 8：Scenario 與實驗結果

- [ ] RF channel presets。
- [ ] IP `tc netem` presets。
- [ ] 可重複 traffic timeline。
- [ ] config snapshot。
- [ ] Run result summary。
- [ ] JSON/CSV export。

### Phase 9：Hardening 與交付

- [ ] 權限/allowlist/audit review。
- [ ] 斷線、程序 crash、port 衝突測試。
- [ ] MongoDB/Prometheus 暫時不可用測試。
- [ ] 磁碟滿與 log rotation 測試。
- [ ] 使用者操作文件。
- [ ] 備份與還原文件。

## 23. 驗收情境

### 23.1 Clean 3 UE Run

1. UI 建立三 UE。
2. Preflight 全部通過。
3. 一鍵啟動。
4. 三台取得不同 IP/RNTI。
5. 三台 ping UPF/Internet。
6. 三台分別執行 iperf3。
7. UI/Prometheus 顯示各 UE 指標。
8. 一鍵停止且無殘留。

### 23.2 Subscriber 缺失

- 啟動前發現 subscriber 缺失並可安全 provision。
- 不允許帶著半完成 subscriber 進入 RUNNING。

### 23.3 Port 衝突

- 預先占用 UE port。
- Preflight 應清楚指出 PID/port，不得啟動部分元件。

### 23.4 Channel Scenario

- UE2 套用較大 path loss。
- UE3 啟用 fading/delay profile。
- UI 顯示需重啟範圍。
- 實際 Run snapshot 可重現。

### 23.5 Concurrent Traffic

- 三台使用不同 iperf server ports 同時發送 UDP。
- UI 顯示每台 bitrate/loss/jitter。
- Stop All 可終止全部 traffic，但不停止 radio experiment。

### 23.6 Failure/Rollback

- 模擬 gNB 啟動失敗。
- Run 進入 START_FAILED。
- 已啟動的 RIC/namespace/helper 能安全回滾。
- 下一次 start 不受殘留影響。

## 24. 本階段完成標準

- [ ] 使用者不需要手動開七個 terminal。
- [ ] 使用者不需要手動編輯 UE/gNB/Broker config。
- [ ] 可在啟動前設定 UE 數量與每台 UE channel。
- [ ] 可安全管理 Open5GS subscribers。
- [ ] Preflight 能阻止已知 port/identity/sample-rate 問題。
- [ ] 一鍵啟停三 UE 實驗可連續成功三次。
- [ ] 可單獨控制預先配置的 UE。
- [ ] 可從 UI 控制每台 UE 流量。
- [ ] Prometheus 能區分每台 UE。
- [ ] UI 可看平台、元件、UE、traffic 與 metrics 狀態。
- [ ] 每次 Run 有 immutable config snapshot 與結果。
- [ ] Stop 後沒有殘留程序、traffic job 或 ZMQ ports。
- [ ] 密碼、K、OPc 不出現在 log/API response。
- [ ] xApp 演算法與 E2SM-RC control 沒有混入本階段。

## 25. 為未來 xApp 預留但不實作

本階段資料模型可預留：

- xApp component status
- KPM input reference
- decision event
- control command/ack event
- baseline/control experiment tag

但所有欄位先保持 inactive。等 Experiment Manager 完成，再另開計畫盤點
OCUDU + FlexRIC 真正支援的 E2SM-RC actions，之後才開始老師要求的 xApp
演算法研究。

## 26. 參考資料

- FastAPI Deployment Concepts：
  https://fastapi.tiangolo.com/deployment/concepts/
- Vite Getting Started（React/TypeScript templates）：
  https://vite.dev/guide/
- Prometheus HTTP API：
  https://prometheus.io/docs/prometheus/latest/querying/api/
- Prometheus Querying Basics：
  https://prometheus.io/docs/prometheus/latest/querying/basics/
- 本地 srsUE channel parser：
  `src/srsRAN_4G/srsue/src/main.cc`
- 本地 Plan 3 多 UE 基線：
  `plan3.md`
