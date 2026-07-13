# Open5GS + OCUDU + FlexRIC 第三階段：GNU Radio Broker 多 UE 運行計畫

整理日期：2026-07-12

## 0. 本階段目標與邊界

Plan 1（`plan.md`）已完成單 UE 的 Open5GS、OCUDU、srsUE、FlexRIC 與 xApp 路線；Plan 2（`plan2.md`）已完成 Prometheus、Grafana、UE exporter 與 KPM 路線驗證。

第三階段只做一件事：

> 在不更換目前 Open5GS、OCUDU、FlexRIC 與 srsUE 主線的前提下，加入 srsRAN 官方 GNU Radio Broker，讓一個 OCUDU gNB 可以同時連接多個完整 srsUE。

本階段最小成功目標固定為 3 台 UE：

```text
Open5GS
  ↕ N2 / N3
OCUDU gNB（同一個 cell）
  ↕ ZMQ I/Q
GNU Radio Broker
  ├─ srsUE1 / ue1 / IMSI ...001
  ├─ srsUE2 / ue2 / IMSI ...002
  └─ srsUE3 / ue3 / IMSI ...003
```

三台 UE 跑通後，才依序測 5 台與 10 台。

本階段明確不做：

- 不做 React、Vue、FastAPI 或任何網站。
- 不做 Broker Web API、XMLRPC 或网页控制。
- 不做新的 Grafana dashboard 或视觉化页面。
- 不做自訂 Python xApp 演算法。
- 不做实体 O-RU、USRP 或 O-RAN Split 7.2。
- 不重做 Plan 1、Plan 2 已经完成的环境安装。

GNU Radio Companion 的窗口只用来打开并执行官方 flowgraph，不把 GUI 美化列为本阶段任务。

## 1. 先固定目前已经跑通的基线

目前已验证的单 UE 基线如下：

| 项目 | 当前值 |
| --- | --- |
| Lab root | `/home/zju/Desktop/oran-lab` |
| AMF | `127.0.0.5:38412` |
| gNB N3 bind | `127.0.0.1` |
| PLMN | `99970` |
| TAC | `1` |
| Slice | `sst: 1` |
| NR band | Band 3 |
| DL ARFCN | `368500` |
| SSB ARFCN | `368410` |
| Bandwidth | `20 MHz` |
| SCS | `15 kHz` |
| PRB | `106` |
| Sample rate | `23.04e6` |
| gNB ZMQ TX/RX | `2000 / 2001` |
| FlexRIC E2 | `127.0.0.1:36421` |
| APN / DNN | `internet` |
| UE subnet | `10.45.0.0/16` |

第三阶段不得直接覆盖以下已成功档案：

```text
config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
config/srsue/ue-zmq-open5gs.conf
start_stage1.sh
stop_stage1.sh
status_stage1.sh
```

所有多 UE 修改都建立新档案。这样失败时可以立刻回到单 UE 基线。

## 2. Broker 在本阶段的角色

现在的单 UE ZMQ 是点对点：

```text
gNB TX 2000 ──> UE RX 2000
gNB RX 2001 <── UE TX 2001
```

多 UE 时，GNU Radio Broker 负责：

1. 将 gNB downlink I/Q samples 复制给每台 UE。
2. 将每台 UE uplink I/Q samples 按相同时间轴加总后送给 gNB。
3. 为每台 UE 提供独立 ZMQ ports。
4. 保留后续增加 path loss 或 channel model 的可能性，但第三阶段先不扩充这些功能。

目标拓扑：

```text
                         ┌── UE1 RX 2100 / TX 2101
gNB 2000/2001 ↔ Broker ──├── UE2 RX 2200 / TX 2201
                         └── UE3 RX 2300 / TX 2301
```

Broker 不是 Open5GS、不是 gNB，也不是 xApp。它只处理 I/Q samples。

## 3. 第三阶段预期新增的档案

完成后目录应至少包含：

```text
oran-lab/
├─ radio/
│  └─ broker/
│     ├─ upstream/
│     │  └─ multi_ue_scenario.grc
│     ├─ flows/
│     │  └─ multi_ue_3.grc
│     ├─ build/
│     └─ README.md
├─ config/
│  ├─ ocudu/
│  │  └─ gnb-fdd-srsue-zmq-open5gs-multiue.yml
│  └─ srsue/
│     └─ multiue/
│        ├─ ue1.conf
│        ├─ ue2.conf
│        └─ ue3.conf
├─ logs/
│  └─ stage3/
├─ run/
│  └─ stage3/
├─ start_stage3.sh
├─ stop_stage3.sh
└─ status_stage3.sh
```

`start_stage3.sh` 等脚本应在手动跑通三台 UE 后才建立，不能一开始就用脚本隐藏问题。

## 4. 变更前检查与备份

在 Ubuntu lab 主机执行：

