# Open5GS + OCUDU + Near-RT RIC + xApp 環境建置計畫

整理日期：2026-07-02

## 0. 目標與範圍

這份 plan 只做第一階段：**把新的 O-RAN demo 環境架起來**。

主線固定為：

```text
Open5GS
  ↓ N2 / N3
OCUDU gNB / O-CU / O-DU
  ↓ E2
FlexRIC Near-RT RIC
  ↓
xApp Controller
```

第一階段暫時不寫自己的 Python xApp 演算法。目標先做到：

1. Linux 環境裝好 Docker、build tools、SCTP、ZeroMQ。
2. Open5GS 可以啟動。
3. OCUDU 可以編譯成功。
4. FlexRIC Near-RT RIC 可以啟動。
5. FlexRIC E2 agent emulator 可以連到 Near-RT RIC。
6. OCUDU gNB 可以用設定檔啟動。
7. OCUDU gNB 後續可設定連到 Open5GS AMF。
8. OCUDU gNB 後續可設定透過 E2 連到 Near-RT RIC。
9. example xApp 可以接到 Near-RT RIC。

第二階段才開始寫自己的 xApp Controller，例如 PRB allocation、QoS slicing、traffic control。

## 1. 建議實驗機環境

建議先用單台 Linux server / VM，不要一開始就上 K8s。

| 項目 | 建議 |
| --- | --- |
| OS | Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS x86_64 |
| CPU | 至少 4 cores，建議 8 cores |
| RAM | 至少 16 GB，建議 32 GB |
| Disk | 至少 100 GB |
| GPU | 第一階段不需要 |
| SDR / USRP | 第一階段可先不用；後續要真 UE / OTA 才需要 |
| K8s | 第一階段不需要 |

不建議第一階段用 WSL，因為 SCTP、network namespace、tunnel interface、Docker network 比較容易遇到問題。

## 2. 元件版本決策

這份 plan 只採用新的主線：

| 區塊 | 採用元件 | 角色 |
| --- | --- | --- |
| 5G Core | Open5GS | AMF、SMF、UPF、UDM、AUSF 等 5GC 功能 |
| gNB / CU / DU | OCUDU | O-RAN 5G CU/DU/gNB |
| Near-RT RIC | FlexRIC | 輕量 Near-RT RIC、E2 agent emulator、xApp SDK |
| xApp | FlexRIC example xApp，之後改成自己的 xApp | 接 KPM、做控制決策 |
| Monitoring | Prometheus / Grafana，後續再加 | 收實驗數據，不是第一階段必要 |

OCUDU 官方目前把重點放在 CU/DU/gNB。RIC、SMO、Core 仍然是外部元件，所以這裡用 FlexRIC 和 Open5GS 搭配。

## 3. 建立工作目錄

```bash
mkdir -p /home/zju/Desktop/oran-lab/src
mkdir -p /home/zju/Desktop/oran-lab/config
mkdir -p /home/zju/Desktop/oran-lab/logs
cd /home/zju/Desktop/oran-lab/src
```

建議所有 repo 都放在：

```text
/home/zju/Desktop/oran-lab/src/
```

## 4. 安裝基本套件

```bash
sudo apt update
sudo apt install -y \
  git curl wget ca-certificates gnupg lsb-release software-properties-common \
  build-essential cmake make ninja-build pkg-config ccache \
  net-tools iproute2 iputils-ping iperf3 tcpdump \
  python3 python3-dev python3-pip python3-venv \
  libfftw3-dev libmbedtls-dev libsctp-dev lksctp-tools \
  libyaml-cpp-dev libgtest-dev libzmq3-dev \
  libconfig-dev libconfig++-dev \
  clang-14 lld-14

```


確認 SCTP kernel module：

```bash
sudo modprobe sctp
lsmod | grep sctp
```

成功標準：

```text
lsmod | grep sctp 有輸出。
```

SCTP 很重要，因為 NGAP 與 E2AP 都會用到 SCTP。

可選：安裝封包分析工具。

```bash
sudo DEBIAN_FRONTEND=noninteractive apt install -y tshark
```

## 5. 安裝 Docker 與 Docker Compose

第一階段 FlexRIC / OCUDU 可以先 bare-metal 編譯執行，但 Docker 仍建議先裝好，之後可用來跑輔助服務、Prometheus、Grafana 或 O-RAN SC RIC。

