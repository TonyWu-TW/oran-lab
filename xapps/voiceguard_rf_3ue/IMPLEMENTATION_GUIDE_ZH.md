# VoiceGuard RF 3 UE：完整實作、資料蒐集、訓練與控制流程

這份文件用盡量白話但不省略技術細節的方式，說明 `voiceguard_rf_3ue` 是怎麼完成的。

它要解決的問題是：

> UE1、UE2 正在產生會波動的短影片流量。UE3 開始語音通話後，系統根據最近的流量與通話品質，自動判斷應該把影片需求保留在 100%、85%、70% 還是 40%，讓 UE3 比較容易維持穩定通話。

先講最重要的事：目前真正讓圖表上的影片 Offered Load 降下來的主要控制手段，是 **traffic pacing（流量節奏／需求量控制）**。Python xApp 把比例寫入 JSON 控制檔，UE1、UE2 的 traffic process 每秒讀取這個檔案，再改變下一個 HTTP segment 的大小。

系統也會透過 FlexRIC C bridge 發送 E2SM-RC 訊息，但目前發送的是三台 UE 的安全 baseline：

```text
min PRB = 0
max PRB = 100
dedicated PRB = 0
```

它用來確認 Python → C bridge → FlexRIC → E2 node → O-CU-DU 的 RC 通道可以送達並收到 ACK。**目前 RF 選出的 85%／70%／40% 並不是直接轉成 PRB 百分比送進 E2SM-RC。** 這兩條控制路徑一定要分清楚。

---

## 1. 實驗場景

### 1.1 三台 UE 的角色

| UE | 場景 | 應用層流量 | 傳輸層 | 方向 | 基本設定 |
|---|---|---|---|---|---|
| UE1 | 短影片背景流量 | HTTP-like segment | TCP | DL | 基準 8 Mbps、wave、±45%、peak 11.2 Mbps |
| UE2 | 短影片背景流量 | HTTP-like segment | TCP | DL | 基準 8 Mbps、random burst、±45%、peak 11.2 Mbps |
| UE3 | 語音通話 | RTP-like packet | UDP | BOTH | 128 Kbps、每 20 ms 一包 |

這裡的 `DL` 是 Downlink，下行，也就是伺服器往手機下載資料；`BOTH` 表示雙向。

TCP、UDP 是「傳輸層協定」：

- TCP 會重傳、保證順序，適合 HTTP 影片下載。
- UDP 不等待重傳，比較符合即時語音；封包丟了就丟了，但不會為了補舊封包拖慢後面的聲音。

### 1.2 為什麼影片是波動的

真實短影片不是永遠固定每秒下載完全一樣的資料量。播放器會分段下載、預載、停一下再下載。因此 UE1 使用週期波形，UE2 使用固定 seed 的隨機 burst：

```text
UE1 wave period = 16 秒，random seed = 3101
UE2 random burst period = 18 秒，random seed = 3102
segment pacing interval = 1 秒
```

`random seed` 是偽隨機數的起點。使用相同 seed 重啟流量時，會得到相同的隨機序列，方便公平比較四種策略。

---

## 2. 整體架構

```text
前端 Live 頁
    │ REST API：啟動／停止 xApp、查狀態
    ▼
Experiment Manager :8088
    ├── 管理 Run、Traffic Job、資料庫與前端狀態
    ├── 啟動 Python VoiceGuard RF process
    ├── 提供 /traffic API（每台 UE 的即時 progress）
    └── 提供 /metrics/query API（Prometheus 可用時）
             │
             ▼
Python RF xApp
    ├── 每秒讀取 UE1/UE2/UE3 progress
    ├── 組成 12 個 feature
    ├── 取最近 3 秒中位數
    ├── Random Forest 輸出四類機率
    ├── q=0.8 風險決策選控制比例
    ├── 寫 traffic-control.json ───────────────┐
    └── 呼叫 C bridge 發 E2SM-RC baseline     │
             │                                 │
             ▼                                 ▼
       FlexRIC Near-RT RIC              UE1/UE2 traffic process
             │ E2SM-RC                         │ 每個 cycle 讀比例
             ▼                                 ▼
          O-CU-DU                     調整 HTTP segment Offered Load
```

### 2.1 主要程式檔案