```bash
cd /home/zju/Desktop/oran-lab

git status
git branch --show-current

mkdir -p backup/stage3
cp config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml \
  backup/stage3/gnb-singleue.$(date +%F-%H%M%S).yml
cp config/srsue/ue-zmq-open5gs.conf \
  backup/stage3/ue-singleue.$(date +%F-%H%M%S).conf
```

确认单 UE 基线仍可执行：

```bash
./start_stage1.sh
./status_stage1.sh
./stop_stage1.sh
```

如果单 UE 基线此时已经失败，先修复基线，不要同时加入 Broker。

## 5. 安装 GNU Radio

GNU Radio 是 GPL-3.0 开源软件。Ubuntu 不需要 clone GNU Radio source，先使用 apt package：

```bash
sudo apt update
sudo apt install -y gnuradio
```

确认版本与 ZeroMQ blocks。GNU Radio Companion 3.10.1.1 不支持
`gnuradio-companion --version`，因此版本统一使用
`gnuradio-config-info --version` 检查：

```bash
gnuradio-config-info --version
gnuradio-config-info --enabled-components | tr ';' '\n' | grep -Ei 'zeromq|gr-zeromq'

/usr/bin/python3 - <<'PY'
from gnuradio import gr, zeromq
print("GNU Radio version:", gr.version())
print("GNU Radio runtime and ZeroMQ blocks are available")
PY
```

本实验机目前的默认 `python3` 是
`/home/zju/anaconda3/bin/python3`（Python 3.12），而 Ubuntu apt 安装的
GNU Radio Python 模块属于 `/usr/bin/python3`。因此 Plan 3 中所有 GNU Radio
flowgraph 的检查与执行都必须明确使用 `/usr/bin/python3`，不要使用默认的
Anaconda `python3`，也不要把两个 Python 环境的 `site-packages` 混在一起。

可以用以下指令再次确认解释器顺序：

```bash
which -a python3
head -n 1 "$(command -v gnuradio-companion)"
/usr/bin/python3 -c 'from gnuradio import gr, zeromq; print(gr.version())'
```

本机已于实际检查中得到：

```text
GNU Radio version: 3.10.1.1
GNU Radio runtime and ZeroMQ blocks are available
```

这表示 GNU Radio runtime 与 `gr-zeromq` 已通过安装检查。若未来
`/usr/bin/python3` 也出现 `ModuleNotFoundError`，才需要检查或修复 Ubuntu
package；不要因为 Anaconda 的 import 失败而重装 GNU Radio。

## 6. 下载并保留官方 Broker 范例

建立目录：

```bash
cd /home/zju/Desktop/oran-lab
mkdir -p radio/broker/upstream radio/broker/flows radio/broker/build
```

下载 srsRAN 官方 flowgraph：

```bash
curl -fL \
  https://docs.srsran.com/projects/project/en/latest/_downloads/0e089ea8fa3a22bf1eec673ab3dffd94/multi_ue_scenario.grc \
  -o radio/broker/upstream/multi_ue_scenario.grc

cp radio/broker/upstream/multi_ue_scenario.grc \
   radio/broker/flows/multi_ue_3.grc
```

检查：

```bash
ls -lh radio/broker/upstream/multi_ue_scenario.grc
sha256sum radio/broker/upstream/multi_ue_scenario.grc
```

`upstream` 内的原档不修改，所有调整只做在 `flows/multi_ue_3.grc`。

第一次打开：

```bash
gnuradio-companion /home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc
```

如果 GRC 提示旧版格式转换，另存回 `flows/multi_ue_3.grc`，不要覆盖 `upstream` 原档。

### 6.1 目前画面代表什么

只要窗口标题显示的是：

```text
/home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc
```

而且画面中能看到 `ZMQ REQ Source`、`ZMQ REP Sink`、`Throttle`、
`Multiply Const` 和 `Add`，就表示官方 Flowgraph 已正确下载并能被 GRC
解析。右侧的 block 分类清单只是 GNU Radio 元件库，本阶段不需要从那里新增
任何 block。

这个 Flowgraph 只处理 gNB 与 UE 之间的 complex I/Q samples，不保存 IMSI、
IMEI、K、OPc、APN 或 namespace。那些身份参数在第 9、10 节处理，不要在
GRC 画面里寻找。

官方图的资料流如下：

```text
Downlink:
gNB TX 2000 -> Throttle -> 三份 Multiply Const -> UE RX 2100/2200/2300

Uplink:
UE TX 2101/2201/2301 -> 三份 Multiply Const -> Add -> gNB RX 2001
```

画面中的 `Multiply Const` 数值 `1`、约 `0.316`、`0.1` 分别来自官方
UE1/UE2/UE3 的 `0/10/20 dB` path loss。它们不是 port，也不是错误；第一轮
不要修改这些方块或右侧的 Pathloss 控制。

### 6.2 现在在 GRC 只修改一个值

目前专案内已经跑通的 gNB 和 srsUE 都是 `20 MHz / 23.04e6`，但官方
Flowgraph 是 `10 MHz / 11.52e6`。因此现在只修改 `samp_rate`：