```bash
sudo apt update
sudo apt install -y ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

sudo tee /etc/apt/sources.list.d/docker.sources <<EOF
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}")
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
EOF

sudo apt update
sudo apt install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

讓目前使用者可以不用每次都打 `sudo docker`：

```bash
sudo usermod -aG docker "$USER"
newgrp docker
```

驗證：

```bash
docker run --rm hello-world
docker compose version
```

成功標準：

```text
hello-world 可以成功執行。
docker compose version 有版本輸出。
```

## 6. 下載專案

進入 repo 目錄：

```bash
cd /home/zju/Desktop/oran-lab/src
```

### 6.1 Open5GS

Open5GS 可用 package 安裝；repo 主要用來查文件、設定範例與版本。

```bash
git clone https://github.com/open5gs/open5gs.git
```

### 6.2 OCUDU

OCUDU 是新的 O-RAN CU/DU/gNB 主線。

```bash
git clone https://gitlab.com/ocudu/ocudu.git
```

### 6.3 FlexRIC

FlexRIC 是第一階段使用的 Near-RT RIC 與 xApp SDK。

```bash
git clone https://gitlab.eurecom.fr/mosaic5g/flexric.git
```

## 7. 安裝 Open5GS

第一階段使用 native package Open5GS，這樣後續和 OCUDU 的 AMF / UPF / subscriber 設定比較清楚。

### 7.1 安裝 MongoDB

MongoDB 先不要用系統套件安裝，直接用 Docker 版部署，讓 Open5GS native package 透過本機 `27017` 連線。

```bash
docker volume create open5gs-mongodb-data

docker run -d \
  --name open5gs-mongodb \
  --restart unless-stopped \
  -p 127.0.0.1:27017:27017 \
  -v open5gs-mongodb-data:/data/db \
  mongo:8.0
```

驗證：

```bash
docker ps --filter name=open5gs-mongodb
docker exec open5gs-mongodb mongosh --eval 'db.runCommand({ ping: 1 })'
```

### 7.2 安裝 Open5GS

```bash
sudo add-apt-repository -y ppa:open5gs/latest
sudo apt update
sudo apt install -y open5gs
```

驗證：

```bash
systemctl status open5gs-amfd --no-pager
systemctl status open5gs-smfd --no-pager
systemctl status open5gs-upfd --no-pager
```

### 7.3 安裝 Open5GS WebUI

官方 WebUI 安裝腳本會檢查 host 上是否存在 `/usr/bin/mongod`，所以 MongoDB 改用 Docker 後不要跑這個腳本：

```bash
# 不要使用；它會要求系統版 MongoDB package
# curl -fsSL https://open5gs.org/open5gs/assets/webui/install | sudo -E bash -
```

改用手動安裝 WebUI。先安裝 Node.js：

```bash
sudo apt update
sudo apt install -y ca-certificates curl gnupg
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | \
  sudo gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg

NODE_MAJOR=20
echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | \
  sudo tee /etc/apt/sources.list.d/nodesource.list

sudo apt update
sudo apt install -y nodejs
```

下載並建置 WebUI：

```bash
OPEN5GS_VERSION=2.8.0
cd /tmp
curl -sLf "https://github.com/open5gs/open5gs/archive/v${OPEN5GS_VERSION}.tar.gz" | tar zxf -

cd "/tmp/open5gs-${OPEN5GS_VERSION}/webui"
npm clean-install
npm run build

sudo rm -rf /usr/lib/node_modules/open5gs
sudo mkdir -p /usr/lib/node_modules
sudo cp -a . /usr/lib/node_modules/open5gs
sudo chown -R open5gs:open5gs /usr/lib/node_modules/open5gs
```

建立 systemd service。WebUI 預設也會連 `mongodb://127.0.0.1/open5gs`，這裡明確寫出來，對應前面 Docker MongoDB 的 `127.0.0.1:27017`。

```bash
sudo tee /etc/systemd/system/open5gs-webui.service >/dev/null <<'EOF'
[Unit]
Description=Open5GS WebUI
After=docker.service
Requires=docker.service

[Service]
Type=simple
WorkingDirectory=/usr/lib/node_modules/open5gs
Environment=NODE_ENV=production
Environment=DB_URI=mongodb://127.0.0.1/open5gs
Environment=HOSTNAME=127.0.0.1
Environment=PORT=9999
ExecStart=/usr/bin/node server/index.js
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now open5gs-webui
```

初始化 WebUI 預設管理員帳號：

```bash
docker cp "/tmp/open5gs-${OPEN5GS_VERSION}/docs/assets/webui/mongo-init.js" \
  open5gs-mongodb:/tmp/mongo-init.js
docker exec open5gs-mongodb mongosh open5gs /tmp/mongo-init.js
```

WebUI：

```text
http://localhost:9999
username: admin
password: 1423
```

### 7.4 Open5GS 後續要改的設定

