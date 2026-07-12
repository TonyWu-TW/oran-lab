# O-RAN Lab 常用指令

這份文件整理本機 `/home/zju/Desktop/oran-lab` 的 Git、Open5GS 5GC、FlexRIC Near-RT RIC、OCUDU gNB、srsUE、xApp、log 與網路測試指令。

> 一般啟動順序：**MongoDB / Open5GS 5GC → Near-RT RIC → gNB → xApp → UE**。  
> 專案已提供一鍵腳本，平常優先使用下一節的指令即可。

## 1. 每天最常用：更新並啟動全部服務

```bash
cd /home/zju/Desktop/oran-lab

# 先確認有沒有尚未提交的修改
git status

# 從 GitHub 更新 main；只允許 fast-forward，避免自動產生 merge commit
git pull --ff-only origin main

# 啟動 Open5GS log、Near-RT RIC、gNB、KPM xApp、srsUE
./start_stage1.sh

# 檢查所有服務與 UE 網路
./status_stage1.sh
```

停止由 `start_stage1.sh` 啟動的服務：

```bash
cd /home/zju/Desktop/oran-lab
./stop_stage1.sh
```

## 2. 用 tmux 一次啟動全部服務

這個方式會把各服務放在同一個 tmux 畫面，適合直接觀察輸出：

```bash
cd /home/zju/Desktop/oran-lab
./run_stage1_tmux.sh
```

tmux 常用快捷鍵：

- 暫時離開但不停止服務：按 `Ctrl+b`，放開後按 `d`
- 回到畫面：`tmux attach -t oran-stage1`
- 切換窗格：按 `Ctrl+b`，放開後按方向鍵
- 列出 session：`tmux ls`
- 停止整個 tmux session：`tmux kill-session -t oran-stage1`

不啟動 UE：

```bash
cd /home/zju/Desktop/oran-lab
RUN_UE=0 ./run_stage1_tmux.sh
```

## 3. Git 常用指令

### 查看目前狀態與更新程式碼

```bash
cd /home/zju/Desktop/oran-lab

git status
git branch --show-current
git log --oneline -10
git fetch origin
git pull --ff-only origin main
```

如果 `git pull` 顯示本機有未提交修改，先查看內容：

```bash
git status
git diff
```

想保留修改但暫時收起來，再 pull：

```bash
git stash push -u -m "before pull"
git pull --ff-only origin main
git stash pop
```

`git stash pop` 若顯示 conflict，不要再重複執行；先用 `git status` 找出衝突檔並處理。

### 提交並推送自己的修改

```bash
cd /home/zju/Desktop/oran-lab

git status
git diff
git add <檔案路徑>
git commit -m "描述這次修改"
git push origin main
```

例如提交這份文件：

```bash
git add command.md
git commit -m "docs: add service command reference"
git push origin main
```

取消「尚未 git add」的單一檔案修改（會丟掉修改，使用前先確認）：

```bash
git restore <檔案路徑>
```

取消已經 `git add`、但保留檔案內容：

```bash
git restore --staged <檔案路徑>
```

## 4. 各服務分開啟動

若不使用一鍵腳本，請開不同 terminal，依照以下順序啟動。

### Terminal 1：MongoDB 與 Open5GS 5GC

MongoDB 容器：

```bash
docker start open5gs-mongodb
docker ps --filter name=open5gs-mongodb
docker exec open5gs-mongodb mongosh --eval 'db.runCommand({ ping: 1 })'
```

啟動主要 Open5GS 服務：

```bash
sudo systemctl start open5gs-nrfd open5gs-amfd open5gs-smfd open5gs-upfd
sudo systemctl status open5gs-nrfd open5gs-amfd open5gs-smfd open5gs-upfd --no-pager
```

若修改過 `/etc/open5gs/*.yaml`，重新啟動：

```bash
sudo systemctl restart open5gs-nrfd open5gs-amfd open5gs-smfd open5gs-upfd
```

查看 5GC 即時 log：

```bash
sudo journalctl -u open5gs-amfd -u open5gs-smfd -u open5gs-upfd -f -l
```

WebUI（若本機已安裝此 service）：

```bash
sudo systemctl start open5gs-webui
sudo systemctl status open5gs-webui --no-pager
```

瀏覽器網址：`http://localhost:9999`，目前專案筆記中的帳號為 `admin`、密碼為 `1423`。

### Terminal 2：FlexRIC Near-RT RIC

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/ric/nearRT-RIC
```

### Terminal 3：OCUDU gNB

使用目前已驗證可連 Open5GS 與 srsUE 的設定：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
sudo ./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs.yml
```

若 FlexRIC E2 設定檔存在，改用：

```bash
cd /home/zju/Desktop/oran-lab/src/ocudu/build/apps/gnb
sudo ./gnb -c /home/zju/Desktop/oran-lab/config/ocudu/gnb-fdd-srsue-zmq-open5gs-flexric.yml
```

### Terminal 4：xApp

啟動目前一鍵腳本使用的 KPM monitoring xApp：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
./build/examples/xApp/c/monitor/xapp_kpm_moni
```

列出所有已編譯、可以執行的 xApp：

```bash
cd /home/zju/Desktop/oran-lab/src/flexric
find ./build/examples/xApp -type f -executable | sort
```

其他已編譯的常用 xApp 範例：

```bash
# Hello world
./build/examples/xApp/c/helloworld/xapp_hw

