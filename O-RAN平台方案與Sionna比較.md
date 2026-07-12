# O-RAN Demo 平台方案與 Sionna / DGX Spark 比較

整理日期：2026-07-02

## 1. 目前想做的主線

目前比較穩定、適合 demo 與論文實驗的主線是：

```text
Open5GS
  ↓
srsRAN/OCUDU gNB
  ↓ E2
Near-RT RIC
  ↓
xApp Controller
```

也可以用完整 O-RAN 架構來看：

```text
SMO / Non-RT RIC / rApp
        |
        | A1 / O1
        v
Near-RT RIC
        |
        | E2
        v
O-CU / O-DU  ---- F1 / internal split ---- O-RU or simulated RU
        |
        | N2 / N3
        v
5G Core
```

實作組合可以先定成：

```text
Open5GS + srsRAN/OCUDU + FlexRIC
```

其中各元件的角色如下。

| 元件 | 角色 | 在 demo 中做什麼 |
| --- | --- | --- |
| Open5GS | 5G Core | 提供 AMF、SMF、UPF 等核心網功能，讓 UE 可以註冊、建立 PDU session、跑 ping / iperf traffic。 |
| srsRAN / OCUDU gNB | RAN / gNB / CU-DU | 模擬或實作基地台，連接 UE 與 5G Core，並透過 E2 interface 把 RAN 指標送給 RIC。 |
| FlexRIC | Near-RT RIC | 接收 gNB 的 E2 訊息，讓 xApp 可以取得 KPM 指標並下發控制命令。 |
| xApp Controller | 自己的演算法 | 讀取 throughput、PRB usage、UE 狀態等指標，決定 PRB allocation、QoS、slicing 或 traffic control。 |
| SMO / Non-RT RIC / rApp | 長時間策略與管理 | 初期可以先不完整實作，後續可用 Python mock 或簡化服務表示 policy、model training、configuration management。 |

## 2. 這個平台可以驗證什麼

這套平台最適合驗證的是 O-RAN 的 closed-loop control：

```text
UE traffic
  ↓
gNB / O-CU / O-DU 產生 KPM
  ↓
Near-RT RIC 接收 E2 indication
  ↓
xApp 執行自定義演算法
  ↓
RIC 下發 control action
  ↓
觀察 throughput / latency / PRB usage / fairness 是否改善
```

可以做的實驗案例：

| 實驗案例 | 說明 | 適合程度 |
| --- | --- | --- |
| KPM monitoring xApp | xApp 讀取 UE throughput、PRB usage、cell load 等指標。 | 很適合當第一個 demo |
| PRB allocation xApp | 根據 traffic load 動態調整不同 slice 或 UE group 的 PRB 配置。 | 很適合作為論文主案例 |
| QoS / slicing control | 比較 fixed slicing 與 dynamic slicing 對 throughput、latency、fairness 的影響。 | 適合 |
| Traffic steering / handover | 根據 cell load 或 UE signal quality 決定是否切換 cell。 | 可做，但多 cell 設定較麻煩 |
| AI inference for xApp | xApp 呼叫 ML model 做決策，例如流量預測或資源分配。 | 適合後續加強 |

## 3. 傳統 O-RAN Testbed 與 Sionna / DGX Spark 的差異

重點是：這兩條路不是完全互斥，但研究層級不同。

- Open5GS + srsRAN/OCUDU + FlexRIC：偏系統層、RAN control、RIC/xApp、5G Core、UE traffic。
- Sionna / DGX Spark：偏 PHY 層、channel simulation、neural receiver、GPU AI inference。

| 你想改的東西 | 傳統 Open5GS / srsRAN / FlexRIC | Sionna / DGX Spark |
| --- | --- | --- |
| 5G Core 設定 | 很適合。Open5GS 就是 5G Core。 | 不是重點。Sionna 不負責核心網。 |
| UE attach / traffic | 很適合。可以做 UE registration、PDU session、ping、iperf。 | 可以模擬部分 traffic 或 link behavior，但不是完整 5G Core attach 流程。 |
| RIC / xApp 資源分配 | 很適合。FlexRIC / Near-RT RIC 就是為 xApp control 設計。 | 可以模擬決策邏輯，但不是標準 RIC testbed 主線。 |
| PRB / slicing / QoS 控制 | 很適合。可以透過 xApp 做 PRB allocation 或 slicing control 實驗。 | 可以模擬 PHY/link-level 效果，但不一定對應真實 RIC control interface。 |
| PHY channel simulation | 不方便。srsRAN 可以跑實驗，但不適合大量改 channel model。 | 很適合。Sionna 強項就是 GPU 加速的 link-level / channel simulation。 |
| neural demapper / neural receiver | 很難。要改 PHY 底層，工程量大。 | 比較適合。Sionna 本來就適合做 AI-native PHY / neural receiver 研究。 |
| TensorRT / CUDA real-time AI | 要自己整合。xApp 可以呼叫 GPU inference service，但不是原生流程。 | NVIDIA 路線比較順。TensorRT、CUDA、Sionna、DGX 生態系整合度高。 |
| ray tracing channel | 幾乎不是主線。傳統 testbed 通常用實體 RF、ZMQ 模擬或簡化 channel。 | Sionna RT 很適合。可以根據 3D 場景模擬無線傳播路徑。 |

## 4. 三個容易混淆的 NVIDIA / PHY 名詞

### 4.1 Ray tracing channel 是什麼

Ray tracing channel 是一種比較真實的無線通道模擬方式。

一般簡化通道模型可能只說：

```text
距離越遠，訊號越弱
有雜訊
有 fading
```

但 ray tracing channel 會考慮實際 3D 場景，例如建築物、牆壁、地面、天線位置，然後追蹤電波可能走過的路徑：