| 檔案 | 功能 |
|---|---|
| `common.py` | UE 角色、12 個特徵、SLA、四種策略、feature extraction、JSON actuator |
| `collect.py` | 控制真實 3 UE 實驗、遍歷負載與策略、每秒收資料、checkpoint |
| `relabel.py` | 合併三輪結果、重新產生較穩定的 label、建立 3 秒訓練列 |
| `train.py` | Random Forest 訓練、GroupKFold 驗證、風險決策評估、保存模型 |
| `analyze.py` | 資料完整性、指標範圍、策略成功率、非單調場景分析 |
| `voiceguard_rf_3ue.py` | 3 UE 啟動入口，把 3 UE 的 `common.py` 注入共享 RF runtime |
| `../voiceguard_rf/voiceguard_rf.py` | 真正持續執行的 RF state machine 與 closed-loop runtime |
| `dataset/*.csv/json` | 真實 raw data、訓練資料、實驗 manifest 與品質報告 |
| `models/*.joblib/json` | 模型、訓練報告、closed-loop 驗證結果 |

---

## 3. 流量是怎麼產生的

流量產生器在：

```text
scripts/oranlab-traffic.py
```

Manager 建立 Traffic Job 時，會把下面這個控制檔路徑傳給每個 traffic process：

```text
experiment-manager/backend/data/voiceguard/<run-id>.traffic-control.json
```

### 3.1 UE1／UE2 短影片

短影片每一秒計算一次本次 segment 的目標 Offered Load：

```text
target_mbps = configured_base_mbps × shaping_factor
offered_mbps = min(peak, target_mbps × pattern_factor)
segment_size ≈ offered_bps × 1 秒 ÷ 8
```

其中：

- `configured_base_mbps` 是前端實驗設定的 8 Mbps。
- `shaping_factor` 是 xApp 寫入的 1.0、0.85、0.70 或 0.40。
- `pattern_factor` 是 wave 或 random burst 當下的波動。
- 最後換算出本秒要請求多大的 HTTP segment。

例如 UE1 目前波形計算後原本想要 10 Mbps，RF 選擇 `STRONG_40`：

```text
實際 Offered Load = 10 × 0.40 = 4 Mbps
```

這不是 Linux `tc` 限速，也不是把 TCP socket 的頻寬硬切斷；它是讓應用程式「少要求一些資料」。這比較像影片播放器降低預載量或畫質需求。

### 3.2 UE3 RTP-like 語音

UE3 每 20 ms 發出一個 UDP packet，也就是理想狀況每秒約 50 包。

128 Kbps、20 ms 對應的 payload 大約是：

```text
128000 bit/s ÷ 8 × 0.020 s = 320 bytes
```

packet 前 12 bytes 放 sequence number 與單調時鐘 timestamp。伺服器收到後直接 echo 回 UE3。UE3 用「現在時間－封包內 timestamp」得到 RTT。

這是 RTP-like，不是完整 SIP/IMS/VoLTE 協定棧。它模擬固定間隔、小封包、低延遲的語音資料面特性。

---

## 4. 系統怎麼蒐集資料

### 4.1 資料不是直接從前端 DOM 抓的

collector 透過 Experiment Manager 的 REST API 工作：

```text
GET  /api/runs/<run-id>
GET  /api/runs/<run-id>/traffic/config
GET  /api/runs/<run-id>/traffic
POST /api/runs/<run-id>/traffic/batch
DELETE /api/runs/<run-id>/traffic/<job-id>
GET  /api/runs/<run-id>/metrics/query?metric=ue_rx_bps
```

最重要的是 `/traffic`。Manager 的 `traffic_worker` 持續讀取 traffic process 輸出的 JSON progress，再保存到 Traffic Job 的 `result.progress`。因此 collector 每秒查一次 `/traffic`，就能取得：

- UE1／UE2 的 offered、delivered、shaping factor、HTTP latency、成功/失敗 request。
- UE3 的 offered、received、delivery ratio、loss、jitter、RTT P95。

Prometheus 可用時，collector 也會查 `ue_rx_bps`。若 Prometheus 沒開或回 502，程式會回傳空表並自動使用 Traffic Job progress，不會讓資料蒐集中斷。

### 4.2 本次正式資料設定

本次 manifest 保存在 `dataset/collection_manifest.json`：

```text
Run ID             a6b82179-1375-43a6-9724-a4ad5564bbdb
base levels        0.15, 0.25, 0.40, 0.55, 0.75, 1.00
UE1/UE2 load pairs 6 × 6 = 36 種
repeated rounds    3 輪
candidate policies 4 種
warm-up            3 秒
sample window      3 秒，每秒 1 筆
```

因此總場景數：

```text
36 load pairs × 3 rounds = 108 個 round-specific scenarios
```

總 raw samples：

```text
108 scenarios × 4 policies × 3 seconds = 1,296 筆
```

四個 policy 各有 324 筆，沒有缺值、NaN 或重複 sample key。

### 4.3 為什麼有 base scale

UE1、UE2 前端基準都是 8 Mbps，但 collector 需要測不同壓力：

