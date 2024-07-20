# メインインポート
import ntptime
import time

# 補助
import network
from wifi import connect

#設定
ntptime.server = 'ntp.nict.jp'

def settime():
    if network.WLAN(network.STA_IF).isconnected():
        connect()

    ntptime.settime()
    print(time.localtime())