# RC monitoring
./build/examples/xApp/c/monitor/xapp_rc_moni

# GTP / MAC / RLC / PDCP monitoring
./build/examples/xApp/c/monitor/xapp_gtp_mac_rlc_pdcp_moni

# KPM + RC
./build/examples/xApp/c/kpm_rc/xapp_kpm_rc

# Slice monitoring / control
./build/examples/xApp/c/slice/xapp_slice_moni_ctrl
```

一次只測一個控制型 xApp，並先確認它支援目前 gNB 提供的 E2 Service Model。

### Terminal 5：srsUE（5G UE）

```bash
cd /home/zju/Desktop/oran-lab/src/srsRAN_4G/build/srsue/src
sudo ./srsue /home/zju/Desktop/oran-lab/config/srsue/ue-zmq-open5gs.conf
```

成功時應看到 `RRC Connected` 與 `PDU Session Establishment successful`。

## 5. 一鍵腳本的自訂方式

只啟動到 gNB/xApp，不啟動 UE：

```bash
cd /home/zju/Desktop/oran-lab
RUN_UE=0 ./start_stage1.sh
```

指定另一個 xApp：

```bash
cd /home/zju/Desktop/oran-lab
XAPP_BIN=/home/zju/Desktop/oran-lab/src/flexric/build/examples/xApp/c/monitor/xapp_rc_moni ./start_stage1.sh
```

指定另一份 gNB 或 UE 設定檔：

```bash
cd /home/zju/Desktop/oran-lab
GNB_CONFIG=/完整路徑/gnb.yml SRSUE_CONFIG=/完整路徑/ue.conf ./start_stage1.sh
```

## 6. 狀態、log 與程序檢查

完整狀態檢查：

```bash
cd /home/zju/Desktop/oran-lab
./status_stage1.sh
```

觀看一鍵腳本產生的 log：

```bash
tail -f /home/zju/Desktop/oran-lab/logs/stage1/open5gs-log.log
tail -f /home/zju/Desktop/oran-lab/logs/stage1/nearRT-RIC.log
tail -f /home/zju/Desktop/oran-lab/logs/stage1/gnb.log
tail -f /home/zju/Desktop/oran-lab/logs/stage1/xapp.log
tail -f /home/zju/Desktop/oran-lab/logs/stage1/srsue.log
```

找出相關程序：

```bash
pgrep -af 'nearRT-RIC|/gnb|srsue|xapp_kpm'
```

Open5GS 個別 log：

```bash
sudo journalctl -u open5gs-amfd -n 100 --no-pager
sudo journalctl -u open5gs-smfd -n 100 --no-pager
sudo journalctl -u open5gs-upfd -n 100 --no-pager
```

## 7. UE 網路測試

查看 UE IP 與 route：

```bash
sudo ip netns exec ue1 ip addr
sudo ip netns exec ue1 ip route
```

若 `tun_srsue` 有 IP、但沒有 default route：

```bash
sudo ip netns exec ue1 ip route replace default dev tun_srsue
```

測試 UPF 與 Internet：

```bash
sudo ip netns exec ue1 ping -c 4 10.45.0.1
sudo ip netns exec ue1 ping -c 4 8.8.8.8
```

測試流量：

```bash
sudo ip netns exec ue1 iperf3 -c <IPERF_SERVER_IP>
```

抓包除錯（分別在不同 terminal 執行）：

```bash
sudo tcpdump -ni ogstun icmp
sudo tcpdump -ni any udp port 2152
sudo ip netns exec ue1 tcpdump -ni tun_srsue icmp
```

## 8. UE 無法上網時的 NAT 設定

以下規則在重新開機後可能消失；UE 無法連外時再執行：

```bash
sudo sysctl -w net.ipv4.ip_forward=1

sudo iptables -t nat -C POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE 2>/dev/null || \
sudo iptables -t nat -A POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE

sudo iptables -C FORWARD -s 10.45.0.0/16 -i ogstun -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 1 -s 10.45.0.0/16 -i ogstun -j ACCEPT

sudo iptables -C FORWARD -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
sudo iptables -I FORWARD 2 -d 10.45.0.0/16 -o ogstun -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
```

## 9. 分開停止服務

在前景執行的 gNB、RIC、xApp 或 UE，可在它的 terminal 按 `Ctrl+C`。

停止 Open5GS 與 WebUI：

```bash
sudo systemctl stop open5gs-webui
sudo systemctl stop open5gs-amfd open5gs-smfd open5gs-upfd open5gs-nrfd
```

停止 MongoDB 容器：

```bash
docker stop open5gs-mongodb
```

若是一鍵背景腳本啟動，仍應使用：

```bash
cd /home/zju/Desktop/oran-lab
./stop_stage1.sh
```

## 10. 最短速查表

```bash
# 更新
cd /home/zju/Desktop/oran-lab && git pull --ff-only origin main

# 全部啟動（背景）
cd /home/zju/Desktop/oran-lab && ./start_stage1.sh

# 全部啟動（tmux 畫面）
cd /home/zju/Desktop/oran-lab && ./run_stage1_tmux.sh

# 檢查
cd /home/zju/Desktop/oran-lab && ./status_stage1.sh

# 全部停止
cd /home/zju/Desktop/oran-lab && ./stop_stage1.sh
```