```text
scenario base offered ≈ 8 Mbps × base_scale
```

例如：

```text
UE1 base scale = 0.25
UE2 base scale = 1.00
```

代表 UE1 的基準需求約 2 Mbps，UE2 約 8 Mbps；實際值仍會再受到 wave、random burst 與 peak 影響。

之後再乘 policy：

```text
final UE factor = scenario_base_scale × policy_scale
```

程式限制最後 factor 在 0.1 到 1.0 之間。

### 4.4 四種候選策略

| Policy | UE1/UE2 保留的 Offered Load | 意思 |
|---|---:|---|
| `EQUAL_100` | 100% | 完全不保護，影片照原需求送 |
| `LIGHT_85` | 85% | 輕微降低影片需求 |
| `MEDIUM_70` | 70% | 中度降低 |
| `STRONG_40` | 40% | 強力保護語音 |

UE3 永遠是 1.0，不會被影片策略限速。

### 4.5 每個候選策略怎麼測

對每一個 load pair：

1. 將四種策略順序隨機打亂，減少「永遠先測某策略」造成的偏差。
2. 停止 UE1、UE2 的影片 process；UE3 語音保持連續。
3. 原子寫入新的 base scale 與 policy。
4. 重新啟動 UE1、UE2。
5. 等 3 秒 warm-up，避開程序剛啟動的瞬態。
6. 連續 3 秒，每秒收一筆 feature 與 UE 指標。
7. 計算這個 policy 的 SLA success ratio 與各指標中位數。
8. 四個 policy 都測完後，立即 checkpoint 到磁碟。

重新啟動 UE1／UE2 是為了讓 wave phase 與 random seed 從相同起點開始，讓 A/B 比較比較公平。3 秒 warm-up 很重要；曾實測 1 秒 warm-up 時，量到的大多是 process 啟動瞬態，四策略的效果幾乎無法區分，所以那批資料沒有拿來訓練。

### 4.6 Checkpoint 與中斷恢復

每完成一個 scenario 就原子寫入：

```text
dataset/raw_samples.csv
dataset/training.csv               # collector 的暫時版本
dataset/collection_results.json    # collector resume checkpoint
```

`--resume` 會讀 `collection_results.json`，跳過已完成 scenario。使用獨立的 `collection_results.json`，是為了避免後面的 relabel output 覆蓋 collector 的 scenario ID。

不管正常完成或按 Ctrl+C，`finally` 都會把 UE1、UE2 恢復 `EQUAL_100`，避免機器停留在 40%。

---

## 5. 12 個模型輸入指標

`feature` 是機器學習模型的輸入欄位，可以理解成模型每次做決定前看到的「狀態摘要」。

| Feature | 單位 | 白話意思 | 來源／公式 |
|---|---:|---|---|
| `video_offered_mbps` | Mbps | UE1+UE2 目前總共想送多少影片 | 兩台 offered 相加 |
| `video_delivered_mbps` | Mbps | 模型使用的影片送達量 | 兩台 delivered 相加，但不允許高於 offered |
| `video_delivery_ratio` | 0–1 | 影片需求滿足比例 | delivered / offered，上限 1 |
| `video_gap_mbps` | Mbps | 想送但沒有送到的差距 | max(0, offered-delivered) |
| `video_worst_delivery_ratio` | 0–1 | UE1/UE2 中較差的滿足率 | min(UE1 ratio, UE2 ratio) |
| `video_imbalance_mbps` | Mbps | 兩台影片需求有多不平均 | abs(UE1 offered-UE2 offered) |
| `ue1_offered_mbps` | Mbps | UE1 自己的影片需求 | UE1 progress |
| `ue2_offered_mbps` | Mbps | UE2 自己的影片需求 | UE2 progress |
| `voice_delivery_ratio` | 0–1 | UE3 語音封包送達比例 | rolling received / sent，修正一個邊界在途封包 |
| `voice_loss_percent` | % | UE3 最近窗口丟包率 | rolling lost / sent × 100 |
| `voice_jitter_ms` | ms | UE3 RTT 變化幅度 | 相鄰 RTT 差的絕對值平均 |
| `voice_rtt_p95_ms` | ms | 95% 語音封包 RTT 不超過此值 | 最近 3 個一秒窗口的 RTT 第 95 百分位 |

### 5.1 RTT P95 是什麼

假設最近 RTT 排序後是：

```text
35, 36, 37, 38, 40, 42, 45, 50, 80, 110 ms
```

平均值可能被少量尖峰影響，也可能把尖峰藏起來。P95 是接近最慢 5% 的位置，用來觀察「大部分封包裡比較差的延遲」。即時通話很怕偶發卡頓，所以 P95 通常比平均 RTT 更有意義。

