# main.py -- put your code here!

# https://github.com/micropython/micropython-lib/tree/master/micropython/bluetooth/aioble
# https://github.com/micropython/micropython-lib/blob/master/micropython/bluetooth/aioble/examples/temp_client.py

import asyncio
import bluetooth
import aioble

from micropython import const
_IO_CAPABILITY_NO_INPUT_OUTPUT = const(3)

_DIS_SERVICE_UUID = bluetooth.UUID(0x180A)
_BLP_SERVICE_UUID = bluetooth.UUID(0x1810)

async def find_blood_pressure_sensor():
    print('scan start')
    async with aioble.scan(duration_ms=10000, active=True) as scanner:
        #どうやらアクティブで無いと、サービスが拾って来れない

        async for result in scanner:
            print(result, result.name(), result.rssi)
            if _BLP_SERVICE_UUID in result.services():
                print('found device(scan end)', result.device)
                return result.device

    print('scan end(not found...)')
    return None

async def printDeviceInfo(conn: DeviceConnection):
    dis = await conn.service(uuid = _DIS_SERVICE_UUID)
    print(dis)
    async for characteristic in dis.characteristics(timeout_ms=10_000):
        characteristics.append(characteristic)
    
    result = {}
    for characteristic in characteristics:
        result[characteristic.uuid] = await characteristic.read()

    for key,value in result.items():
        print(key,value)
    return result

async def printDatetime(conn: DeviceConnection):
    #ほかでペアリング済みだと読み取りさえできない
    from struct import unpack
    blp = await conn.service(uuid = _BLP_SERVICE_UUID)
    print(blp)
    dateTimeChar = await blp.characteristic(uuid = bluetooth.UUID(0x2A08))
    print(dateTimeChar)

    try:
        raw = await dateTimeChar.read(timeout_ms=5_000)
        dateTimeValue = unpack("<HBBBBB",raw)
        print(dateTimeValue)
        asyncio.sleep(2)
    except aioble.GattError as e:
        print(e)

async def printBloodPressure(conn: DeviceConnection):
    blp = await conn.service(uuid = _BLP_SERVICE_UUID)
    print(blp)

    characteristics = []
    async for characteristic in blp.characteristics(timeout_ms=10_000):
        characteristics.append(characteristic)
    
    result = {}
    for characteristic in characteristics:
        try:
            result[characteristic.uuid] = await characteristic.read()
        except ValueError as e:
            result[characteristic.uuid] = e

    for key,value in result.items():
        print(key,value)
    return result

async def printBloodPressureRecord(conn: DeviceConnection):
    blp = await conn.service(uuid = _BLP_SERVICE_UUID)
    print(blp)
    
    recordChar = await blp.characteristic(uuid = bluetooth.UUID(0x2A35))
    await recordChar.subscribe(notify=False, indicate=True)
    #デフォルトだとnotifyがtrueだから明示的に殺す必要がある

    dataList = []
    try:
        while True:
            data = await recordChar.indicated(timeout_ms=1_000)
            print(data)
            dataList.append(data)
    except asyncio.TimeoutError:
        print('read timeout')
    
    return dataList

async def setDatetime(conn: DeviceConnection):
    from struct import pack
    from time import gmtime

    blp = await conn.service(uuid = _BLP_SERVICE_UUID)
    print(blp)
    dateTimeChar = await blp.characteristic(uuid = bluetooth.UUID(0x2A08))
    print(dateTimeChar)

    now_utc = gmtime() #localtimeはタイムゾーン設定できないので強制的にUTCになってしまう
    print("set datetime from", now_utc)
    raw = pack("<HBBBBB", *now_utc) #nowはタプル　順番がちょうど仕様通り　余ったものは捨てられるため、ぴったりにしなくていい
    await dateTimeChar.write(data=raw, timeout_ms=5_000)
    print("datetime setting is success!!")

async def pairing(conn: DeviceConnection):
    print('start pairing with bonding')
    # https://github.com/micropython/micropython-lib/blob/master/micropython/bluetooth/aioble/aioble/security.py#L163
    args = {
        'bond': True,
        'le_secure': False,
        'mitm': False,
        'io': _IO_CAPABILITY_NO_INPUT_OUTPUT,
        'timeout_ms': 45 * 1000
    }
    print(args)

    try:
        await conn.pair(**args)
        print('pairing success!!')
    except OSError as e:
        print(e)
        print('pairing end????')

async def main():
    device = await find_blood_pressure_sensor()
    if not device:
        return

    try:
        print('connection start')
        conn = await device.connect(timeout_ms=2_000)
        print(conn)

        print('mtu', conn.mtu)
        await conn.exchange_mtu(mtu=23, timeout_ms=5_000) #最小(血圧計がそれしか使えないようだ)
        #デフォだとちょいちょいタイムアウトこける？
        print('mtu', conn.mtu)
        print('connected')

        #await printDeviceInfo(conn)
        #await printDatetime(conn)

        #await pairing(conn)
        #await setDatetime(conn)
        #await printBloodPressure(conn)

        from bloodPressureMeasurementParser import parse
        records = await printBloodPressureRecord(conn)
        print(records)
        for record in records:
            print(record)
            parse(raw=record)

        await conn.disconnect()

    except asyncio.TimeoutError:
        print('Timeout')

def b():
    import btn
    btn.standby()