1. 在画面上方找到 `Variable` 方块，确认 `ID: samp_rate`、`Value: 11.52M`。
2. 双击该方块。
3. 将 Value 从 `11520000` 改成 `23040000`。
4. 按 OK。
5. 确认方块显示 `Value: 23.04M`。
6. 确认 `Throttle` 自动由 `2.88M` 变成 `5.76M`。
7. 按 `Ctrl+S` 保存；窗口标题前面的 `*` 应该消失。

`Throttle` 之所以显示 `5.76M`，是因为官方公式是：

```python
1.0 * samp_rate / (1.0 * slow_down_ratio)
```

而 `slow_down_ratio` 默认是 `4`。这里不要把 Throttle 直接改成
`23.04e6`，也不要修改 `slow_down_ratio`。

从目前画面已经确认官方 port 拓扑正确，以下方块都不要修改：

| Flowgraph 角色 | Block | Address |
| --- | --- | --- |
| 接收 gNB Downlink | ZMQ REQ Source | `tcp://127.0.0.1:2000` |
| 发送给 UE1 | ZMQ REP Sink | `tcp://127.0.0.1:2100` |
| 发送给 UE2 | ZMQ REP Sink | `tcp://127.0.0.1:2200` |
| 发送给 UE3 | ZMQ REP Sink | `tcp://127.0.0.1:2300` |
| 接收 UE1 Uplink | ZMQ REQ Source | `tcp://127.0.0.1:2101` |
| 接收 UE2 Uplink | ZMQ REQ Source | `tcp://127.0.0.1:2201` |
| 接收 UE3 Uplink | ZMQ REQ Source | `tcp://127.0.0.1:2301` |
| 发送给 gNB | ZMQ REP Sink | `tcp://127.0.0.1:2001` |

保存后在 terminal 检查，不需要继续拖动或重新接线：

```bash
FLOW=/home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc

grep -nE "23040000|23.04e6|11520000" "$FLOW"
grep -oE 'tcp://127\.0\.0\.1:[0-9]+' "$FLOW" | sort -Vu
```

第一条输出必须能看到 `23040000` 或 `23.04e6`，而且不能再看到
`11520000`。第二条应该正好列出：

```text
tcp://127.0.0.1:2000
tcp://127.0.0.1:2001
tcp://127.0.0.1:2100
tcp://127.0.0.1:2101
tcp://127.0.0.1:2200
tcp://127.0.0.1:2201
tcp://127.0.0.1:2300
tcp://127.0.0.1:2301
```

到这里关闭 GRC 即可，**现在不要按 Execute/播放键**。Flowgraph 要等到
Open5GS、gNB 和三台 UE 都启动后，才在第 14 节最后执行。

## 7. 固定三台 UE 的身份、port 与档名（本节只确认规划）

本节不需要操作 GNU Radio Companion，也还不需要输入命令。这里只是先固定
后面建立三份 srsUE config 与 Open5GS subscriber 时要使用的数值；实际档案
在第 9 节建立，Open5GS subscriber 在第 10 节建立。

第三阶段先固定以下规划，不在测试过程中临时换值：

| UE | IMSI | IMEI | RX port | TX port | namespace | log |
| --- | --- | --- | ---: | ---: | --- | --- |
| UE1 | `999700000000001` | `353490069873319` | 2100 | 2101 | `ue1` | `/tmp/ue1.log` |
| UE2 | `999700000000002` | `353490069873327` | 2200 | 2201 | `ue2` | `/tmp/ue2.log` |
| UE3 | `999700000000003` | `353490069873335` | 2300 | 2301 | `ue3` | `/tmp/ue3.log` |

三台 UE 可先共用目前实验用的 K 与 OPc：

```text
K   = 00112233445566778899AABBCCDDEEFF
OPc = 63BFA50EE6523365FF14C1F45F88737D
AMF = 8000
APN = internet
SST = 1
```

IMSI 必须不同。namespace、log、PCAP 与 ZMQ port 也必须不同。

表格里的 RX/TX 是从 UE 角度命名：

- UE1 `rx_port=2100` 对应 Flowgraph 的 `ZMQ REP Sink 2100`，用于接收下行。
- UE1 `tx_port=2101` 对应 Flowgraph 的 `ZMQ REQ Source 2101`，用于送出上行。
- UE2、UE3 依相同规则分别使用 `2200/2201`、`2300/2301`。

因此 GRC 只需要知道这些 port，不需要知道每台 UE 的 IMSI 或密钥。

## 8. 建立多 UE gNB config

先从已成功的 gNB config 复制，不直接改原档：

```bash
cd /home/zju/Desktop/oran-lab

cp config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml \
   config/ocudu/gnb-fdd-srsue-zmq-open5gs-multiue.yml
```

第一轮保留现有 20 MHz 基线：