### 5.2 這裡的 jitter 是什麼

本專案的 jitter 是「相鄰 RTT 的變化量平均」：

```text
jitter = mean(abs(RTT[n] - RTT[n-1]))
```

它反映封包延遲是否忽快忽慢。這不是完整 RFC RTP inter-arrival jitter estimator，所以文件與論文中要稱為 `RTT variation based jitter` 或清楚說明計算方式。

### 5.3 為什麼 delivered 要 cap 到 offered

HTTP progress 的 `delivered_bps` 是某個 segment 實際下載時的瞬時 burst rate；播放器可能一瞬間用 20 Mbps 下載一個 segment，然後剩下時間不下載。Offered Load 則是整個 pacing interval 的平均需求。

因此可能看到：

```text
offered = 2 Mbps
HTTP burst delivered = 15 Mbps
```

這不代表需求滿足率是 750%。模型輸入會做：

```text
model_delivered = min(offered, raw_delivered)
```

raw CSV 仍保留原始量測，relabel 與 runtime 都使用相同 normalization。

### 5.4 本次資料裡哪些 feature 幫助不大

本次 `video_delivery_ratio`、`video_gap_mbps`、`video_worst_delivery_ratio` 幾乎都是常數，Random Forest feature importance 為 0。這表示目前 traffic progress 沒有提供足夠好的長期 delivered/capacity 訊號。

目前比較有用的是：

1. 總影片 offered load。
2. UE1/UE2 各自 offered load。
3. 兩台流量不平衡程度。
4. UE3 RTT P95。
5. UE3 jitter。

未來若能從 E2SM-KPM 取得 PRB usage、buffer occupancy、CQI、MCS 等真正 radio 指標，模型狀態會更完整。

---

## 6. SLA 與 label 怎麼產生

### 6.1 SLA 是什麼

SLA 是 Service Level Agreement，這裡不是商業合約，而是「我們認定語音品質合格的門檻」。四項必須同時成立：

```text
voice delivery ratio >= 0.95
voice loss percent   <= 2%
voice jitter         <= 30 ms
voice RTT P95        <= 60 ms
```

乾淨的 3 UE 軟體 radio 通話通常約 35–40 ms，因此設 60 ms，既能抓到壅塞惡化，又保留正常 scheduler 波動空間。

### 6.2 一個 policy 何時算可接受

三輪合併後，每個 load pair、每個 policy 有 9 筆一秒資料：

```text
3 rounds × 3 seconds = 9 samples
```

policy 必須同時符合：

```text
至少 75% 的秒數通過完整 SLA
delivery ratio 的中位數 >= 0.95
loss 中位數 <= 2%
jitter 中位數 <= 30 ms
RTT P95 中位數 <= 60 ms
```

9 筆裡至少需要 7 筆完整 SLA pass，因為 6/9 只有 66.7%。

### 6.3 Label 不是人工列舉答案

四個 policy 按「對影片影響最小 → 最大」排序：

```text
EQUAL_100 → LIGHT_85 → MEDIUM_70 → STRONG_40
```

程式實際測完四種結果後，選擇第一個可接受 policy 當 label：

```python
label = first(policy for policy in POLICY_ORDER if acceptable(outcome[policy]))
```

如果四個都失敗，label fallback 到 `STRONG_40`，意思是「已經沒有更強候選」，不代表 STRONG 一定成功。報告會另外計算 STRONG 的真實成功率，不會把 fallback 當成成功。

所以它不是預先寫幾千條：

```text
if 8 Mbps then 70%
if 12 Mbps then 40%
```

而是先用真實系統測出「在這個狀態下，哪個最輕策略足夠」，再讓 Random Forest 從數據學習一般化規律。

### 6.4 為什麼要 relabel

單輪 radio 結果有 scheduler noise；同一 load pair 不同輪可能得到不同單輪 label。`relabel.py` 不使用單輪 label，而是：

1. 用 UE1/UE2 base scale 組成固定 group ID，例如 `load-u1-025-u2-100`。
2. 合併同一 group 的三輪 raw policy samples。
3. 用共 9 秒 outcome 重新判定四個 policy。
4. 重新產生最終 label。

本次 36 個 group 的最終分布：

```text
EQUAL_100  11
LIGHT_85    5
MEDIUM_70   5
STRONG_40  15
```

### 6.5 為什麼訓練列只使用 EQUAL baseline feature

模型要回答的是：「通話剛開始、還沒控制時，應選哪個 policy？」

因此 input 使用每輪 `EQUAL_100` 的最近 3 秒中位數；label 則來自同 load pair 四個 policy 的完整 counterfactual 實測。這樣模型不會因為已經看到 `STRONG_40` 後的低 offered load，才反過來猜 STRONG。

