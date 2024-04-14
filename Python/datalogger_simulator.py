"""
This is a program for simulating the data logger
This program may require root privileges 

Run this program on mac/linux by:
$ sudo python datalogger_simulator.py

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
from utils import *

thisSystem = CheckSystem()
if thisSystem == "Unix":
    print("You are using a UNIX-based system.")
    try:
        NICE_VAL = -20                    # set the "nice" value (OS priority) of the program. [-20, 19], lower gives more priority 
        pid = os.getpid()
        process = psutil.Process(pid)     # Get the process object for the current process
        process.nice(NICE_VAL)            # Set the process priority to high
        os.nice(NICE_VAL)
    except PermissionError as e:
        print(f"Failed to set nice value: {e}")
    except psutil.AccessDenied as e:
        print(f"Failed to set nice value: {e}")
        print("Try running using $ sudo python ... ")
        sys.exit()
elif thisSystem == "Win":
    print("You are using a Windows-based system.")
    pid = os.getpid()  # Get PID of this process
    print("PID:", pid)
    p = psutil.Process(pid)
    p.nice(psutil.HIGH_PRIORITY_CLASS)  # Set to desired priority class
else:
    print("You are not using a UNIX- or Windows-based system.")

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default = 1045, type=int)
parser.add_argument('--ip', default = "192.168.7.2", type=str)
parser.add_argument('--fw', default = 1550, type=int)
parser.add_argument('--loop', action = 'store_true')
parser.add_argument('--time_glitch', default = 0, type=int)
parser.add_argument('--data_glitch', default = 0, type=int)
parser.add_argument('--tdoa_sim', action = 'store_true')
args = parser.parse_args() # Parsing the arguments

UDP_IP = args.ip                   # IP address of the destination
UDP_PORT = args.port
DATA_SCALE = 2**15
DATA_PATH = '../Data/joesdata.mat'
if args.ip == "self":
    UDP_IP = "127.0.0.1"         # Port number of the destination

print('Simulating firmware version: ', args.fw)
print("Sending data to " + UDP_IP + " on port " + str(UDP_PORT))




### import variables according to firmware version specified
if args.fw == 1550:
    from Firmware_config.firmware_1550 import *
    dataMatrix = LoadTest4chData1550(DATA_PATH, DATA_SCALE)
elif args.fw == 1240:
    from Firmware_config.firmware_1240 import *
    dataMatrix = LoadTest4chData1240(DATA_PATH, DATA_SCALE, 
    NUM_CHAN, SAMPS_PER_CHANNEL, args.tdoa_sim)
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

print('Bytes per packet:       ', REQUIRED_BYTES)
print('Time between packets:   ', MICRO_INCR)
print('Number of channels:     ', NUM_CHAN)
print('Data bytes per channel: ', DATA_BYTES_PER_CHANNEL)

### Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

### Make a fake initial time for the "recorded data"
dateTime = datetime.datetime(2000+23, 11, 5, 1, 1, 1, tzinfo=datetime.timezone.utc)
absStartTime = time.time() 

"""
dataMatrix = np.array(dataMatrix,dtype=np.float64)      # convert data to unsigned 16 bit integers
dataMatrix = dataMatrix - np.min(dataMatrix)
dataMatrix = dataMatrix / np.max(dataMatrix)
dataMatrix = dataMatrix * 65535
"""
dataMatrix = np.array(dataMatrix,dtype=np.uint16)      # convert data to unsigned 16 bit integers

print("DataMatrix: ", np.min(dataMatrix), np.max(dataMatrix))
formatString = '>{}H'.format(len(dataMatrix))          # encode data as big-endian
dataMatrixBytes= struct.pack(formatString, *dataMatrix)

flag = 0
print('HERE: ', len(dataMatrixBytes) // DATA_SIZE)
while(True):
    startTime = time.time()

    if args.loop:
        if len(dataMatrixBytes) // DATA_SIZE == flag:
            flag = 0
    
    ### Create the time header
    year = int(dateTime.year - 2000)
    month = int(dateTime.month)
    day = int(dateTime.day)
    hour = int(dateTime.hour)
    minute = int(dateTime.minute)
    second = int(dateTime.second)
    microseconds = int(dateTime.microsecond)
    
    dateTime = dateTime + timedelta(microseconds=int(MICRO_INCR)) # increment the time for next packet
    
    timePack = struct.pack("BBBBBB", *np.array([year,month,day,hour,minute,second]))
    microPack  = microseconds.to_bytes(4, byteorder='big')
    zeroPack = struct.pack("H", 0)
    timeHeader = timePack + microPack + zeroPack
    
    dataPacket = dataMatrixBytes[flag * DATA_SIZE:(flag+1) * DATA_SIZE]
    packet  = timeHeader + dataPacket
    
    ###simulate datalogger glitches
    if args.time_glitch > 0:
        if args.time_glitch == flag:
            dateTime = dateTime + timedelta(microseconds=int(103))
    elif args.data_glitch > 0:
        if args.data_glitch == flag:
            packet = packet + packet[:3]
    
    if len(packet) != PACKET_SIZE and args.data_glitch == 0:
        print('ERROR: packet length error')
        print('FLAG: ',flag)
        break    

    sock.sendto(packet, (UDP_IP, UDP_PORT)) # send the packet
    
    ### Sleep for the correct time
    runTime = time.time() - startTime
    Sleep((MICRO_INCR + 1000) * 1e-6 - runTime)
    
    ### Sleep for an arbitrary time (debugging)
    #sleep(2*MICRO_INCR)
    #time.sleep(1)
    
    if flag == 8000 and not args.loop:
        print('Reached flag ',flag,time.time() - absStartTime)
        break
    flag += 1
# Close the socket
sock.close()

