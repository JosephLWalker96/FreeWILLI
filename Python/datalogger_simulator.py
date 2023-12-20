"""
This is a program for simulating the data logger
This program may require root privileges 

Run this program on mac/linux by:
$ sudo python UDP_send_datalogger_format.py

Setting the "nice" or priority value as done on lines 24 and 25 may not be possible on Windows.
  
"""

import struct                      # library for converting data to binary
import socket                      # library for establishing UDP communication 
import numpy as np
import time
import datetime
from datetime import timedelta
import argparse
import psutil
import os
import sys
from utils import sleep, synthetic_click_generator, load_test_4ch_data_1550, load_test_4ch_data_1240

NICE_VAL = -15                     # set the "nice" value (OS priority) of the program.
pid = os.getpid()
process = psutil.Process(pid)      # Get the process object for the current process
process.nice(NICE_VAL)             # Set the process priority to high
os.nice(NICE_VAL)

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default = 1045, type=int)
parser.add_argument('--ip', default = "192.168.7.2", type=str)
parser.add_argument('--fw', default = 1550, type=int)
args = parser.parse_args() # Parsing the arguments

UDP_IP = args.ip                   # IP address of the destination
UDP_PORT = args.port               # Port number of the destination

print('Simulating firmware version: ', args.fw)
# import variables according to firmware version specified
if args.fw == 1550:
    from firmware_1550 import *
    DATA = load_test_4ch_data_1550(file_path = '../Data/joesdata.mat', scale = 2**15)
elif args.fw == 1250:
    from firmware_1240 import *
    DATA = load_test_4ch_data_1240(file_path = '../Data/joesdata.mat', scale = 2**15, SAMPS_PER_CHANNEL)
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

print('Bytes per packet:       ', REQUIRED_BYTES)
print('Time between packets:   ', MICRO_INCR)
print('Number of channels:     ', NUM_CHAN)
print('Data bytes per channel: ', DATA_BYTES_PER_CHANNEL)

## Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

## Make a fake initial time for the "recorded data"
date_time = datetime.datetime(2000+23, 11, 5, 1, 1, 1, tzinfo=datetime.timezone.utc)

DATA = np.array(DATA,dtype=np.uint16)      # convert data to unsigned 16 bit integers
DATA_bytes = DATA.tobytes()                # convert the data to bytes using big_endian

flag = 0
while(True):
    start_time = time.time()
    
    #### Create the time header
    year = int(date_time.year - 2000)
    month = int(date_time.month)
    day = int(date_time.day)
    hour = int(date_time.hour)
    minute = int(date_time.minute)
    second = int(date_time.second)
    microseconds = int(date_time.microsecond)
    
    print("sending: ", microseconds)
    
    time_list = [year,month,day,hour,minute,second,microseconds]
    
    date_time = date_time + timedelta(microseconds=int(MICRO_INCR * 1e6)) # increment the time for next packet

    time_pack = struct.pack("BBBBBB", *np.array([year,month,day,hour,minute,second]))
    microseconds_pack  = microseconds.to_bytes(4, byteorder='big')
    zero_pack = struct.pack("H", 0)
    time_header = time_pack + microseconds_pack + zero_pack

    #### Create the data to send
    # Send a list of zeros as data
    #packet = time_header + bytes([0] * int(DATA_SIZE))
    
    data_packet = DATA_bytes[flag * DATA_SIZE:(flag+1) * DATA_SIZE]
    packet  = time_header + data_packet
    if len(packet) != PACKET_SIZE:
        print('ERROR: packet length error')
        break    

    #decoded_array = np.frombuffer(data_packet, dtype=np.uint16)
    #print('SIMULATOR - NUMPY', np.array(decoded_array - 2**15).astype(np.int16))
    
    #decoded_array = np.array(struct.unpack('<' + 'H' * int((REQUIRED_BYTES-12)/2), data_packet))
    #print('SIMULATOR - STRUCT', np.array(decoded_array - 2**15).astype(np.int16))

    sock.sendto(packet, (UDP_IP, UDP_PORT)) # send the packet
    
    ### Sleep for the correct time
    run_time = time.time() - start_time
    sleep(MICRO_INCR - run_time)
    
    ### Sleep for an arbitrary time (debugging)
    #sleep(2*MICRO_INCR)
    #time.sleep(1)
    
    if flag == 4000:
        print('Reached flag ',flag)
        break
    flag += 1

# Close the socket
sock.close()