每個 load pair 有 3 輪，每輪產生一個 3 秒 median feature vector：

```text
36 load pairs × 3 rounds = 108 training rows
```

---

## 7. Random Forest 演算法

### 7.1 Random Forest 是什麼

Random Forest 中文是隨機森林。它由很多棵 Decision Tree（決策樹）組成。

一棵簡化的樹可能像：

```text
video_offered_mbps > 10?
├── 否：voice_rtt_p95 > 60?
│   ├── 否：EQUAL_100
│   └── 是：MEDIUM_70
└── 是：video_imbalance > 3?
    ├── 否：STRONG_40
    └── 是：MEDIUM_70
```

單棵樹很容易把 noise 記住。Random Forest 會：

1. 對訓練資料做 bootstrap sampling，也就是有放回抽樣。
2. 每個 split 只看隨機一部分 feature。
3. 訓練很多棵不同的樹。
4. 合併所有樹對四個 class 的投票機率。

本模型參數：

```text
n_estimators       500 棵樹
max_depth          5
min_samples_leaf   3
max_features       sqrt
class_weight       balanced_subsample
random_state       20260803
```

限制深度與 leaf 大小，是因為只有 36 個真正獨立 load pair；樹太深很容易記住 radio noise。

`balanced_subsample` 是讓較少的 LIGHT/MEDIUM class 在每棵樹裡有比較高權重，避免模型只猜最多的 STRONG。

### 7.2 模型輸出什麼

模型不只輸出一個 class，也輸出四個機率，例如：

```json
{
  "EQUAL_100": 0.05,
  "LIGHT_85": 0.10,
  "MEDIUM_70": 0.35,
  "STRONG_40": 0.50
}
```

普通 argmax 會選最大值，也就是 STRONG。

### 7.3 q=0.8 風險決策

語音保障比多留一點影片更重要，因此 runtime 使用 80th-percentile policy。依照保護強度累積機率：

```text
EQUAL cumulative  = 0.05
LIGHT cumulative  = 0.15
MEDIUM cumulative = 0.50
STRONG cumulative = 1.00  ← 第一個超過 0.80
```

所以選 STRONG。

如果模型 argmax 是 MEDIUM，但仍給 STRONG 一定機率，q=0.8 可能選比 argmax 更保守的一級。它不是修改 RF 模型，而是把 RF 的不確定性轉成風險偏好。

離線 held-out counterfactual 結果：

| 決策方式 | SLA 成功率 | 平均影片保留比例 |
|---|---:|---:|
| 永遠 EQUAL | 30.6% | 100% |
| RF argmax | 55.6% | 69.6% |
| RF q=0.8 | 66.7% | 53.3% |
| 永遠 STRONG | 77.8% | 40% |

q=0.8 是折衷：比 argmax 更保護語音，又不是所有情況都硬壓到 40%。

---

## 8. 怎麼驗證模型，避免資料洩漏

### 8.1 資料洩漏是什麼

如果同一 load pair 的 round 1 放到 training、round 2 放到 validation，兩者幾乎是同一場景，模型容易得到虛高成績。這叫 data leakage：考試時偷看過幾乎相同的題目。

### 8.2 GroupKFold

程式用 `scenario_id = load-u1-xxx-u2-xxx` 當 group，做 5-fold GroupKFold：

- 同一 load pair 的所有輪次一定在同一 fold。
- 一次用四部分訓練、一部分驗證。
- 輪流讓五部分都當過驗證集。

本次結果：

```text
scenario exact accuracy              50.0%
scenario balanced accuracy           38.2%
within-one-policy-level accuracy      77.8%
mean absolute policy-level error       0.83 級
```

Exact accuracy 50% 不算高，尤其 LIGHT/MEDIUM 很難分；這和三 UE software radio 的 scheduler noise、只有 36 個獨立 load pair、缺少 PRB/buffer/CQI 等 radio feature 有關。文件不把它包裝成高準確模型。

「差一級內 77.8%」表示即使沒有猜中 exact class，多數錯誤仍在相鄰策略，例如 85% 猜成 70%，而不是 100% 猜成 40%。

---

## 9. Online runtime 怎麼做決策

前端按「啟動 xApp」後，Manager：

1. 確認 Run 是 `RUNNING`。
2. 從 Run snapshot 判斷 UE 集合是不是 `{ue1, ue2, ue3}`。
3. 自動選擇 `voiceguard_rf_3ue.py` 與 `voiceguard_rf_3ue.joblib`。
4. 將 model path、state file、traffic control file、mode 打包為 JSON argument。
5. 用 subprocess 啟動 Python xApp。

