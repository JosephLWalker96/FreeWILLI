"""
 This code is for running a multithreaded version of datalogger_reader.py 

 For running with Joe's Simulator:
    sudo python multi_datalogger_reader.py --port 1045 --ip 192.168.7.2

 For running in HARP lab:
    sudo python multi_datalogger_reader.py

 testing program for getting data from 4 ch HARP 3B04 230307
 two channels at 200kHz/ch
 UDP 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
 1 datagram = 1 packet

 based on udpGetTimes1.m

 230920 smw
"""

import struct
import socket
import threading
import queue  
import numpy as np
import sys
import argparse
import time
import datetime
import psutil
import os
from process_data import *
from utils import CheckSystem

print('This code has been tested for python version 3.11.6, your version is:', sys.version)

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=50000, type=int)                           # port to listen to for UDP packets
parser.add_argument('--ip', default="192.168.100.220",type=str)                  # IP address of data logger or simulator
parser.add_argument('--fw', default = 1550, type=int)                            # firmware version
parser.add_argument('--output_file', default = "clicks_data.txt", type=str)      # output file for logging time and peak amp. of pulses
args = parser.parse_args()
output_file = open(args.output_file, 'w')                 # clear contents in file

UDP_IP = args.ip                                          # IP address of data logger or simulator 
UDP_PORT = args.port                                      # Port to listen for UDP packets

print('Listening to IP address, ', UDP_IP,' and port ',UDP_PORT)

### Set the program to run at high priority on the system ###
thisSystem = CheckSystem()
if thisSystem == "Unix":
    print("You are using a UNIX-based system.")
    try:
        NICE_VAL = -15                    # set the "nice" value (OS priority) of the program. [-20, 19], lower gives more priority 
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
    pid = os.getpid()
    print("PID:", pid)
    p = psutil.Process(pid)
    p.nice(psutil.HIGH_PRIORITY_CLASS)
else:
    print("You are not using a UNIX- or Windows-based system.")

### import variables according to firmware version specified ###
print('Assuming firmware version: ', args.fw)
if args.fw == 1550:
    from Firmware_config.firmware_1550 import *
    PreprocessSegment = PreprocessSegment1550
elif args.fw == 1240:
    from Firmware_config.firmware_1240 import *
    PreprocessSegment = PreprocessSegment1240
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

TIME_WINDOW = .5                                                 # fraction of a second to consider  
NUM_PACKS_DETECT = round(TIME_WINDOW * 100000 / SAMPS_PER_CHANNEL)  # the number of data packets that are needed to perform energy detection 

print('Bytes per packet:       ', REQUIRED_BYTES)
print('Microseconds between packets:   ', MICRO_INCR)
print('Number of channels:     ', NUM_CHAN)
print('Data bytes per channel: ', DATA_BYTES_PER_CHANNEL)
print("Detecting over a time window of ",TIME_WINDOW," seconds, using ",NUM_PACKS_DETECT, " packets") 

# Create a udpport object udpportObj that uses IPV4
def restartListener():
    '''
    This function is responsible for (re)starting the listener.
    The socket connection is re(set). The  buffer (dataBuffer) and segment to be processed (dataSegment) are cleared.

    In order for the changes that this function makes be observable across both threads, this function relies on global variables

    '''
    
    global dataBuffer, dataSegment, times, udpSocket
    
    with socketLock:
        udpSocket.close()
        udpSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if UDP_IP == "192.168.100.220": # IP address of the data logger
            print('Sending wake up data to IP address: ', UDP_IP)

            # need 100 bytes to get Open command through
            m1 = b'Open'
            m2 = bytearray(np.zeros((1,96),dtype=int))
            udpSocket.sendto(m1 + m2, (UDP_IP, UDP_PORT))
        elif UDP_IP == "192.168.7.2": # IP address of Joe's simulator
            udpSocket.bind((UDP_IP, UDP_PORT))
            print('Listening to simulator')
        elif UDP_IP == "self": # IP address of Joe's simulator
            udpSocket.bind(("127.0.0.1", UDP_PORT))
        else:
            print("ERROR: Unknown IP address" )
            sys.exit()
    
    with bufferLock:
        dataBuffer = queue.Queue()
    with segmentLock:
        dataSegment = np.array([])
        times = np.array([])

# UDP listener function to receive data and write to the buffer
def UdpListener():
    global dataBuffer, packetCounter, udpSocket
    while True:
        with socketLock:
            dataBytes, addr1 = udpSocket.recvfrom(PACKET_SIZE + 1)  # + 1 to detect if more bytes are received

        if len(dataBytes) != PACKET_SIZE: # check packet length
            print('Error: recieved incorrect number of packets')
            restartListener()
            continue
        packetCounter += 1
        if packetCounter % 500 == 0:
            print("Num packets received is ", packetCounter)
        with bufferLock:
            dataBuffer.put(dataBytes)  # Put received data into the buffer

# Function to process data from the buffer
def DataProcessor():
    global dataBuffer, dataSegment, times
    while True:
        dataSegment = np.array([])  # clear the segment
        times = np.array([])
        while len(dataSegment) < (NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN):
            if dataBuffer.qsize() > 0:
                with bufferLock:
                    dataBytes = dataBuffer.get()
            else:
                continue
            dataUnpacked = struct.unpack('<' + 'B'*HEAD_SIZE + 'H'*((len(dataBytes) - HEAD_SIZE) // 2), dataBytes)
            dataTime = dataUnpacked[:HEAD_SIZE]  # Extract dataTime
            dataSamples = np.array(dataUnpacked[HEAD_SIZE:]) - 2**15  # Extract and adjust dataSamples
            
            us = (dataBytes[6],dataBytes[7],dataBytes[8],dataBytes[9])
            microSeconds = int.from_bytes(us,'big')         # get micro seconds from dataTime (unsigned char ist)
            dateTime = datetime.datetime(2000+dataTime[0], dataTime[1], dataTime[2], dataTime[3], dataTime[4], dataTime[5], microSeconds, tzinfo=datetime.timezone.utc)
            times = np.append(times, dateTime) 
            dataSegment = np.append(dataSegment,dataSamples)        
            #print(dataSegment) 
        if not IntegrityCheck(dataSegment, times, MICRO_INCR):
            print('Error: Integrity check failed')
            restartListener()
        else:
            ch1, ch2, ch3, ch4 = PreprocessSegment(dataSegment, NUM_CHAN, SAMPS_PER_CHANNEL)
            values = SegmentPulses(ch1, times)
            if values == None:
                continue
            clickTimes, clickAmplitudes, clickStartPoints, clickEndPoints = values
            WritePulses(clickTimes, clickAmplitudes, args.output_file)


# Global Variables 
packetCounter = 0                           # count the number of packets received
udpSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
bufferLock = threading.Lock()               # a lock for safe buffer access
segmentLock = threading.Lock()               # a lock for safe buffer access
socketLock = threading.Lock()               # a lock for safe thread restarting 

dataBuffer = queue.Queue() # create a buffer (Queue) for communication between threads
dataSegment = np.array([])
times = np.array([])

restartListener()

udpThread = threading.Thread(target=UdpListener)
processorThread = threading.Thread(target=DataProcessor)

udpThread.start()
processorThread.start()
processorThread.join()
