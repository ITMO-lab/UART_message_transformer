def crc(pcBlock, len):
    crc = 0xFF
    for iter in range(len):
        crc ^= pcBlock[iter]
        for i in range(8):
            if (crc & 0x80 != 0):
                crc = (crc << 1) ^ 0x31
            else:
                crc = crc << 1
    return crc & 0xFF


print(crc([0B11111111, 0B00000010, 0B00010100, 0B0101101], 4))