```text
直射路徑
反射路徑
繞射路徑
散射路徑
不同 path 的 delay / angle / path loss
```

簡單說，它是在回答：

> 如果基地台在這裡、UE 在那裡，中間有牆、樓、反射面，那實際 radio channel 會長什麼樣子？

適合研究：

- beamforming
- MIMO channel
- RIS / reconfigurable intelligent surface
- 室內外場景 coverage
- AI receiver 在不同真實場景下的 robustness

Sionna RT 的定位就是用 GPU 加速 ray tracing，產生更真實的 radio propagation / channel model。

### 4.2 TensorRT / CUDA real-time AI 是什麼

CUDA 是 NVIDIA GPU 的平行運算平台。簡單說，就是讓程式可以用 GPU 跑大量平行計算。

TensorRT 是 NVIDIA 的 deep learning inference optimization SDK。它不是拿來訓練模型為主，而是把已經訓練好的模型轉成更適合 NVIDIA GPU 執行的版本。

它通常做的事情包括：

```text
模型圖最佳化
layer fusion
kernel tuning
FP16 / INT8 低精度推論
減少 inference latency
提高 throughput
```

在 O-RAN / AI-RAN 的語境下，可以把它想成：

> xApp 或 PHY receiver 需要很快做 AI 推論時，用 TensorRT/CUDA 把模型加速，讓 decision 或 decoding latency 更低。

適合研究：

- real-time neural receiver inference
- xApp inference service acceleration
- AI-based link adaptation
- GPU batched inference
- edge AI / AI-RAN shared compute

### 4.3 Neural demapper / neural receiver 是什麼

在傳統通訊系統裡，receiver 會把收到的無線訊號一步一步處理：

```text
received signal
  ↓
channel estimation
  ↓
equalization
  ↓
demapping
  ↓
decoding
  ↓
bits
```

Demapper 的工作是把接收到的 constellation symbol，例如 QPSK、16QAM、64QAM，轉成 bit 的機率或 soft information。

Neural demapper 就是用神經網路取代或輔助傳統 demapper：

```text
received symbol + channel information
  ↓
neural network
  ↓
bit likelihood / LLR
```

Neural receiver 則更進一步，不只替代 demapper，而是用 neural network 處理 receiver 裡更大一段流程，例如 channel estimation、equalization、demapping，甚至接近 end-to-end receiver。

簡單說：

- neural demapper：只改 receiver 中的 demapping 模組。
- neural receiver：用 AI 改 receiver 的一大段 PHY 處理流程。

這類研究通常比較接近 PHY / signal processing，需要懂 OFDM、MIMO、channel estimation、QAM、LLR、LDPC 等底層細節。對軟體背景來說，直接切這條線會比較硬。

## 5. 對目前碩士研究的建議

如果目標是穩定做出 demo、幫老師驗證架構、並整理成論文，建議主線不要一開始放在 neural receiver 或 ray tracing channel。

比較穩的主線：

```text
Open5GS + srsRAN/OCUDU + FlexRIC
  ↓
建立 O-RAN closed-loop control demo
  ↓
實作 xApp-based PRB allocation / QoS slicing
  ↓
用 throughput、latency、fairness、PRB utilization 做實驗比較
```

Sionna / DGX Spark 可以作為第二條延伸線：

```text
如果老師想走 AI-native PHY / NVIDIA AI-RAN
  ↓
用 Sionna 做 channel / neural receiver simulation
  ↓
用 TensorRT/CUDA 做 inference acceleration
```

因此可以跟老師這樣說：

> 若研究目標是 O-RAN 架構驗證與 xApp 演算法替換，Open5GS + srsRAN/OCUDU + FlexRIC 是比較直接且可落地的方案。若研究目標轉向 AI-native PHY、neural receiver、ray tracing channel 或 NVIDIA AI-RAN accelerated compute，Sionna / DGX Spark 會比較適合，但它不是完整 5G Core + RIC/xApp testbed 的替代品。

## 6. 建議下午報告結論

1. 短期先走 Open5GS + srsRAN/OCUDU + FlexRIC，因為它最直接對應 O-RAN RIC/xApp demo。
2. 第一階段目標是讓 UE attach、產生 traffic、RIC 收到 KPM、xApp 做簡單控制。
3. 論文案例建議選 PRB allocation / QoS slicing，因為它和 xApp、Near-RT RIC、KPM、control action 都直接相關。
4. Sionna / DGX Spark 比較適合 PHY channel simulation、neural receiver、TensorRT/CUDA inference，不適合作為完整 O-RAN Core/RIC/xApp 平台的主線。
5. 如果老師希望結合 NVIDIA AI-RAN，可以把它放成延伸題：xApp inference acceleration 或 Sionna-based PHY simulation，而不是一開始就替代整套 O-RAN testbed。

## 7. 參考資料

- NVIDIA Sionna: https://developer.nvidia.com/sionna
- Sionna RT introduction: https://nvlabs.github.io/sionna/rt/tutorials/Introduction.html
- Sionna RT research page: https://research.nvidia.com/publication/2023-12_sionna-rt-differentiable-ray-tracing-radio-propagation-modeling
- NVIDIA TensorRT: https://developer.nvidia.com/tensorrt
- NVIDIA TensorRT quick start: https://docs.nvidia.com/deeplearning/tensorrt/latest/getting-started/quick-start-guide.html
- NVIDIA DGX Spark: https://www.nvidia.com/en-us/products/workstations/dgx-spark/
- DGX Spark User Guide: https://docs.nvidia.com/dgx/dgx-spark/index.html
- NVIDIA neural receiver example: https://github.com/NVlabs/neural_rx
