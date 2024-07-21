from collections import namedtuple
from struct import unpack_from
from utils import *

Header = namedtuple('Header', ['isKpa', 'hasTimestamp', 'hasPulseRate', 'hasUserID', 'hasStatus', 'reservedBit5', 'reservedBit6', 'reservedBit7'])

#血圧は、上、下、平均の順
Pressure = namedtuple('Pressure', ['systolic', 'diastolic', 'meanArterialPressure'])
Datetime = namedtuple('Datetime', ['year', 'month', 'day', 'hour', 'minute', 'second'])

def parse(raw:str|bytes = None):
    if isinstance(raw, str):
        raw = bytes.fromhex(raw)
    if not (isinstance(raw, bytes)):
        raise TypeError('hex(str) or bytes')
    
    result = dict()

    #下位の桁からbit解釈が必要　メソッドは順序を配列添え字とそろえて返してくれる
    header = Header(*byte_to_booltuple(raw[0]))
    result['header'] = header
    unread_raw = raw[1:]

    result['unit'] = 'kPa' if header.isKpa else 'mmHg'

    #血圧（上、下、平均）
    result['pressure'] = Pressure(*map(medfloat16_to_float, unpack_from('<HHH', unread_raw)))
    unread_raw = unread_raw[ (2*3) : ]

    if header.hasTimestamp:
        #タイムスタンプ
        result['timestamp'] = Datetime(*unpack_from('<HBBBBB',unread_raw))
        unread_raw = unread_raw[7:]

    if header.hasPulseRate:
        #脈拍
        result['pulseRate'] = medfloat16_to_float(unpack_from('<H',unread_raw)[0])
        unread_raw = unread_raw[2:]

    if header.hasUserID:
        unread_raw = unread_raw[1:]
        # うちの対応してないからパス

    if header.hasStatus:
        print('ステータス対応してるがパースだるい 0しかこねぇ')

    return result
