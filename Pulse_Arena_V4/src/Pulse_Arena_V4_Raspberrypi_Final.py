#===========================================
# NRF24L01 SENDING TEST
# SOME PART OF THIS CODE IS THE SAME AS THE EXAMPLE FILE OF THE LIBRARY
# https://github.com/bjarne-hansen/py-nrf24/blob/master/test/fixed-receiver.py
# WE REFINED THE EXAMPLE FILE
# the function we add is:
# if nrf24 has received the specific code from esp, the raspberrypi will play the sound.
# IMPORTANT run sudo systemctl start pigpiod first
#===========================================


import argparse
from datetime import datetime
import struct
import sys
import time
import traceback
import os
import subprocess
from nrf24 import *
import pigpio

if __name__ == "__main__":

    print("Python NRF24 Simple Receiver Example.")

    # Parse command line argument.
    parser = argparse.ArgumentParser(prog="fixed-receiver.py", description="Simple NRF24 receiver with fixed payload size.")
    parser.add_argument('-n', '--hostname', type=str, default='localhost', help="Hostname for the Raspberry running the pigpio daemon.")
    parser.add_argument('-p', '--port', type=int, default=8888, help="Port number of the pigpio daemon.")
    parser.add_argument('address', type=str, nargs='?', default='1SNSR', help="Address to listen to (3 to 5 ASCII characters).")

    args = parser.parse_args()
    hostname = args.hostname
    port = args.port
    address = args.address

    # Verify that address is between 3 and 5 characters.
    if not (2 < len(address) < 6):
        print(f'Invalid address {address}. Addresses must be between 3 and 5 ASCII characters.')
        sys.exit(1)

    # Connect to pigpiod
    print(f'Connecting to GPIO daemon on {hostname}:{port} ...')
    pi = pigpio.pi(hostname, port)
    if not pi.connected:
        print("Not connected to Raspberry Pi ... goodbye.")
        exit()

    # Create NRF24L01 communication object with a fixed payload size of 9 bytes.
    nrf = NRF24(pi, ce=25, payload_size=9, channel=100, data_rate=RF24_DATA_RATE.RATE_250KBPS, pa_level=RF24_PA.MIN)
    nrf.set_address_bytes(len(address))

    # Listen on the address specified as parameter
    nrf.open_reading_pipe(RF24_RX_ADDR.P1, address)

    # Display the content of NRF24L01 device registers.
    nrf.show_registers()

    def play_wav(filename: str):
        """Play wav without blocking the receiver loop."""
        if not os.path.exists(filename):
            print(f"[AUDIO] File not found: {filename}")
            return
        # aplay prints to stdout sometimes; redirect to keep console clean
        subprocess.Popen(["aplay", filename],
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)

    # Enter a loop receiving data on the address specified.
    try:
        print(f'Receive from {address}')
        while True:

            while nrf.data_ready():
                pipe = nrf.data_pipe()
                payload = nrf.get_payload()

                hexstr = ':'.join(f'{i:02x}' for i in payload)
                print(f"HEX: {hexstr}")

                # Convert payload to string (strip trailing 00 bytes)
                msg = payload.decode('utf-8', errors='ignore').rstrip('\x00')
                print(f"Message: {msg}")

                # I added this, this part is when raspberrypi has received the code, it will play the sound.
                if msg == "Shoot":
                    print("Playing Shoot.wav...")
                    play_wav("Shoot.wav")

                elif msg == "Hit":
                    print("Playing ding.wav...")
                    time.sleep(0.3)
                    play_wav("ding.wav")
                elif msg == "Gameover":
                    print("Playing gameover.wav...")
                    time.sleep(1)
                    play_wav("gameover.wav")
            time.sleep(0.1)

    except:
        traceback.print_exc()
        nrf.power_down()
        pi.stop()
