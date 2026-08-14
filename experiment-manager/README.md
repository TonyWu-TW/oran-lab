# O-RAN Experiment Manager

本機實驗控制台，管理已驗收的 Near-RT RIC、gNB、GNU Radio Broker、三台
srsUE、Open5GS、Prometheus 與 Grafana。xApp 演算法與 E2 control 不在此版本。

## 使用

正式服務安裝後開啟：

```text
http://127.0.0.1:8088
http://10.106.133.244:8088
```

API 文件：

```text
http://127.0.0.1:8088/docs
http://10.106.133.244:8088/docs
```

常用命令：

```bash
systemctl status oran-experiment-manager
sudo systemctl restart oran-experiment-manager
journalctl -u oran-experiment-manager -f
```

底層控制器也可獨立使用：

```bash
./scripts/oranlabctl.py status --json
./scripts/oranlabctl.py preflight --json
sudo ./scripts/oranlabctl.py start --json
sudo ./scripts/oranlabctl.py stop --json
```

目前 UI 的啟動按鈕固定使用已驗收的三 UE topology。Experiment/UE/channel
設定保存於 SQLite；每次 Run 都會產生不可變的 gNB、UE 與 Broker config
snapshot，AWGN、fading、delay、RLF、HST 與 per-UE path loss 會在該次啟動套用。
動態 1–10 UE Broker 完成前，不會把未驗收的 UE 數量送到底層啟動。

每台 UE 的 Traffic Profile 與 channel 一起保存在 Experiment definition，啟動時
寫入不可變的 Run snapshot。Live 頁可單獨啟動 UE 或批次啟動全部 UE，支援指定
時間或持續執行到手動停止。目前 runner 支援 ping、TCP/UDP iperf3、HTTP 上下載、
短影片、社群瀏覽、導航與雙向 RTP-like 語音；結果會依場景保存 throughput、request
成功率、P95 latency、packet loss、jitter 與 RTT。短影片可設定 Target Offered
Load、Fixed/Wave/Random Burst/Adaptive pattern、波動百分比、peak、seed 與 pacing
interval。Live 圖表用虛線顯示 Offered Load、實線顯示 Prometheus 實測 Delivered
Throughput。多 UE exporter 由
`oran-ue-exporter.service` 管理，Prometheus 以 `run_id`、`ue` label 區分指標。

RTP-like voice runner 使用獨立 sender/receiver：sender 固定依 packet interval 發包，
不會因等待 echo 而降低 Offered bitrate；每秒回報 Offered/Received bitrate、三秒
rolling loss、delivery ratio、jitter、平均 RTT 與 RTT P95。Live 的 UE9/UE10 Voice
Quality 圖會取目前通話 UE 的最差品質，並把 Offered、Received 與
loss/jitter/RTT 分開呈現；三 UE snapshot 則顯示 UE3。

Live 頁也提供 Python `VoiceGuard` xApp 的啟停、Observe Only / Closed Loop 切換與
狀態機畫面。Rule V1 由 `xapps/voiceguard/voiceguard.py` 執行；Random Forest V2
由共享 runtime 執行；三 UE snapshot 會自動選擇
`xapps/voiceguard_rf_3ue/voiceguard_rf_3ue.py` 與對應模型。兩者都透過 Manager、
Prometheus 與 RTP progress 讀取 UE 指標。FlexRIC 這版沒有標準 RC Python binding，
所以 `voiceguard_rc` 原生 C bridge 負責送出 E2SM-RC Style 2 / Action 6，啟動
RF Closed Loop 時會先對該 topology 的 UE 發送 `min=0/max=100/dedicated=0` 安全基線並要求 ACK；
停止與異常退出也會恢復基線。

實測確認目前 O-CU-DU 的 Action 6 將 Min/Max PRB 解讀為「單次 grant size limit」，
並不是 UE 總頻寬配額；直接壓低影片 UE 的 Max PRB 會增加小 grant 排程頻率並傷害
語音 latency。因此 VoiceGuard 的有效保護 actuator 採用動態 Offered Load pacing：
RF V2 模型從
`100/85/70/40%` 中選擇影片比例，連續三秒未達 SLA 時安全層只會加強保護；
語音結束後逐級恢復，關閉 xApp 或程序退出時直接恢復。狀態 API 分開回報 RC link/ACK、
traffic shaping factor 與目前策略，避免把 RC ACK 誤當成 QoS 改善。

Manager systemd unit 使用 `KillMode=process`，讓只部署／重啟 Web Manager 時不會
連帶中止由 `oranlabctl` 管理的 gNB、Broker 與 UE。

## 開發驗證

```bash
cd experiment-manager/backend
PYTHONPATH=. .venv/bin/pytest -q tests

cd ../frontend
npm run build
```