後面要讓 OCUDU gNB 接 Open5GS 時，通常要確認：

| 設定                              | 檔案                      |
| ------------------------------- | ----------------------- |
| PLMN / TAC                      | `/etc/open5gs/amf.yaml` |
| AMF NGAP bind address           | `/etc/open5gs/amf.yaml` |
| UPF GTP-U address               | `/etc/open5gs/upf.yaml` |
| UE subnet                       | `/etc/open5gs/upf.yaml` |
| subscriber IMSI / K / OPc / APN | Open5GS WebUI           |

如果 gNB 和 Open5GS 在同一台機器，可以先用 loopback / local IP；如果分不同機器，AMF NGAP address 和 UPF GTP-U address 要改成實際網卡 IP。

修改後重啟：

```bash
sudo systemctl restart open5gs-nrfd
sudo systemctl restart open5gs-amfd
sudo systemctl restart open5gs-smfd
sudo systemctl restart open5gs-upfd
```

如果要讓 UE 走 UPF 對外連線，需要開 IP forwarding 與 NAT：

```bash
sudo sysctl -w net.ipv4.ip_forward=1

sudo iptables -t nat -C POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE 2>/dev/null || \
sudo iptables -t nat -A POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE
```

如果 Docker 已啟動，IPv4 `FORWARD` policy 可能會被 Docker 設成 `DROP`。這時 UE 封包可以到 `ogstun`，但不會被 forward 到實體網卡，NAT counter 也不會增加。需要補允許 `ogstun` 的 forwarding 規則：

```bash
sudo iptables -C FORWARD -s 10.45.0.0/16 -i ogstun -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 1 -s 10.45.0.0/16 -i ogstun -j ACCEPT

sudo iptables -C FORWARD -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 2 -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
```

## 8. 建置 OCUDU

OCUDU 官方文件建議使用 Linux-based OS，並且實際 radio / realtime 場景需要 realtime kernel support。第一階段先以 ZMQ / lab mode 編譯為主。

### 8.1 編譯 OCUDU with ZMQ

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
mkdir -p build
cd build


//這段沒辦法用
CC=clang-14 CXX=clang++-14 cmake ../ -DENABLE_EXPORT=ON -DENABLE_ZEROMQ=ON -DBUILD_TESTING=OFF
make -j"$(nproc)"

//用這段
cd ~/Desktop/oran-lab/src/ocudu
rm -rf build
mkdir build
cd build

CC=gcc CXX=g++ cmake ../ \
  -DENABLE_EXPORT=ON \
  -DENABLE_ZEROMQ=ON \
  -DBUILD_TESTING=OFF

make -j"$(nproc)"

```

驗證：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
./gnb --help
```

成功標準：

```text
./gnb --help 可以印出參數說明。
CMake output 裡面有找到 ZeroMQ。
```

如果 CMake 沒找到 ZeroMQ，先確認：

```bash
dpkg -l | grep libzmq3-dev
```

然後清掉 build 重編：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
rm -rf build
mkdir build
cd build

env -i HOME="$HOME" USER="$USER" PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  cmake .. \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DENABLE_EXPORT=ON \
    -DENABLE_ZEROMQ=ON \
    -DBUILD_TESTING=OFF \
    -DCMAKE_SKIP_RPATH=ON

make -j"$(nproc)"
```


### 8.2 系統效能設定

OCUDU 提供 performance script，可先在跑 gNB 前執行：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
sudo ./scripts/ocudu_performance
```

這一步主要會調整 CPU governor、network buffers 等參數。

### 8.3 準備 OCUDU gNB 設定檔

OCUDU 的 example config 通常在 repo 的 `configs/` 目錄。先查有哪些設定檔：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
find configs -maxdepth 3 -type f | sort
```

找跟 gNB、ZMQ、Open5GS、E2 相關的 config：
這些config是示範的 可以先用著這樣

```bash
find configs -type f \( -iname "*gnb*" -o -iname "*zmq*" -o -iname "*e2*" -o -iname "*open5gs*" \) | sort
```

建立自己的 lab config 目錄：

```bash
mkdir -p /home/zju/Desktop/oran-lab/config/ocudu
```

複製最接近的 gNB config 到 lab config 目錄，檔名可先統一成：

```bash
cp <OCUDU_EXAMPLE_GNB_CONFIG> /home/zju/Desktop/oran-lab/config/ocudu/gnb-zmq-open5gs-flexric.yml
```

需要確認或修改的重點：

| 設定區塊      | 要確認什麼                                   |
| --------- | --------------------------------------- |
| AMF / N2  | Open5GS AMF IP、port 38412、local bind IP |
| UPF / N3  | Open5GS UPF GTP-U IP                    |
| PLMN      | 和 Open5GS subscriber / AMF 一致           |
| TAC       | 和 Open5GS AMF 一致                        |
| S-NSSAI   | 先用 `sst: 1` 即可                          |
| RF driver | 第一階段若不用 SDR，先走 ZMQ                      |
| E2        | enable E2 agent，RIC IP 指到 FlexRIC       |
| KPM / RC  | enable KPM；RC 後續做 control xApp 再打開      |

注意：OCUDU 的設定欄位以當前 repo 的 configuration reference 為準。

去和ai交互 問一下如何配置設定 基本上要用b210的這個去做

目前 ZMQ + Open5GS 實測可用的 gNB 設定檔是：

```text
/home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
```

其中 N2 / N3 目前使用同機 loopback：

```yaml
cu_cp:
  amf:
    addrs: 127.0.0.5
    port: 38412
    bind_addrs: 127.0.0.1

