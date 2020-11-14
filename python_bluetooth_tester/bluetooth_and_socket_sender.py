import asyncio
import socket
from random import randint
from time import time
from bleak import BleakClient

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def new_message():
    adr = randint(0, 127)
    counter = randint(0, 127)
    command = [randint(0, 127), randint(0, 127)]
    hash_sum = (adr ^ counter ^ command[0] ^ command[1]) & 0x7F
    message = [0b10000000 + adr, counter, *command, hash_sum]
    print("{5}: send: [{0}|{1}|{2}|{3}|{4}]".format(*message, time()))
    print("{5}: binF: [{0:08b}|{1:08b}|{2:08b}|{3:08b}|{4:08b}]".format(*message, time()))
    print('>'*30)
    message = bytes(message)
    return message


def callback_handler(sender, data):
    print("{6}: recv: [{0}|{1}|{2}|{3}|{4}] : from {5}".format(*data, sender, time()))
    print("{5}: binF: [{0:08b}|{1:08b}|{2:08b}|{3:08b}|{4:08b}]".format(*data, time()))
    print('<'*30 + "\n")


async def run(address):
    async with BleakClient(address) as client:
        print("Connected: {0}".format(await client.is_connected()))
        await client.start_notify("0000ffe1-0000-1000-8000-00805f9b34fb", callback_handler)
        while asyncio.get_event_loop().is_running():
            message = new_message()
            await client.write_gatt_char("0000ffe1-0000-1000-8000-00805f9b34fb", message)
            s.sendto(bytes([*message]), ('localhost', 8082))
            await asyncio.sleep(0.1)
        client.disconnect()


if __name__ == "__main__":
    address = "B4:52:A9:07:DD:20"
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run(address))