```yaml
ru_sdr:
  device_driver: zmq
  device_args: tx_port=tcp://*:2000,rx_port=tcp://localhost:2001,id=gnb,base_srate=23.04e6
  srate: 23.04

cell_cfg:
  dl_arfcn: 368500
  band: 3
  channel_bandwidth_MHz: 20
  common_scs: 15
  plmn: "99970"
  tac: 1
```

gNB ports 仍然是 2000/2001，因为现在连接对象从单一 UE 改成 Broker。

在现有 `cell_cfg.prach` 下加入多 UE Random Access 参数：

```yaml
  prach:
    prach_config_index: 1
    total_nof_ra_preambles: 64
    nof_ssb_per_ro: 1
    nof_cb_preambles_per_ssb: 64
```

保留现有 E2/KPM：

```yaml
e2:
  enable_du_e2: true
  e2sm_kpm_enabled: true
  e2sm_rc_enabled: true
  addr: 127.0.0.1
  port: 36421
  bind_addr: 127.0.0.1
```

先让 OCUDU 验证 config schema：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs-multiue.yml
```

看到 gNB 正常启动后用 `Ctrl+C` 停止。若 OCUDU 当前版本拒绝新增的 PRACH 字段，先记录准确错误，再用当前 repo 的 configuration reference 对齐字段名；不要修改其他已成功的 AMF、E2、band 或 ARFCN 参数。

## 9. 建立三份 srsUE config

建立目录并复制：

```bash
cd /home/zju/Desktop/oran-lab
mkdir -p config/srsue/multiue

cp config/srsue/ue-zmq-open5gs.conf config/srsue/multiue/ue1.conf
cp config/srsue/ue-zmq-open5gs.conf config/srsue/multiue/ue2.conf
cp config/srsue/ue-zmq-open5gs.conf config/srsue/multiue/ue3.conf
```

### 9.1 UE1 必改参数

```ini
[rf]
srate = 23.04e6
device_name = zmq
device_args = tx_port=tcp://*:2101,rx_port=tcp://localhost:2100,id=ue1,base_srate=23.04e6

[usim]
mode = soft
algo = milenage
opc  = 63BFA50EE6523365FF14C1F45F88737D
k    = 00112233445566778899AABBCCDDEEFF
imsi = 999700000000001
imei = 353490069873319

[gw]
netns = ue1
ip_devname = tun_srsue

[log]
filename = /tmp/ue1.log

[pcap]
mac_filename = /tmp/ue1_mac.pcap
mac_nr_filename = /tmp/ue1_mac_nr.pcap
nas_filename = /tmp/ue1_nas.pcap

[gui]
enable = false
```

### 9.2 UE2 必改参数

```ini
[rf]
srate = 23.04e6
device_name = zmq
device_args = tx_port=tcp://*:2201,rx_port=tcp://localhost:2200,id=ue2,base_srate=23.04e6

[usim]
opc  = 63BFA50EE6523365FF14C1F45F88737D
k    = 00112233445566778899AABBCCDDEEFF
imsi = 999700000000002
imei = 353490069873327

[gw]
netns = ue2
ip_devname = tun_srsue

[log]
filename = /tmp/ue2.log

[pcap]
mac_filename = /tmp/ue2_mac.pcap
mac_nr_filename = /tmp/ue2_mac_nr.pcap
nas_filename = /tmp/ue2_nas.pcap

[gui]
enable = false
```

### 9.3 UE3 必改参数

```ini
[rf]
srate = 23.04e6
device_name = zmq
device_args = tx_port=tcp://*:2301,rx_port=tcp://localhost:2300,id=ue3,base_srate=23.04e6

[usim]
opc  = 63BFA50EE6523365FF14C1F45F88737D
k    = 00112233445566778899AABBCCDDEEFF
imsi = 999700000000003
imei = 353490069873335

[gw]
netns = ue3
ip_devname = tun_srsue

[log]
filename = /tmp/ue3.log

[pcap]
mac_filename = /tmp/ue3_mac.pcap
mac_nr_filename = /tmp/ue3_mac_nr.pcap
nas_filename = /tmp/ue3_nas.pcap

[gui]
enable = false
```

三份 config 的这些无线参数必须继续一致：

```ini
[rat.nr]
bands = 3
nof_carriers = 1
max_nof_prb = 106
nof_prb = 106
dl_nr_arfcn = 368500
ssb_nr_arfcn = 368410
scs = 15
ssb_scs = 15

