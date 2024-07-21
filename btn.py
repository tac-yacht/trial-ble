import machine, neopixel
statusLED = neopixel.NeoPixel(machine.Pin(2), 1)

button = machine.Pin(9)
import time,asyncio,main

def standby():
    while True:
        # print('button on wait...')
        if button.value() == 0:
            print('call main')
            statusLED[0] = (0,0,1)
            statusLED.write()

            asyncio.run(main.main())

            statusLED[0] = (0,1,0)
            statusLED.write()

        time.sleep_ms(500)