### 9.1 Observe Only 與 Closed Loop

- `observe_only`：模型照常取樣與建議，但不改 traffic control file。
- `closed_loop`：模型建議會實際寫入 traffic control file，並在啟動時驗證 E2SM-RC baseline。

### 9.2 最近 3 秒中位數

xApp 每秒收一次 12 維 feature，保存最近 3 筆：

```text
t-2 秒 feature
t-1 秒 feature
t   秒 feature
```

每個 feature 分別取 median，再送入 RF。Median 對單一異常尖峰比平均值穩定。

模型 artifact 內保存：

```text
input_window_seconds = 3
decision_quantile = 0.8
feature_names = 固定 12 欄
```

runtime 會檢查模型 feature 順序和程式是否完全相同；不相同就拒絕啟動，避免餵錯欄位。

### 9.3 State machine

```text
OFF
  │ 啟動
  ▼
OBSERVING ──收滿 3 秒──> RF decision
  │                           │
  │ policy < 100%             ▼
  └──────────────────────> PROTECTING
                                  │
                       連續 3 秒 SLA fail
                                  │
                                  ▼
                         安全層加強一級
                                  │
                         通話停止／關閉 xApp
                                  ▼
                    COOLDOWN → EQUAL_100 → OFF
```

RF 在通話剛被偵測到時做初始決策。之後 deterministic safety layer（固定規則安全層）只允許加強：

```text
EQUAL → LIGHT → MEDIUM → STRONG
```

連續 3 秒完整 SLA 未達標才升一級，避免一秒 noise 造成策略來回震盪。已經 STRONG 時不會再假裝升級。

通話結束後每隔幾秒逐級恢復；xApp 收到 SIGTERM 或正常停止時直接恢復 100%。

---

## 10. 控制命令到底怎麼送下去

目前有兩條不同路徑。

### 10.1 路徑 A：真正改變影片流量的 Traffic Pacing

假設 RF 選 `MEDIUM_70`：

1. Python 呼叫 `write_traffic_scale(control_file, 0.70, reason)`。
2. `common.py` 使用 temporary file + `os.replace` 原子更新 JSON。
3. JSON 大致是：

```json
{
  "updated_at": 1785750000.0,
  "reason": "voiceguard_rf_medium_70",
  "scenario_base_scales": {"ue1": 1.0, "ue2": 1.0},
  "policy": "MEDIUM_70",
  "policy_scale": 0.7,
  "ues": {"ue1": 0.7, "ue2": 0.7, "ue3": 1.0}
}
```

4. UE1、UE2 的 `oranlab-traffic.py` 每個一秒 cycle 呼叫 `shaping_factor(job)`。
5. 它讀 `ues[ue1]` 或 `ues[ue2]`，限制在 0.1–1.0。
6. 下一個 HTTP segment 的 target Mbps、peak Mbps、segment size 都乘這個 factor。
7. traffic process 輸出新的 `offered_bps` 與 `shaping_factor` progress。
8. Manager 收到 progress，前端下一次 polling 就看到 Offered Load 降低。

原子寫入的意思是讀取者只會看到完整舊檔或完整新檔，不會看到寫到一半的 JSON。

### 10.2 路徑 B：E2SM-RC C bridge

這版 FlexRIC Python SDK 沒有 expose 標準 RC control API，因此 Python 用環境變數把策略傳給 C 程式：

```text
VOICEGUARD_POLICIES=0:0:100:0,1:0:100:0,2:0:100:0
VOICEGUARD_SST=1
VOICEGUARD_SD=ffffff
```

每個 item 格式：

```text
ue_f1ap_id:min_prb:max_prb:dedicated_prb
```

C bridge：

1. 初始化 FlexRIC xApp API。
2. 找到宣告 RC RAN function ID 3 的 E2 node。
3. 建立 E2SM-RC Control Header Format 1。
4. 使用 RC Style Type 2、Control Action ID 6。
5. UE identity 使用 `GNB_DU_UE_ID_E2SM` 與 CU UE F1AP ID。
6. 組出 RRM Policy Ratio List，包括 PLMN、SST、SD、min/max/dedicated PRB ratio。
7. 呼叫：

```c
control_sm_xapp_api(&node->id, 3, &control)
```

8. 將 ACK 印成：

```text
VOICEGUARD_RC_RESULT success=true ue_id=0 min=0 max=100 dedicated=0 ...
```

Python 要求三台 UE 都有 `success=true` 才把 `e2_connected` 設為 true。

### 10.3 為什麼 RC 目前只送 baseline