cu_up:
  ngu:
    socket:
      - bind_addr: 127.0.0.1
```

這裡 `cu_up.ngu.socket.bind_addr: 127.0.0.1` 很重要。Open5GS UPF 目前 `gtpu.server.address` 是 `127.0.0.7`，gNB 的 N3 socket 是 `127.0.0.1:2152`，兩者同機可經 loopback 互通。若 gNB 和 Open5GS 分到不同機器或不同 network namespace，UPF GTP-U address 和 gNB N3 bind address 要改成可互達的實體或容器網路 IP。

也可以在 gNB 設定檔開 N3 pcap：

```yaml
pcap:
  n3_enable: true
  n3_filename: /tmp/gnb_n3.pcap
```

### 8.4 啟動 OCUDU gNB

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
sudo ./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-zmq-open5gs-flexric.yml
```

成功時預期看到類似：

```bash
OCUDU gNB started
Connecting to AMF on <AMF_IP>:38412
Connecting to NearRT-RIC on <RIC_IP>:36421
```

實際 log 文字可能依 OCUDU 版本不同而不同，重點是：

1. gNB process 沒有 crash。
2. AMF connection 有成功或至少有連線嘗試。
3. E2 / Near-RT RIC connection 有成功或至少有連線嘗試。


最後這樣成功了！
```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
```


### 8.5 安裝 srsRAN_4G / srsUE

```bash
cd /home/zju/Desktop/oran-lab/src

sudo apt install -y \
  build-essential cmake libfftw3-dev libmbedtls-dev \
  libboost-program-options-dev libconfig++-dev libsctp-dev \
  libzmq3-dev

git clone https://github.com/srsran/srsRAN_4G.git
cd srsRAN_4G
rm -rf build
mkdir build
cd build

env -i HOME="$HOME" USER="$USER" PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  cmake .. \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_SKIP_RPATH=ON

make -j"$(nproc)"

sudo make install
sudo ldconfig

```

好，`srsUE` 已經 build 出來了。下一步做 **srsUE 設定檔**。

先建立 config 目錄：

```
mkdir -p /home/zju/Desktop/oran-lab/config/srsue
```

複製 srsUE 預設設定檔：

```
cp /home/zju/Desktop/oran-lab/src/srsRAN_4G/srsue/ue.conf.example \
   /home/zju/Desktop/oran-lab/config/srsue/ue-zmq-open5gs.conf
```

然後打開 UE config：

```
nano /home/zju/Desktop/oran-lab/config/srsue/ue-zmq-open5gs.conf
```

你要改幾個區塊。

**1. `[rf]` 區塊改成 ZMQ**

找 `[rf]`，改成或補成：

```
[rf]
freq_offset = 0
tx_gain = 0
rx_gain = 0
srate = 23.04e6
nof_antennas = 1
device_name = zmq
device_args = tx_port=tcp://127.0.0.1:2001,rx_port=tcp://127.0.0.1:2000,base_srate=23.04e6
```

注意 port 跟 gNB 是反過來的。

**2. `[gw]` 區塊設定 namespace**

找 `[gw]`，改成：

```
[gw]
netns = ue1
ip_devname = tun_srsue
ip_netmask = 255.255.255.0
```

**3. `[usim]` 區塊**

找 `[usim]`，先用一組測試值：

```
[usim]
mode = soft
algo = milenage
opc  = 63BFA50EE6523365FF14C1F45F88737D
k    = 00112233445566778899AABBCCDDEEFF
imsi = 999700000000001
imei = 353490069873319
```

這些等一下要在 Open5GS WebUI 新增同樣 subscriber。

**4. `[rrc]` / PLMN**

找看看有沒有 `mcc` / `mnc`，如果有，設成：

