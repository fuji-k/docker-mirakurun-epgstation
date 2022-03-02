docker-mirakurun-epgstationv2-rpi4
====

[Mirakurun](https://github.com/Chinachu/Mirakurun) + [EPGStation](https://github.com/l3tnun/EPGStation) の Raspberry Pi4 用 Docker コンテナ
[ykym氏](https://github.com/ykym/docker-mirakurun-epgstation-rpi)のdocker-mirakurun-epgstation-rpiをrpi4でRPiOS32bit(Raspbian GNU/Linux 11 (bullseye))を利用しPLEX PX-W3U4に固定し最新版のmirakurun、epgstationv2を登録できるようにしてみました。
今回test版ですがAmatsukaze連携のモジュールを組み込んであります。epgstation/config/qsvenc.shを環境に即して適宜修正して頂ければエンコードを他の端末のAmatsukazeに投げる事が可能となっています。

## 構築例

Raspberry Pi 構築

### bullseye lite イメージを SDカードに dd
https://www.raspberrypi.org/downloads/raspbian/

（デスクトップ環境が欲しかったら with desktop）

boot パーティションのディレクトリに 「ssh」(拡張子なし) のファイルを作っておくと最初から SSH が有効化される

### 起動

### login
```
user: pi
pass: raspberry
```

### IPを固定する場合
```
sudo vi /etc/dhcpcd.conf

以下の部分のコメントアウトを外して適時設定

#interface eth0
#static ip_address=192.168.0.10/24
#static ip6_address=fd51:42f8:caae:d92e::ff/64
#static routers=192.168.0.1
#static domain_name_servers=192.168.0.1 8.8.8.8 fd51:42f8:caae:d92e::1

面倒ならDHCPサーバ側で固定振り
```

### 初期設定
```
sudo timedatectl set-timezone 'Asia/Tokyo'

sudo apt update && sudo apt upgrade -y

sudo apt install locales-all
sudo localectl set-locale 'LANG=ja_JP.utf8'
```
### ドライバインストール (例:[px4_drv](https://github.com/nns779/px4_drv))
```
sudo apt -y install raspberrypi-kernel-headers dkms git

git clone https://github.com/nns779/px4_drv.git
cd px4_drv
cd fwtool
make
wget http://plex-net.co.jp/plex/pxw3u4/pxw3u4_BDA_ver1x64.zip -O pxw3u4_BDA_ver1x64.zip
unzip -oj pxw3u4_BDA_ver1x64.zip pxw3u4_BDA_ver1x64/PXW3U4.sys
./fwtool PXW3U4.sys it930x-firmware.bin
sudo mkdir -p /lib/firmware
sudo cp it930x-firmware.bin /lib/firmware/
cd ../
sudo cp -a ./ /usr/src/px4_drv-0.2.1
sudo dkms add px4_drv/0.2.1
sudo dkms install px4_drv/0.2.1
cd
```
### チューニング
```
echo 'SUBSYSTEM=="vchiq",GROUP="video",MODE="0666"' | sudo tee /etc/udev/rules.d/10-vchiq-permissions.rules
echo 'SUBSYSTEM=="video10",GROUP="video",MODE="0666"' | sudo tee /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video11",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video12",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video13",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video14",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video15",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video16",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video18",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video20",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video21",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video22",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo 'SUBSYSTEM=="video23",GROUP="video",MODE="0666"' | sudo tee -a /etc/udev/rules.d/10-v4l2-permissions.rules
echo "options px4_drv xfer_packets=51 urb_max_packets=816 max_urbs=6" | sudo tee /etc/modprobe.d/px4_drv.conf
```
### install docker
```
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker pi
sudo apt install -y libffi-dev libssl-dev python3 python3-pip python3-dev
sudo pip3 install docker-compose
```

### 一旦再起動
```
sudo reboot
```

### install Mirakurun+EPGStation
```
cd
git clone http://github.com/thik98/docker-mirakurun-epgstationv2-rpi4.git dmer
cd dmer
sudo docker-compose build
```

### 環境に応じて設定ファイル書き換え
```
vi epgstation/config/config.yml
vi mirakurun/conf/tuners.yml
vi mirakurun/conf/channels.yml
vi docker-compose.yml
```

### 録画領域がローカル以外ならマウントさせる
```
sudo vi /etc/fstab

sudo mount -a
```

### 起動
```
sudo docker-compose up -d
```
