#===========================================
# NRF24L01 SENDING TEST
# THIS CODE IS THE SAME AS THE EXAMPLE FILE OF THE LIBRARY, except the play sound part
# https://github.com/bjarne-hansen/py-nrf24/blob/master/test/fixed-receiver.py
# most part of this file is NOT MY WORK, I COPIED FROM THE EXAMPLE FILE!!!
#===========================================

import argparse
from datetime import datetime
import struct
import sys
import time
import traceback
import os
from nrf24 import *
import pigpio


#
# A simple NRF24L receiver that connects to a PIGPIO instance on a hostname and port, default "localhost" and 8888, and
# starts receiving data with a fixed payload size of 9 bytes on the address specified.  
# Use the companion program "fixed-sender.py" to send data to it from a different Raspberry Pi.
#
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
    # PLEASE NOTE: payload_size=9 sets a default payload size for all pipes being opened.
    # PLEASE NOTE: PA level is set to MIN, because test sender/receivers are often close to each other, and then MIN works better.
    nrf = NRF24(pi, ce=25, payload_size=9, channel=100, data_rate=RF24_DATA_RATE.RATE_250KBPS, pa_level=RF24_PA.MIN)
    nrf.set_address_bytes(len(address))

    # Listen on the address specified as parameter
    nrf.open_reading_pipe(RF24_RX_ADDR.P1, address)
    
    # Display the content of NRF24L01 device registers.
    nrf.show_registers()

    # Enter a loop receiving data on the address specified.
    try:
        count = 0
        print(f'Receive from {address}')
        while True:

            # As long as data is ready for processing, process it.
            while nrf.data_ready():
                pipe = nrf.data_pipe()
                payload = nrf.get_payload()
            
                hexstr = ':'.join(f'{i:02x}' for i in payload)
                print(f"HEX: {hexstr}")
            
                # Convert payload to string (strip trailing 00 bytes)
                msg = payload.decode('utf-8', errors='ignore').rstrip('\x00')
                print(f"Message: {msg}")
            
                # If message == "Shoot", play the sound
                if msg == "Shoot":
                    print("Shoot command received! Playing Shoot.wav...")
                    os.system("aplay Shoot.wav")             

            time.sleep(0.1)
    except:
        traceback.print_exc()
        nrf.power_down()
        pi.stop()
