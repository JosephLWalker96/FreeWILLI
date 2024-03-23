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
import queue  # Queue for thread-safe communication between threads
import numpy as np
import sys
import argparse
import time
import datetime
import psutil
import os
from process_data import *

# The following 5 lines of code makes the program run at high priority 
NICE_VAL = -15                    # set the "nice" value (OS priority) of the program. [-20, 19], lower gives more priority 
pid = os.getpid()
process = psutil.Process(pid)     # Get the process object for the current process
process.nice(NICE_VAL)            # Set the process priority to high
os.nice(NICE_VAL)

print('This code has been tested for python version 3.11.6, your version is:', sys.version)

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=50000, type=int)
parser.add_argument('--ip', default="192.168.100.220",type=str)
parser.add_argument('--fw', default = 1240, type=int)
parser.add_argument('--output_file', default = "clicks_data.txt", type=str)
args = parser.parse_args() # Parsing the arguments
output_file = open(args.output_file, 'w')

UDP_IP = args.ip                                          # IP address of data logger or simulator 
UDP_PORT = args.port                                      # Port to listen for UDP packets
print('Listening to IP address, ', UDP_IP,' and port ',UDP_PORT)

# import variables according to firmware version specified
print('Assuming firmware version: ', args.fw)
if args.fw == 1550:
    from Firmware_config.firmware_1550 import *
    process_segment = process_segment_1550
elif args.fw == 1240:
    from Firmware_config.firmware_1240 import *
    process_segment = process_segment_1240
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
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
if UDP_IP == "192.168.100.220": # IP address of the data logger
    print('Sending wake up data to IP address: ', UDP_IP)

    # need 100 bytes to get Open command through
    m1 = b'Open'
    m2 = bytearray(np.zeros((1,96),dtype=int))
    sock.sendto(m1 + m2, (UDP_IP, UDP_PORT))
elif UDP_IP == "192.168.7.2": # IP address of Joe's simulator
    sock.bind((UDP_IP, UDP_PORT))
else:
    print("ERROR: Unknown IP address" )

time.sleep(2) # take time to make sure above parameters are correct
print('Listening...')

# UDP listener function to receive data and write to the buffer
packet_counter = 0
def udp_listener(udp_socket,buffer):
    global packet_counter 
    while True:
        dataB, addr1 = udp_socket.recvfrom(PACKET_SIZE)  # bytes object
        packet_counter += 1
        if packet_counter % 500==0:
            print("Num packets received is ", packet_counter)
        buffer.put(dataB)  # Put received data into the buffer

# Function to process data from the buffer
def data_processor(buffer):
    while True:
        data_segment = np.array([])
        times = np.array([])
        while len(data_segment) < (NUM_PACKS_DETECT * SAMPS_PER_CHANNEL):
            #segment = np.append(segment,buffer.get())
            dataB = buffer.get()
            dataI = struct.unpack('>' + 'B'*len(dataB),dataB) # convert bytes to unsigned char list
            ###lenJ = int(len(dataB) / 2)
            ###dataJ = struct.unpack('>' + 'H'*lenJ,dataB) # convert bytes to short integer list
            dataJ = np.frombuffer(dataB[12:], dtype=np.uint16)
            dataJ = np.array(dataJ - 2**15).astype(np.int16)
            #yy = dataI[0]
            #mm = dataI[1]
            #dd = dataI[2]
            #HH = dataI[3]
            #MM = dataI[4]
            #SS = dataI[5]
            us = (dataB[6],dataB[7],dataB[8],dataB[9])
            usec = int.from_bytes(us,'big')                                  # get micro seconds from dataI (unsigned char ist)
            date_time = datetime.datetime(2000+dataI[0], dataI[1], dataI[2], dataI[3], dataI[4], dataI[5], usec, tzinfo=datetime.timezone.utc)
            times = np.append(times, date_time) 
            data_segment = np.append(data_segment,dataJ)        
            ###ch1 = np.array(dataJ[6:lenJ-5:4]) - 2**15  # shift for two complement
            #ch1 = dataJ[0::NUM_CHAN]
            #print('HERE',ch1)
        
            #print("################## SEGMENT ####################")
            #print("length of segment ", len(segment))
            #print(segment[:1000]) 
        if not integrityCheck(data_segment, times, MICRO_INCR):
            restartListener()
        else:
            process_segment(data_segment, times, args.output_file)

# Create a buffer (Queue) for communication between threads
data_buffer = queue.Queue()

# Create and start the UDP listener thread
udp_thread = threading.Thread(target=udp_listener, args=(sock,data_buffer,))
udp_thread.daemon = False  # Daemonize the thread to exit when the main program exits
udp_thread.start()

# Create and start the data processor thread
processor_thread = threading.Thread(target=data_processor, args=(data_buffer,))
processor_thread.daemon =False 
processor_thread.start()

# Keep the main thread alive to allow other threads to run

# Wait for threads to finish (optional)
udp_thread.join()
processor_thread.join()

print('GLOBAL COUNTER: ', COUNTER)


"""
try:
    while True:
        pass
except KeyboardInterrupt:
    print("\nExiting...")
"""
