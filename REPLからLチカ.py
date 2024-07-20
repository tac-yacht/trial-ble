import machine, neopixel
statusLED = neopixel.NeoPixel(machine.Pin(2),1)
statusLED[0] = (0,1,0)
statusLED.write()
#緑に光ったらOK
