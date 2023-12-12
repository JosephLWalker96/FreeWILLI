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
import psutil
import os
from utils import sleep, synthetic_click_generator, load_test_4ch_data

NICE_VAL = -15                     # set the "nice" value (OS priority) of the program. [-20, 19], lower gives more priority 

pid = os.getpid()
process = psutil.Process(pid)      # Get the process object for the current process
#process.nice(NICE_VAL)            # Set the process priority to high
#os.nice(NICE_VAL)

UDP_IP = "192.168.7.2"             # IP address of the destination
UDP_PORT = 1045                    # Port number of the destination

HEAD_SIZE = 12                     # packet head size (bytes)
NUM_CHAN = 4;                      # number of channels per packet
SAMPS_PER_CHANNEL = 155;           # samples per packet per channel, for 2 channels, this value is 5*62  = 310
BYTES_PER_SAMP = 2;                                             # bytes per sample
DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP;   # packet data size (bytes) = 1240
PACKET_SIZE = HEAD_SIZE + DATA_SIZE;                            # packet size (bytes) = 1252

REQUIRED_BYTES = 1252
DATA_BYTES_PER_CHANNEL = 310       # number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels

MICRO_INCR = .001550

## Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

## Make a fake initial time for the "recorded data"
date_time = datetime.datetime(2000+23, 11, 5, 1, 1, 1, tzinfo=datetime.timezone.utc)

#DATA = synthetic_click_generator(signal_length = 10000, click_dur = 100)
DATA = load_test_4ch_data(file_path = '../Data/joesdata.mat', scale = 2**15)

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
    
    #decoded_array = np.frombuffer(data_packet, dtype=np.uint16)
    #print('SIMULATOR - NUMPY', np.array(decoded_array - 2**15).astype(np.int16))
    
    #decoded_array = np.array(struct.unpack('<' + 'H' * int((REQUIRED_BYTES-12)/2), data_packet))
    #print('SIMULATOR - STRUCT', np.array(decoded_array - 2**15).astype(np.int16))

    #### Send the data
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    
    ### Sleep for a set time
    run_time = time.time() - start_time
    sleep(MICRO_INCR - run_time)
    #print(time.time() - start_time)
    #time.sleep(1)
    
    if flag == 4000:
        break
    flag += 1

# Close the socket
sock.close()

