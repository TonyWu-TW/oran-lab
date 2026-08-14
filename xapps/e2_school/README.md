# e2_school：用 FlexRIC 真正的 E2 API 練手

現有 VoiceGuard Python xApp **不走這條路**。它的觀測來自 Experiment Manager / Prometheus，
控制主要是改 traffic pacing。這份練習專門回答：

- FlexRIC 到底露出什麼 API
- E2 訂閱資料怎麼發
- E2 控制指令怎麼下

KPM / RC **沒有 Python SDK**（FlexRIC README 寫明了）。這三支程式是 C。

## FlexRIC 公開 API 只有這些

檔案：`src/flexric/src/xApp/e42_xapp_api.h`

| 函式 | 對應 E2 | 做什麼 |
| --- | --- | --- |
| `init_xapp_api()` | xApp ↔ RIC 連線 | 連 Near-RT RIC（預設 `127.0.0.1`） |
| `e2_nodes_xapp_api()` | E2 Setup / RAN Function 廣告 | 看有哪些 E2 node、各支援哪些 SM |
| `report_sm_xapp_api()` | **Subscription Request** | 訂閱，之後 Indication 進 callback |
| `rm_report_sm_xapp_api()` | Subscription Delete | 取消訂閱 |
| `control_sm_xapp_api()` | **Control Request** | 下一次性控制 |
| `try_stop_xapp_api()` | 離開 | 關掉 xApp |

沒有「Python `subscribe_kpm()`」。KPM 訂閱要自己組 `kpm_sub_data_t`，RC 控制要自己組 `rc_ctrl_req_data_t`。

這套 lab 的 gNB 開了：

```yaml
e2:
  enable_du_e2: true
  e2sm_kpm_enabled: true
  e2sm_rc_enabled: true
  addr: 127.0.0.1
  port: 36421
```

慣例：RAN Function **2 = KPM**，**3 = RC**。先跑 `e2_hello` 確認，不要寫死假設。

## 三支程式

程式在 `src/flexric/examples/xApp/c/e2_school/`。

1. `e2_hello` — 只連 RIC，列出 E2 node 與 RAN Function
2. `e2_kpm_sub` — 訂閱 KPM 20 秒，印 gNB 廣告的 measurement 與 Indication
3. `e2_rc_ctrl` — 送一筆 RC Style 2 / Action 6（預設 `0/100/0`，安全基線）

## 編譯

Near-RT RIC 跟 gNB 要先在跑（Experiment Manager 啟動即可）。

```bash
cd /home/zju/Desktop/oran-lab/src/flexric/build
cmake ..
cmake --build . --target e2_hello e2_kpm_sub e2_rc_ctrl -j"$(nproc)"
```

## 跑

```bash
cd /home/zju/Desktop/oran-lab
./xapps/e2_school/run.sh hello
./xapps/e2_school/run.sh kpm
./xapps/e2_school/run.sh rc
```

建議順序：`hello` → 看到 function 2 和 3 → `kpm` 看 Indication → `rc` 看 ACK。

`e2_rc_ctrl` 可用環境變數改參數：

```bash
E2_SCHOOL_UE_ID=0 E2_SCHOOL_MAX_PRB=100 ./xapps/e2_school/run.sh rc
```

## 讀碼順序

1. `e42_xapp_api.h` — 6 個函式，10 分鐘看完
2. `e2_hello.c` — 連線與發現
3. `e2_kpm_sub.c` 的 `make_subscription()` + `sm_cb_kpm()` — 訂閱與回報
4. `e2_rc_ctrl.c` 的 `make_control()` — 控制樹
5. 對照官方 `examples/xApp/c/monitor/xapp_kpm_moni.c` 與 `voiceguard_rc/voiceguard_rc.c`

官方 Python 例子 `examples/xApp/python3/` 只能訂 MAC/RLC/PDCP/GTP 這些**非標準** SM。OCUDU 這條線開的是標準 KPM/RC，所以 Python 走不通。
