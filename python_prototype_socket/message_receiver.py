import socket
from time import sleep


def test_message(addr, msg):
    print(addr, " - ", msg)
    msg = bytearray(msg)
    print(f"message [{msg[0]}|{msg[1]}|{msg[2]}|{msg[3]}|{msg[4]}]")
    print("binary [{0:08b}|{1:08b}|{2:08b}|{3:08b}|{4:08b}]".format(*msg))


s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('localhost', 8083))

while True:
    data, addr = s.recvfrom(1024)
    if not data or data == b'':
        sleep(0.001)
        continue
    test_message(addr, data)