```
mcc = 999
mnc = 70
```


**改 `[rat.eutra]`**

```
[rat.eutra]
dl_earfcn = 2850
nof_carriers = 0
```

**改 `[rat.nr]`**

```
[rat.nr]
bands = 3
nof_carriers = 1
max_nof_prb = 106
nof_prb = 106
```

**改 `[rrc]`**

```
[rrc]
ue_category = 4
release = 15
```

**改 `[nas]`**

```
[nas]
apn = internet
apn_protocol = ipv4
```

改完先檢查：

```
grep -nE "^\[rf\]|device_name|device_args|srate|tx_gain|rx_gain|^\[gw\]|netns|ip_devname|^\[usim\]|algo|opc|k|imsi|imei|mcc|mnc" \
  /home/zju/Desktop/oran-lab/config/srsue/ue-zmq-open5gs.conf
```

接著建立 UE namespace：

```
sudo ip netns add ue1
sudo ip netns list
```

### 8.6 在 Open5GS 裡面註冊這台裝置

**Open5GS WebUI 要新增這個 UE subscriber**，而且 IMSI/K/OPc 要跟你 `srsUE` config 一樣。

你現在 UE 參數是：

```
IMSI = 999700000000001
K    = 00112233445566778899AABBCCDDEEFF
OPc  = 63BFA50EE6523365FF14C1F45F88737D
PLMN = 99970
```


去 Open5GS WebUI：

```
http://localhost:9999
username: admin
password: 1423
```

新增 subscriber：

```
IMSI: 999700000000001
Subscriber Key / K: 00112233445566778899AABBCCDDEEFF
OPc: 63BFA50EE6523365FF14C1F45F88737D
AMF: 8000
Subscriber Status = SERVICE_GRANTED
Operator Determined Barring = 留空，不要選任何 barred
APN / DNN: internet
SST: 1
```


新增完後，跑網路 NAT，讓 UE 後面可以上網：

```
sudo sysctl -w net.ipv4.ip_forward=1

sudo iptables -t nat -C POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE 2>/dev/null || \
sudo iptables -t nat -A POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE

sudo iptables -C FORWARD -s 10.45.0.0/16 -i ogstun -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 1 -s 10.45.0.0/16 -i ogstun -j ACCEPT

sudo iptables -C FORWARD -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 2 -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
```

然後啟動順序：

**終端 1：Open5GS log**

```
sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd -f -l
```

**終端 2：gNB**

```
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb

./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
```

**終端 3：srsUE**

```
cd /home/zju/Desktop/oran-lab/src/srsRAN_4G/build/srsue/src

sudo ./srsue /home/zju/Desktop/oran-lab/config/srsue/ue-zmq-open5gs.conf
```

如果成功，你會看到類似：

```
RRC Connected
PDU Session Establishment
Network attach successful
```

然後檢查 UE namespace 裡有沒有 tun：

```
sudo ip netns exec ue1 ip addr
sudo ip netns exec ue1 ip route
```

如果 `tun_srsue` 有 UE IP 但沒有 default route，補：

```
sudo ip netns exec ue1 ip route replace default dev tun_srsue
```

如果有 `tun_srsue`，再測：

```
sudo ip netns exec ue1 ping -c 4 10.45.0.1
sudo ip netns exec ue1 ping -c 4 8.8.8.8
```

定位 user-plane 時，三個抓包一起看：

```bash
sudo tcpdump -ni ogstun icmp
sudo tcpdump -ni any udp port 2152
sudo ip netns exec ue1 tcpdump -ni tun_srsue icmp
```

本次實測成功狀態：

```text
srsUE:
Random Access Complete
RRC Connected
PDU Session Establishment successful. IP: 10.45.0.4

gateway:
sudo ip netns exec ue1 ping -c 4 10.45.0.1
4 packets transmitted, 4 received, 0% packet loss

internet:
sudo ip netns exec ue1 ping -c 4 8.8.8.8
4 packets transmitted, 4 received, 0% packet loss
```

成功時 `tcpdump -ni any udp port 2152` 會看到：

```text
127.0.0.1.2152 > 127.0.0.7.2152
127.0.0.7.2152 > 127.0.0.1.2152
```

成功時 `tcpdump -ni ogstun icmp` 會看到：

```text
10.45.0.x > 10.45.0.1: ICMP echo request
10.45.0.1 > 10.45.0.x: ICMP echo reply
```

## 9. 建置 FlexRIC

FlexRIC 是第一階段的 Near-RT RIC 與 xApp SDK。

### 9.1 安裝 FlexRIC 額外相依套件