[nas]
apn = internet
apn_protocol = ipv4
```

检查重复与漏改：

```bash
for f in /home/zju/Desktop/oran-lab/config/srsue/multiue/*.conf; do
  echo "===== $f ====="
  grep -nE 'device_args|srate =|imsi =|imei =|netns =|filename =|enable = false' "$f"
done
```

特别确认没有任何新 config 仍使用 UE 直连 port `2000/2001`。

## 10. 在 Open5GS 注册 UE2 与 UE3

UE1 已在 Plan 1 注册。进入 Open5GS WebUI：

```text
http://localhost:9999
username: admin
password: 1423
```

新增：

```text
UE2
IMSI: 999700000000002
K:    00112233445566778899AABBCCDDEEFF
OPc:  63BFA50EE6523365FF14C1F45F88737D
AMF:  8000
APN:  internet
SST:  1
Subscriber Status: SERVICE_GRANTED
```

```text
UE3
IMSI: 999700000000003
K:    00112233445566778899AABBCCDDEEFF
OPc:  63BFA50EE6523365FF14C1F45F88737D
AMF:  8000
APN:  internet
SST:  1
Subscriber Status: SERVICE_GRANTED
```

也可以使用 repo 内的 `open5gs-dbctl`，但第三阶段第一次建议用 WebUI，避免数据库格式或参数顺序错误。完成后确认 WebUI 同时存在三笔 IMSI。

## 11. 建立 UE network namespaces

以下命令可重复执行：

```bash
for ns in ue1 ue2 ue3; do
  sudo ip netns list | awk '{print $1}' | grep -qx "$ns" || sudo ip netns add "$ns"
done

sudo ip netns list
```

三台 UE 内都可以使用同名 `tun_srsue`，因为它们位于不同 namespace，不会冲突。

保留 Plan 1 已验证的 forwarding/NAT：

```bash
sudo sysctl -w net.ipv4.ip_forward=1

sudo iptables -t nat -C POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE 2>/dev/null || \
sudo iptables -t nat -A POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE

sudo iptables -C FORWARD -s 10.45.0.0/16 -i ogstun -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 1 -s 10.45.0.0/16 -i ogstun -j ACCEPT

sudo iptables -C FORWARD -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 2 -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
```

## 12. 调整 GNU Radio flowgraph

第 6 节已经完成 `samp_rate` 修改与 port 检查。本节是建立完 gNB、三份 UE
config 和 namespace 后的最终复查，不要重新设计 Flowgraph，也不要交换
REQ/REP 或重新接线。

打开：

```bash
gnuradio-companion /home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc
```

按顺序复查：

1. gNB TX/RX 使用官方的 `2000/2001`。
2. UE1 使用 `2100/2101`。
3. UE2 使用 `2200/2201`。
4. UE3 使用 `2300/2301`。
5. 所有 stream type 都是 complex float。
6. 所有 sample rate 统一为 `23.04e6`。
7. Downlink 有三条复制路径。
8. Uplink 三条路径经过 Add 后才送回 gNB。
9. 第一轮不要增加 AWGN、fading、delay、FFT 或额外 GUI block。
10. 第一轮保留官方 `0/10/20 dB` path loss，不在 attach 过程中拖动滑杆。

先用文字检查确保没有遗留官方 10 MHz 的 `11520000`：

```bash
FLOW=/home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc
if grep -n '11520000' "$FLOW"; then
  echo "ERROR: Flowgraph 仍是官方 10 MHz sample rate"
else
  echo "OK: 未发现 11.52e6"
fi
```

保存后先让 GRC 产生 Python：

```bash
grcc \
  -d /home/zju/Desktop/oran-lab/radio/broker/build \
  /home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc
```

检查输出：

```bash
find /home/zju/Desktop/oran-lab/radio/broker/build -maxdepth 1 -type f -print
/usr/bin/python3 -m py_compile /home/zju/Desktop/oran-lab/radio/broker/build/*.py
```

如果官方 graph 仍包含 Qt GUI，第一次可直接在 GNU Radio Companion 按 Execute。第三阶段跑通后再复制一个 No GUI 版本用于脚本，不在第一轮同时改 GUI 架构。

## 13. 启动前 port 与残留程序检查

旧的 Stage 1 不可和 Stage 3 同时运行：

```bash
cd /home/zju/Desktop/oran-lab
./stop_stage1.sh || true

pgrep -af 'nearRT-RIC|/gnb|srsue|multi_ue|gnuradio' || true
sudo ss -lntp | grep -E ':(2000|2001|2100|2101|2200|2201|2300|2301)\b' || true
```

若 port 已被旧 process 占用，先确认 PID 后正常停止，不能直接继续启动第二份。

建议执行 OCUDU performance 设置：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
sudo ./scripts/ocudu_performance
```

建立日志目录：

```bash
mkdir -p /home/zju/Desktop/oran-lab/logs/stage3
mkdir -p /home/zju/Desktop/oran-lab/run/stage3
```

## 14. 第一次手动启动顺序

第一次必须使用多个 terminal 手动启动，确认每一层的错误来自哪里。

### Terminal 1：确认 MongoDB 与 Open5GS

```bash
docker start open5gs-mongodb 2>/dev/null || true

sudo systemctl start open5gs-nrfd open5gs-amfd open5gs-smfd open5gs-upfd
sudo systemctl status open5gs-nrfd open5gs-amfd open5gs-smfd open5gs-upfd --no-pager

sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd -f -l
```

### Terminal 2：启动 FlexRIC Near-RT RIC

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/ric/nearRT-RIC \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/nearRT-RIC.log
```

### Terminal 3：启动多 UE gNB

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
sudo ./gnb \
  -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs-multiue.yml \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/gnb.log
```

先确认：

```text
gNB started
N2 connected to AMF
E2 connected to Near-RT RIC
```

### Terminal 4：启动 UE1

```bash
cd /home/zju/Desktop/oran-lab/src/srsRAN_4G/build/srsue/src
sudo ./srsue /home/zju/Desktop/oran-lab/config/srsue/multiue/ue1.conf \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/ue1.log
```

### Terminal 5：启动 UE2

```bash
cd /home/zju/Desktop/oran-lab/src/srsRAN_4G/build/srsue/src
sudo ./srsue /home/zju/Desktop/oran-lab/config/srsue/multiue/ue2.conf \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/ue2.log
```

### Terminal 6：启动 UE3

```bash
cd /home/zju/Desktop/oran-lab/src/srsRAN_4G/build/srsue/src
sudo ./srsue /home/zju/Desktop/oran-lab/config/srsue/multiue/ue3.conf \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/ue3.log
```

Broker 尚未启动前，UE process 可以处于等待 I/Q samples 状态，不应期待 attach。

### Terminal 7：最后启动 GNU Radio Broker

官方顺序是 Open5GS、gNB、所有 UE，最后执行 flowgraph。

开发测试方式：

```bash
gnuradio-companion /home/zju/Desktop/oran-lab/radio/broker/flows/multi_ue_3.grc
```

打开后按 Execute。

如果已产生可直接执行的 Python 且不依赖额外 GUI 操作：

```bash
/usr/bin/python3 /home/zju/Desktop/oran-lab/radio/broker/build/multi_ue_3.py \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/broker.log
```

### Terminal 8：可选的既有 KPM monitor

这不是新 xApp 开发，只用于确认多 UE 后原本 E2/KPM 路线没有坏：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/xApp/c/monitor/xapp_kpm_moni \
  2>&1 | tee /home/zju/Desktop/oran-lab/logs/stage3/xapp_kpm_moni.log
```

## 15. 三台 UE 的验收命令

### 15.1 检查 attach 与 PDU Session

```bash
grep -EH 'Random Access Complete|RRC Connected|PDU Session Establishment successful|IP:' \
  /home/zju/Desktop/oran-lab/logs/stage3/ue*.log
```

三台 UE 都必须出现：

```text
Random Access Complete
RRC Connected
PDU Session Establishment successful
```

### 15.2 检查 namespace 与 TUN

```bash
for ns in ue1 ue2 ue3; do
  echo "===== $ns ====="
  sudo ip netns exec "$ns" ip -brief addr show tun_srsue
  sudo ip netns exec "$ns" ip route
done
```

如果 UE 已有 IP 但没有 default route：

```bash
for ns in ue1 ue2 ue3; do
  sudo ip netns exec "$ns" ip route replace default dev tun_srsue
done
```

### 15.3 检查三台 UE 到 UPF

```bash
for ns in ue1 ue2 ue3; do
  echo "===== ping from $ns ====="
  sudo ip netns exec "$ns" ping -c 3 -W 2 10.45.0.1
done
```

### 15.4 检查 Internet

```bash
for ns in ue1 ue2 ue3; do
  echo "===== internet from $ns ====="
  sudo ip netns exec "$ns" ping -c 3 -W 2 8.8.8.8
done
```

### 15.5 检查 Open5GS

```bash
sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd \
  --since '-10 min' --no-pager | \
  grep -E '99970000000000[123]|Number of gNB-UEs|AMF-UEs|UPF-Sessions|SMF-Sessions'
```

### 15.6 检查 gNB 与 KPM

```bash
grep -E 'rnti|RNTI|ue=|UE=' /home/zju/Desktop/oran-lab/logs/stage3/gnb.log | tail -50
grep -E 'RNTI|measurement|KPM ind_msg' \
  /home/zju/Desktop/oran-lab/logs/stage3/xapp_kpm_moni.log | tail -50
```

目标是确认 gNB／KPM 中出现多个不同 UE context 或 RNTI，不只是三个 process running。

### 15.7 最小流量测试

```bash
iperf3 -s -B 10.45.0.1 -p 5201
```

分别从 UE 测试，第一轮先不要三台同时压满：

```bash
sudo ip netns exec ue1 iperf3 -c 10.45.0.1 -p 5201 -t 15 -i 1
sudo ip netns exec ue2 iperf3 -c 10.45.0.1 -p 5201 -t 15 -i 1
sudo ip netns exec ue3 iperf3 -c 10.45.0.1 -p 5201 -t 15 -i 1
```

三台分别成功后，再做小流量并发测试，例如每台 UDP 2 Mbps，而不是一开始就 30 Mbps：

```bash
sudo ip netns exec ue1 iperf3 -c 10.45.0.1 -p 5201 -u -b 2M -t 30 &
sudo ip netns exec ue2 iperf3 -c 10.45.0.1 -p 5201 -u -b 2M -t 30 &
sudo ip netns exec ue3 iperf3 -c 10.45.0.1 -p 5201 -u -b 2M -t 30 &
wait
```

## 16. 停止顺序

停止时使用启动的反方向：

1. 停止 iperf traffic。
2. 停止 KPM xApp。
3. 停止 GNU Radio Broker。
4. 停止 UE3、UE2、UE1。
5. 停止 gNB。
6. 停止 Near-RT RIC。
7. Open5GS 可保留运行，或最后再停。

停止后检查残留：

```bash
pgrep -af 'nearRT-RIC|/gnb|srsue|multi_ue|gnuradio' || true
sudo ss -lntp | grep -E ':(2000|2001|2100|2101|2200|2201|2300|2301)\b' || true
```

namespace 可保留供下次使用。只有确定要清理时才执行：

```bash
for ns in ue1 ue2 ue3; do
  sudo ip netns del "$ns" 2>/dev/null || true
done
```

## 17. 常见问题与判断顺序

### 17.1 `Address already in use`

```bash
sudo ss -lntp | grep -E ':(2000|2001|2100|2101|2200|2201|2300|2301)\b'
pgrep -af 'gnb|srsue|gnuradio|multi_ue'
```

原因通常是旧 gNB、UE 或 Broker 没有停止，或两份 UE config 使用相同 TX port。

### 17.2 UE process running，但找不到 cell

按顺序检查：

1. Broker 是否已经 Execute。
2. flowgraph 是否仍使用 `11.52e6`，而 gNB/UE 使用 `23.04e6`。
3. UE ports 是否与 flowgraph 完全相同。
4. ZMQ REQ/REP、bind/connect 是否被手动翻转。
5. Downlink 是否确实从 gNB 分成三条。
6. gNB、Broker、UE 是否使用 complex float。

### 17.3 只有 UE1 成功，UE2/UE3 失败

优先检查：

- UE2/UE3 是否已加入 Open5GS subscriber DB。
- 三份 IMSI 是否真的不同。
- UE2/UE3 是否仍错误使用 `ue1` namespace。
- UE2/UE3 log/PCAP 是否覆盖 UE1。
- Broker 是否真的有 UE2/UE3 的上下行 branch。
- PRACH preamble 参数是否成功被 OCUDU 接受。

### 17.4 `RRC Connected` 后马上 `RRC Release`

通常是 Open5GS subscriber 的 IMSI、K、OPc、APN 或 slice 不一致：

```bash
sudo journalctl -u open5gs-amfd -u open5gs-smfd --since '-5 min' --no-pager
```

查找：

```text
Unknown UE by SUCI
Cannot find IMSI in DB
Authentication failure
PDU Session Reject
```

### 17.5 GNU Radio underflow、late 或 CPU 满载

先做：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
sudo ./scripts/ocudu_performance

htop
```

并确认：

- srsUE GUI 已关闭。
- 第一轮不要开启 PCAP。
- log level 不要全部设为 debug。
- 不要同时运行额外 FFT、Waterfall 或大量 GUI sinks。
- 先只跑三台 UE。

如果 20 MHz 三 UE 仍无法稳定，才切换为 10 MHz fallback profile，必须同时修改所有元件：

| 参数 | 20 MHz | 10 MHz fallback |
| --- | ---: | ---: |
| gNB `channel_bandwidth_MHz` | 20 | 10 |
| gNB `srate` | 23.04 | 11.52 |
| gNB `base_srate` | 23.04e6 | 11.52e6 |
| UE `srate` | 23.04e6 | 11.52e6 |
| UE `base_srate` | 23.04e6 | 11.52e6 |
| UE `max_nof_prb` | 106 | 52 |
| UE `nof_prb` | 106 | 52 |
| Broker sample rate | 23.04e6 | 11.52e6 |

10 MHz 应另存 `*-10mhz` config，不覆盖 20 MHz config。不要只改 Broker 或只改 UE。

### 17.6 Broker 启动但没有数据流动

检查 GNU Radio terminal 是否有 ZMQ timeout、REQ/REP state 或连接错误；同时检查：

```bash
sudo ss -ntp | grep -E ':(2000|2001|2100|2101|2200|2201|2300|2301)\b'
```

必须看到 gNB、Broker 与三台 UE 之间建立对应 TCP connections。

## 18. 三台成功后扩充到 5 台与 10 台

扩充顺序固定：

```text
3 UE 完整通过 → 5 UE 完整通过 → 10 UE 实验
```

port 规划继续使用每台加 100：

| UE | IMSI 结尾 | RX | TX | namespace |
| --- | --- | ---: | ---: | --- |
| UE1 | 001 | 2100 | 2101 | ue1 |
| UE2 | 002 | 2200 | 2201 | ue2 |
| UE3 | 003 | 2300 | 2301 | ue3 |
| UE4 | 004 | 2400 | 2401 | ue4 |
| UE5 | 005 | 2500 | 2501 | ue5 |
| UE6 | 006 | 2600 | 2601 | ue6 |
| UE7 | 007 | 2700 | 2701 | ue7 |
| UE8 | 008 | 2800 | 2801 | ue8 |
| UE9 | 009 | 2900 | 2901 | ue9 |
| UE10 | 010 | 3000 | 3001 | ue10 |

每增加一台 UE，都必须同时完成：

1. Open5GS 新 subscriber。
2. 新 IMSI 与有效 IMEI。
3. 新 UE config。
4. 新 namespace。
5. 新 log/PCAP 名称。
6. Broker 新增一条 downlink copy branch。
7. Broker uplink Add 增加一个输入。
8. 新 ZMQ RX/TX ports。
9. attach、ping、PDU Session 单独验收。

官方明确说明 GNU Radio 多 UE flowgraph 是教学与实验方案，不是高效、无限扩充的商用 UE simulator。因此 10 UE 是性能实验目标，不应在 3 UE 尚未稳定时直接跳过去。

## 19. 手动跑通后才建立 Stage 3 scripts

三台 UE 连续重启三次都成功后，再建立：

```text
start_stage3.sh
stop_stage3.sh
status_stage3.sh
```

脚本要求：

- 不修改或调用 `start_stage1.sh` 的单 UE 逻辑。
- 启动前检查 port 与残留 PID。
- 不使用固定 `sleep` 作为唯一健康判断。
- Open5GS、RIC、gNB、UE、Broker 各自有独立 log/PID。
- Broker 必须最后启动。
- 停止顺序必须反向。
- status 必须逐台检查 UE attach、namespace、TUN 与 ping。
- 重复执行 start 时必须拒绝第二次启动，不能删除运行中的 PID。

在 Broker 仍需手动按 GRC Execute 时，先不要假装已经“一键启动”。要完成真正脚本化，必须先产生并验证可无 GUI 执行的 Python flowgraph。

## 20. 第三阶段完成标准

以下全部通过才算 Plan 3 完成：

- [x] GNU Radio 3.10.1.1 与 ZeroMQ blocks 可用（使用 `/usr/bin/python3`）。
- [ ] 官方 `multi_ue_scenario.grc` 已保留在 `upstream`。
- [ ] OCUDU 多 UE config 没有覆盖单 UE config。
- [ ] 三份 srsUE config 的 IMSI、ports、namespace、log 都不同。
- [ ] Open5GS 有三笔正确 subscriber。
- [ ] gNB 同时连接 Open5GS 与 FlexRIC。
- [ ] Broker 能建立 gNB 与三台 UE 的 ZMQ connections。
- [ ] UE1、UE2、UE3 都完成 Random Access。
- [ ] UE1、UE2、UE3 都进入 RRC Connected。
- [ ] UE1、UE2、UE3 都建立 PDU Session 并取得不同 IP。
- [ ] 三个 namespace 都有 `tun_srsue`。
- [ ] 三台 UE 都能 ping `10.45.0.1`。
- [ ] 三台 UE 都能分别通过 iperf3 产生流量。
- [ ] gNB 或 KPM log 能看到多个 UE/RNTI。
- [ ] 完整停止后没有残留 gNB、srsUE、Broker process 或占用 ports。
- [ ] 单 UE Stage 1 仍然可以恢复运行。

加分但不是本阶段必要条件：

- [ ] 五台 UE 能稳定 attach 与 ping。
- [ ] 十台 UE 在降低带宽或调校 CPU 后能运行。
- [ ] Broker 可用无 GUI Python process 启动。
- [ ] `start_stage3.sh`、`stop_stage3.sh`、`status_stage3.sh` 可重复执行。

## 21. Plan 3 完成后的下一阶段

Plan 3 完成后才另开新计划讨论：

- 统一管理网站。
- FastAPI/React 控制台。
- Broker runtime 参数控制。
- 多 UE Prometheus exporter 与 Grafana。
- Python xApp 演算法。
- PRB、QoS、slice 或 traffic control。

这些内容不要提前混入第三阶段，否则遇到问题时无法判断是 RF Broker、多 UE、监控、网站还是 xApp 控制造成。

## 22. 参考资料

- srsRAN 官方 Multi-UE Emulation：
  https://docs.srsran.com/projects/project/en/latest/tutorials/source/srsUE/source/index.html#multi-ue-emulation
- srsRAN 官方 GNU Radio flowgraph：
  https://docs.srsran.com/projects/project/en/latest/_downloads/0e089ea8fa3a22bf1eec673ab3dffd94/multi_ue_scenario.grc
- GNU Radio 官方 repository：
  https://github.com/gnuradio/gnuradio
- GNU Radio ZeroMQ blocks：
  https://wiki.gnuradio.org/index.php/Understanding_ZMQ_Blocks
- OCUDU configuration：
  https://docs.ocudu.org/user_manual/configuration_ref.html
- Open5GS subscriber 与 Quickstart：
  https://open5gs.org/open5gs/docs/guide/01-quickstart/

