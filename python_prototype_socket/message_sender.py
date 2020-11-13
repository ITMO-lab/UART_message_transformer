import socket
from random import randint

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

adr = randint(0, 127)
counter = randint(0, 127)
command = [0b1000001, 0b1000101]
hash_sum = (adr ^ counter ^ command[0] ^ command[1]) & 0x7F

print(f"message [{0b10000000 + adr}|{counter}|{command[0]}|{command[1]}|{hash_sum}]")
print("binary [{0:08b}|{1:08b}|{2:08b}|{3:08b}|{4:08b}]".format(0b10000000 + adr, counter, *command, hash_sum))
s.sendto(bytes([0b10000000 + adr, counter, command[0], command[1], hash_sum]), ('localhost', 8082))
