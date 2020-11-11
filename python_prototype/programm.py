import socket
from time import sleep

def crc(pcBlock, len):
    crc = 0xFF
    for iter in range(len):
        crc ^= pcBlock[iter]
        for i in range(8):
            if (crc & 0x80 != 0):
                crc = (crc << 1) ^ 0x31
            else:
                crc = crc << 1
    return crc % 0xFF


def message_handler(input_data):
    # Тут немного меняем тип данных, чтобы работать с массивом u_int8[]
    print(f"\n---\nИсходное сообщение {input_data}")

    message = [*bytearray(input_data), ]

    print(f"Его байты {message}")
    print("В двоичном виде [{0:08b}|{1:08b}|{2:08b}|{3:08b}|{4:08b}]".format(*message))

    # Далее по ТЗ нужно проферить хэш сумму. Код околосишный, то есть без фишек Питона.
    hash_sum = (message[0] ^ message[1] ^ message[2] ^ message[3]) & 0x7F
    if (hash_sum != message[4]):
        print("\n---\n---\n[ERROR] - Hash sum did not match")
        return

    # Процесс формирование сообщения результата. ~Сиподобный массив фиксированной длины.
    result_message = [0xFF, 0x02, 0x14, 0x00, 0x00]
    command = (message[2] << 7) + message[3]
    result_message[3] = command & 0xFF # Только младший байт команды.
    result_message[4] = crc(result_message, 4)

    print(f"\nНовое сообщение {bytes(result_message)}")
    print(f"Его байты {result_message}")
    print("В двоичном виде [{0:08b}|{1:08b}|{2:08b}|{3:08b}|{4:08b}]".format(*result_message))
    return bytes(result_message)


s1 = socket.socket()
s1.bind(('', 8001))
s1.listen(1)
while True:
    conn, addr = s1.accept()
    data = conn.recv(1024)
    if not data or data == b'':
        sleep(0.001)
        continue
    s2 = socket.socket()
    s2.connect(('localhost', 8002))
    message_to_send = message_handler(data)
    if message_to_send is not None:
        s2.send(message_to_send)
    s2.close()
conn.close()
