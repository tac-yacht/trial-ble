#1byte分をbit配列と解釈して、boolタプルに変換する　デフォルトは下位から
def byte_to_booltuple(byte, bit_order_little=True):
    if not(0 <= byte <= 0b1111_1111):
        raise ValueError('out range', byte)

    bitstr_nonfill = bin(byte)[2:]
    padding = 8 - len(bitstr_nonfill)
    bitstr = ['0'] * padding
    bitstr.extend(bitstr_nonfill)

    if bit_order_little:
        bitstr = bitstr[::-1]

    return tuple(map(bool,map(int,bitstr)))

# https://qiita.com/Nomisugi/items/293a02bce5903a298f00
# sは数値表現
# dはビット数
def signed_hex_to_int(s, d):
    return s - ((s>>(d-1))<<d)

# 16進数で2byte分直接指定する　0x0000
def medfloat16_to_float(v):
    if not(0 <= v <= 0xFF_FF):
        raise ValueError('out range', v)
    
    # 16bit
    exp=signed_hex_to_int( (v>>12), 4) #先頭4bitが指数部　[16-4 = 12] をシフトすると指数部になる
    mantissa=signed_hex_to_int( (0xfff&v), 12) #仮数部 指数部を消し去る
    return mantissa * (10 ** exp)
