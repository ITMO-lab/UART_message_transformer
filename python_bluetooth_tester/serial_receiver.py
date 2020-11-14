import serial
from time import time

ser = serial.Serial('/dev/ttyACM0', baudrate=9600)

data = b''
pkg_len = 5
while True:
    data += ser.read_all()
    begin_sign = 0
    for i in range(len(data)):
        if data[i] & 0x80:
            begin_sign = i
            break
    data = data[begin_sign:]
    if len(data) >= pkg_len:
        print(f"{time()}: recv: [{data[0]}", end='')
        for sym in data[1:pkg_len]:
            print(f"|{sym}", end='')
        print("]")
        print("{0}: binF: [{1:08b}".format(time(), data[0]), end='')
        for sym in data[1:pkg_len]:
            print("|{0:08b}".format(sym), end='')
        print("]")
        print('-'*30)
        data = data[pkg_len + 1:]