目前這台 Ubuntu 22.04 的預設 `gcc` 是 11.4，但 FlexRIC 不支援 gcc-11。系統已經有 `gcc-12`，所以 FlexRIC 直接指定 `gcc-12/g++-12`。不要優先裝 `gcc-13`；Ubuntu 22.04 預設 repository 通常沒有 `gcc-13`。

```bash
sudo apt update
sudo apt install -y \
  git build-essential autoconf automake libtool bison \
  gcc-12 g++-12 cpp-12 \
  libsctp-dev cmake-curses-gui libpcre2-dev \
  python3.10-dev python3-dev \
  libconfig-dev libconfig++-dev
```

因為 9.2 要開 `-DXAPP_MULTILANGUAGE=ON`，FlexRIC 需要 SWIG >= 4.1。Ubuntu 22.04 apt 內建的 `swig` 是 4.0.2，會造成：

```text
Could NOT find SWIG: Found unsuitable version "4.0.2", but required is at least "4.1"
```

所以 SWIG 要從 source 安裝到 `/usr/local`：

```bash
cd /home/zju/Desktop/oran-lab/src
if [ ! -d swig ]; then
  git clone https://github.com/swig/swig.git
fi

cd swig
git checkout release-4.1
./autogen.sh
./configure --prefix=/usr/local
make -j"$(nproc)"
sudo make install

/usr/local/bin/swig -version
```

成功標準：

```text
SWIG Version 4.1.x
```

### 9.2 編譯 FlexRIC

先用 main branch。若之後 OCUDU 的 E2AP / KPM 版本有指定，再把 FlexRIC CMake 參數對齊。

注意：多行指令每行最後的 `\` 後面不能有空格。

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
mkdir -p build
cd build

rm -f CMakeCache.txt
rm -rf CMakeFiles

cmake -DCMAKE_BUILD_TYPE=Release \
  -DXAPP_MULTILANGUAGE=ON \
  -DKPM_VERSION=KPM_V3_00 \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
  -DSWIG_EXECUTABLE=/usr/local/bin/swig \
  ../

make -j"$(nproc)"
sudo make install
sudo ldconfig
```

目前已遇到並修正的錯誤：

```text
FlexRIC doesn't support gcc-11.
```

處理方式是指定：

```text
-DCMAKE_C_COMPILER=/usr/bin/gcc-12
-DCMAKE_CXX_COMPILER=/usr/bin/g++-12
```

目前 9.2 若繼續失敗，優先看是不是 SWIG 版本還抓到 `/usr/bin/swig4.0`。正確 CMake output 應該要使用 `/usr/local/bin/swig`。

驗證 Near-RT RIC binary：先用 build 目錄內的 binary 測，不急著依賴系統安裝版。

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/ric/nearRT-RIC --help
```

## 10. 先測 FlexRIC 本身

這一步還沒有接 Open5GS，也沒有接 OCUDU；只是確認 Near-RT RIC 與 E2 emulator 能正常跑。

Terminal 1：啟動 Near-RT RIC。

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/ric/nearRT-RIC
```

Terminal 2：啟動 E2 agent emulator。

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/emulator/agen應該是有一個文件我看一下AppDD在一個筆記暫存去下面一個play嗎那個play會寫要怎麼去安裝操作那裡面有提到一個MongoDB的安裝然後把那個MomoTV安裝直接改成那個Data版的不要用t/emu_agent_gnb
```

成功標準：

```text
Near-RT RIC console 看到 E2 SETUP REQUEST / RESPONSE 類似訊息。
```

Terminal 3：啟動 example monitoring xApp。

先找 FlexRIC repo 裡有哪些 xApp：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
find ./build/examples/xApp -type f -executable | sort
```

常見 monitoring xApp 可能在：

```bash
./build/examples/xApp/c/monitor/
```

執行方式依實際檔名調整，例如：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/xApp/c/monitor/xapp_kpm_moni
```

或：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/xApp/c/monitor/xapp_oran_moni -c <KPM_XAPP_CONFIG>
```

成功標準：

```text
xApp 可以連上 Near-RT RIC。
xApp 可以收到 emulator 送出的 indication。
```

## 11. 第一階段啟動順序

建議用 4 個 terminal。

### Terminal 1：Open5GS

Open5GS package 安裝後會由 systemd 管理。

```bash
systemctl status open5gs-amfd --no-pager
systemctl status open5gs-smfd --no-pager
systemctl status open5gs-upfd --no-pager
```

看 AMF log：

```bash
sudo tail -f /var/log/open5gs/amf.log
```

### Terminal 2：FlexRIC Near-RT RIC

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/ric/nearRT-RIC
```

### Terminal 3：OCUDU gNB

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
sudo ./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-zmq-open5gs-flexric.yml
```