在目前 O-CU-DU 實測中，Action 6 的 Min/Max PRB 比較像單次 grant size 限制，不是「這台 UE 的總頻寬份額」。直接把影片 UE 的 Max PRB 改小，可能增加許多小 grant，反而讓語音 latency 變差。

因此目前採取：

- E2SM-RC：確認標準控制通道與安全 baseline ACK。
- Traffic pacing：作為真正、可觀察、已驗證能改善語音的 actuator。

`actuator` 是控制器真正能改變系統的手段，例如油門、閥門；這裡就是影片 Offered Load factor。

---

## 11. 實際結果

### 11.1 資料集整體

四策略逐秒 SLA 成功率：

```text
EQUAL_100   51.5%
LIGHT_85   52.5%
MEDIUM_70  63.6%
STRONG_40  82.7%
```

這代表整體方向合理：保護越強，UE3 成功率越高。但 36 個 load pair 裡有 13 個出現某些非單調 outcome，例如某輪 MEDIUM 過、STRONG 反而沒過。這是 software radio scheduler、CPU scheduling、短窗口與 traffic restart noise 的結果，原始資料沒有被刪除。

### 11.2 Closed-loop 20 秒 A/B smoke test

| 模式 | RTT median | RTT P95 | jitter median | SLA success |
|---|---:|---:|---:|---:|
| RF `STRONG_40` | 41.2 ms | 100.8 ms | 11.4 ms | 75% |
| Baseline 100% | 90.8 ms | 114.0 ms | 12.3 ms | 20% |

這次短測說明 RF pacing 確實能大幅改善 median RTT 與 SLA rate，但沒有消除所有 RTT spike。它是 sequential smoke test，不是隨機化長期 A/B，不能當成正式統計論文結論。

---

## 12. 資料檔案怎麼看

### `dataset/raw_samples.csv`

每列是一秒、某個 scenario、某個 policy 的真實量測。重要欄位：

```text
timestamp
scenario_id
round
ue1_base_scale / ue2_base_scale
policy / policy_scale
sample_index
12 個 feature
sla_ok
ue_metrics（完整 per-UE JSON）
```

### `dataset/collection_manifest.json`

保存 Run ID、UE traffic snapshot、levels、rounds、warm-up、sample seconds、random seed、SLA 與預期筆數，用來重現實驗。

### `dataset/collection_results.json`

保存每一輪、每個 load pair 的四策略順序、outcome、單輪 label，也是 collector 的 resume checkpoint。

### `dataset/policy_results.json`

`relabel.py` 合併三輪後的 36 個最終 load-pair outcome 與 label。

### `dataset/training.csv`

最終 108 列訓練資料。每列是某輪 EQUAL baseline 最近 3 秒的 median feature，label 是三輪聚合後最少必要 policy。

### `dataset/quality_report.json`

資料筆數、缺值、feature min/median/max、各 policy SLA rate、三輪 label agreement、非單調 pair 數量。

### `models/training_report.json`

模型參數、class distribution、GroupKFold accuracy、confusion matrix、feature importance、argmax 與 q=0.8 counterfactual 評估。

### `models/voiceguard_rf_3ue.joblib`

不只保存 sklearn model，也保存 feature order、SLA、policy scales、3 秒 window、q=0.8 與版本資料。Runtime 直接載入它。

---

## 13. 如何重跑

先確保 3 UE Run 與三種 traffic 設定已存在，再執行：

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue \
experiment-manager/backend/.venv/bin/python \
xapps/voiceguard_rf_3ue/collect.py \
  --run-id RUN_ID \
  --output-dir xapps/voiceguard_rf_3ue/dataset \
  --control-file experiment-manager/backend/data/voiceguard/RUN_ID.traffic-control.json \
  --rounds 3 \
  --levels 0.15,0.25,0.40,0.55,0.75,1.00 \
  --campaign fair-base8-w3 \
  --warmup-seconds 3 \
  --sample-seconds 3 \
  --resume
```

重新聚合 label：

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue \
experiment-manager/backend/.venv/bin/python \
xapps/voiceguard_rf_3ue/relabel.py \
  --raw-samples xapps/voiceguard_rf_3ue/dataset/raw_samples.csv \
  --output-dir xapps/voiceguard_rf_3ue/dataset \
  --run-id RUN_ID
```

訓練：

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue \
experiment-manager/backend/.venv/bin/python \
xapps/voiceguard_rf_3ue/train.py \
  --dataset xapps/voiceguard_rf_3ue/dataset/training.csv \
  --policy-results xapps/voiceguard_rf_3ue/dataset/policy_results.json \
  --model xapps/voiceguard_rf_3ue/models/voiceguard_rf_3ue.joblib \
  --report xapps/voiceguard_rf_3ue/models/training_report.json
