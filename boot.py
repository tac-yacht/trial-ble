# boot.py -- run on boot-up

import machine, neopixel
statusLED = neopixel.NeoPixel(machine.Pin(2), 1)
statusLED[0] = (1,0,0)
statusLED.write()

import aioble
aioble.core.log_level=3

from ntpsettime import settime
settime()

statusLED[0] = (0,1,0)
statusLED.write()