觀察重點：

```text
gNB 是否成功啟動。
gNB 是否連到 Open5GS AMF。
gNB 是否連到 FlexRIC Near-RT RIC。
```

### Terminal 4：example xApp

先列出 xApp：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
find ./build/examples/xApp -type f -executable | sort
```

再啟動 KPM monitoring xApp。

成功標準：

```text
xApp console 看到 KPM indication / measurement。
至少能看到 RIC indication 週期性出現。
```

## 12. UE / traffic 產生方式

這一段先不綁死，因為 OCUDU 新版的 UE / RU 測試方式要以當前官方 tutorial、實驗室硬體與老師需求為準。

可選方案：

| 方案                     | 說明                                           | 適合時機                           |
| ---------------------- | -------------------------------------------- | ------------------------------ |
| SDR + COTS UE          | 用 USRP / B210 / N310 等 SDR 加真手機或測試 UE        | 最接近真實 demo，但設定較重               |
| ZMQ virtual radio      | 全軟體測試，用來開發與 debug                            | 適合前期，但要確認 OCUDU 當前 ZMQ UE 相容設定 |
| Core-only UE simulator | 只測 Open5GS attach / PDU session，不經 OCUDU gNB | 可測 Core，但不能驗證 RIC/xApp         |

第一階段建議順序：

1. 先讓 Open5GS、OCUDU、FlexRIC 都單獨跑起來。
2. 再確認 OCUDU 當前官方 tutorial 支援的 UE / RU 路線。
3. 最後再加 UE attach 與 traffic。

## 13. 第一階段完成檢查表

| 檢查項目 | 指令 / 觀察 | 狀態 |
| --- | --- | --- |
| Docker 可用 | `docker run --rm hello-world` |  |
| Docker Compose 可用 | `docker compose version` |  |
| SCTP module 載入 | `lsmod \| grep sctp` |  |
| Open5GS AMF running | `systemctl status open5gs-amfd` |  |
| Open5GS SMF running | `systemctl status open5gs-smfd` |  |
| Open5GS UPF running | `systemctl status open5gs-upfd` |  |
| OCUDU gNB binary 可執行 | `./gnb --help` |  |
| OCUDU config 已建立 | `ls /home/zju/Desktop/oran-lab/config/ocudu/` |  |
| FlexRIC 可啟動 | `./build/examples/ric/nearRT-RIC` |  |
| FlexRIC emulator 可連線 | `emu_agent_gnb` 後 RIC 出現 E2 setup |  |
| FlexRIC xApp 可連線 | example xApp 連上 RIC |  |
| OCUDU 連到 AMF | gNB log / Open5GS AMF log |  |
| OCUDU 連到 RIC | gNB log / FlexRIC log |  |
| xApp 收到 KPM | xApp log 顯示 indication |  |

## 14. 常見問題

### 14.1 OCUDU 找不到 ZMQ

通常是 `libzmq3-dev` 裝完後沒有重新 cmake。

處理方式：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu
rm -rf build
mkdir build
cd build
CC=clang-14 CXX=clang++-14 cmake ../ -DENABLE_EXPORT=ON -DENABLE_ZEROMQ=ON -DBUILD_TESTING=OFF
make -j"$(nproc)"
```

### 14.2 FlexRIC 編譯失敗

先確認 compiler 和 SWIG：

```bash
gcc --version
g++ --version
/usr/bin/gcc-12 --version
/usr/bin/g++-12 --version
/usr/local/bin/swig -version
```

已知問題與處理：

1. `FlexRIC doesn't support gcc-11`

不要用系統預設 `/usr/bin/gcc`。重新 configure 時指定 `gcc-12/g++-12`。

2. `Could NOT find SWIG: Found unsuitable version "4.0.2", but required is at least "4.1"`

不要用 apt 的 `swig`。照 9.1 從 source 安裝 SWIG 4.1，並在 CMake 指定 `-DSWIG_EXECUTABLE=/usr/local/bin/swig`。

重新 configure：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
mkdir -p build
cd build

rm -f CMakeCache.txt
rm -rf CMakeFiles

cmake -DCMAKE_BUILD_TYPE=Release \
  -DXAPP_MULTILANGUAGE=ON \
  -DKPM_VERSION=KPM_V3_00 \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
  -DSWIG_EXECUTABLE=/usr/local/bin/swig \
  ../