```

資料品質分析：

```bash
PYTHONPATH=xapps/voiceguard_rf_3ue \
experiment-manager/backend/.venv/bin/python \
xapps/voiceguard_rf_3ue/analyze.py \
  --dataset-dir xapps/voiceguard_rf_3ue/dataset \
  --output xapps/voiceguard_rf_3ue/dataset/quality_report.json
```

---

## 14. 已知限制與下一步

### 目前限制

1. 只有 3 UE、36 個獨立 load pair，資料規模仍小。
2. LIGHT/MEDIUM label 少且 noisy，exact classification accuracy 只有約 50%。
3. 沒有 KPM PRB usage、RLC buffer、CQI、MCS、BLER 等 radio feature。
4. HTTP delivered 是 burst rate，不能當成長期 capacity；三個 delivery feature 本次幾乎沒有資訊量。
5. RTP-like 不是完整 VoLTE/VoNR/IMS call flow。
6. E2SM-RC 目前只驗證 baseline，主要有效 actuator 是應用層 pacing。
7. Safety layer 只會在通話中加強，不會主動降級；可能多保護一段時間。
8. 3 秒窗口反應快，但也更容易受到短期 scheduler noise 影響。

### 建議下一步

1. 蒐集更多日期、channel、SNR、fading 與 CPU load 的資料，不只重複同一小時。
2. 增加 E2SM-KPM / MAC / RLC 真實 radio feature。
3. 將策略改成「預測每個 action 的 SLA 成功機率與影片 utility」，而不是只做四類分類。
4. 對 q 值做獨立 validation，不要只在同一份 36 group 上調參。
5. 做 randomized longer A/B，而不是只有 sequential 20 秒 smoke test。
6. LLM 版本使用完全相同 12 維 state、四個 action 與 SLA evaluator，才能公平比較。

---

## 15. 名詞快速解釋

| 名詞 | 白話解釋 |
|---|---|
| UE | User Equipment，手機／終端；這裡是軟體 UE |
| gNB | 5G 基地台 |
| O-CU-DU | O-RAN 的 Central Unit / Distributed Unit；本專案基地台協定處理部分 |
| Near-RT RIC | Near-Real-Time RAN Intelligent Controller，讓 xApp 以接近即時方式觀察／控制 RAN |
| xApp | 跑在 Near-RT RIC 生態裡的控制應用；本專案 policy 在 Python process |
| E2 | RIC 和 E2 node（如 O-CU/O-DU）之間的介面 |
| E2SM-RC | E2 Service Model – RAN Control，標準化 RAN control message 格式 |
| E2SM-KPM | Key Performance Measurement，標準化 RAN 指標回報模型 |
| PRB | Physical Resource Block，radio scheduler 分配的基本時頻資源之一 |
| F1AP ID | CU 與 DU 間 F1 Application Protocol 使用的 UE 識別 |
| Offered Load | 應用程式想送多少資料，不等於實際成功送達量 |
| Delivered Throughput | 實際送達／量測到的資料速率 |
| Pacing | 控制資料請求節奏與大小，改變 Offered Load |
| SLA | 用來判斷服務品質是否合格的一組門檻 |
| RTT | Round-Trip Time，封包來回一次的時間 |
| P95 | 第 95 百分位，用來看大多數樣本中偏差的尾端延遲 |
| Jitter | 延遲變化幅度；這裡是相鄰 RTT 差的平均 |
| Feature | 模型輸入指標 |
| Label | 訓練時希望模型學會輸出的正確策略 |
| Random Forest | 多棵隨機化決策樹組成的分類模型 |
| Argmax | 選機率最大的 class |
| Quantile q=0.8 | 根據累積機率選較保守的第 80 百分位策略 |
| Closed Loop | 模型決策會真正改變系統 |
| Observe Only | 只計算建議，不真正控制 |
| Counterfactual | 同一狀態下如果改用另一個 action，結果會怎樣；本專案靠逐一實測四策略取得 |
| GroupKFold | 保證同一 load pair 不跨 train/validation 的交叉驗證 |
| Data Leakage | 驗證集偷看到與訓練集幾乎相同資料，造成虛高成績 |
| ACK | Acknowledgement，接收方確認控制訊息已處理／接受 |

---

## 16. 一句話總結

這個版本不是讓 Random Forest 直接神奇地控制基地台 PRB，而是用真實 3 UE software-radio 實驗逐一測出四種影片 Offered Load action 對 UE3 語音的影響，讓 RF 學習「什麼狀態需要多強的保護」，再由 Python closed-loop 把決策寫入 traffic pacing control file；E2SM-RC C bridge 同時負責驗證標準 RC 通道與安全 baseline。