make -j"$(nproc)"
sudo make install
sudo ldconfig
```

### 14.3 SCTP 連線問題

確認 module：

```bash
sudo modprobe sctp
lsmod | grep sctp
```

確認 port：

```bash
sudo ss -lnp | grep 38412
sudo ss -lnp | grep 36421
sudo ss -lnp | grep 36422
```

常用 port：

| Port | 用途 |
| --- | --- |
| 38412/SCTP | NGAP，gNB 連 AMF |
| 36421/SCTP | FlexRIC E2AP 常見設定 |
| 36422/SCTP | FlexRIC E42 / xApp 常見設定 |

### 14.4 OCUDU 連不上 Open5GS

優先檢查：

1. Open5GS AMF 是否 running。
2. AMF bind address 是否是 gNB 能連到的 IP。
3. OCUDU config 裡的 AMF IP / port 是否正確。
4. PLMN / TAC 是否一致。
5. 防火牆是否擋住 SCTP。

查看 AMF log：

```bash
sudo tail -f /var/log/open5gs/amf.log
```

### 14.5 UE attach 成功但 user-plane 不通

先確認 UE session 是否還活著：

```bash
sudo tail -n 120 /var/log/open5gs/amf.log
sudo tail -n 120 /var/log/open5gs/smf.log
sudo tail -n 120 /var/log/open5gs/upf.log
tail -n 160 /tmp/gnb.log
```

如果看到 `UEContextReleaseRequest`、`BearerContextRelease`、`Removed Session`、`Implicit De-registered`，表示目前不是活著的 PDU session，應先重啟 gNB / srsUE 讓 UE 重新 attach。

活著的 session 下，用三個 tcpdump 定位：

```bash
sudo tcpdump -ni ogstun icmp
sudo tcpdump -ni any udp port 2152
sudo ip netns exec ue1 tcpdump -ni tun_srsue icmp
```

判斷方式：

| 現象 | 可能原因 |
| --- | --- |
| `tun_srsue` 有 ICMP，但 `udp/2152` 沒有 | UE 到 gNB 或 gNB DRB/N3 轉送有問題 |
| `udp/2152` 有上行到 UPF，但 `ogstun` 沒有 ICMP | UPF session / TEID / PDR/FAR 或 UPF 解包有問題 |
| `ogstun` 能 ping `10.45.0.1`，但 `8.8.8.8` 不通 | host forwarding / NAT / firewall 問題 |

本機實測曾遇到 Docker 將 IPv4 `FORWARD` policy 設成 `DROP`，導致 UE 封包到 `ogstun` 後被擋住，`POSTROUTING` NAT counter 不增加。修正：

```bash
sudo iptables -C FORWARD -s 10.45.0.0/16 -i ogstun -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 1 -s 10.45.0.0/16 -i ogstun -j ACCEPT

sudo iptables -C FORWARD -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 2 -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
```

確認 counter：

```bash
sudo iptables -vnL FORWARD --line-numbers
sudo iptables -t nat -vnL POSTROUTING --line-numbers
```

### 14.6 OCUDU 連不上 FlexRIC

優先檢查：

1. FlexRIC Near-RT RIC 是否已啟動。
2. OCUDU config 裡 E2 是否 enable。
3. RIC IP 是否正確。
4. RIC E2 port 是否正確。
5. SCTP 是否啟用。

## 15. 第二階段才做的事

環境跑通後，第二階段才開始做自己的 xApp Controller。

第二階段內容：

1. 複製或參考 FlexRIC example KPM xApp。
2. 讀取 KPM indication。
3. 定義自己的 metric parser。
4. 寫 baseline rule-based algorithm。
5. 寫自己的 PRB / QoS / slicing control policy。
6. 收集 CSV log。
7. 用 Python plot 或 Grafana 畫圖。
8. 比較 baseline vs proposed algorithm。

第二階段建議題目：

```text
xApp-based Dynamic PRB Allocation for QoS-aware O-RAN
```

或：

```text
A Reproducible OCUDU-based O-RAN Testbed for xApp Control
```

## 16. 參考資料

- Open5GS GitHub: https://github.com/open5gs/open5gs
- Open5GS Quickstart: https://open5gs.org/open5gs/docs/guide/01-quickstart/
- OCUDU official site: https://ocudu.org/
- OCUDU GitLab: https://gitlab.com/ocudu/ocudu
- OCUDU documentation: https://docs.ocudu.org/
- OCUDU installation docs: https://docs.ocudu.org/user_manual/installation/
- OCUDU releases and roadmap: https://docs.ocudu.org/releases/
- FlexRIC GitLab: https://gitlab.eurecom.fr/mosaic5g/flexric
- Docker Engine Ubuntu install: https://docs.docker.com/engine/install/ubuntu/
- Docker Compose plugin install: https://docs.docker.com/compose/install/linux/
