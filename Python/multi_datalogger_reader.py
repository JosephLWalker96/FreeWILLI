# This code is for running a multithreaded version of datalogger_reader.py 
#
# For running with Joe's Simulator:
#    python multi_datalogger_reader.py --port 1045 --ip 192.168.7.2
#
# For running in HARP lab:
#    python multi_datalogger_reader.py
#
# testing program for getting data from 4 ch HARP 3B04 230307
# two channels at 200kHz/ch
# UDP 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
# 1 datagram = 1 packet
#
# based on udpGetTimes1.m
#
# 230920 smw

import struct
import socket
import threading
import queue  # Queue for thread-safe communication between threads
import numpy as np
import sys
import argparse
import time
from process_data import detect_click

#from utils import sleep
print('This code has been tested for python version 3.11.6, your version is:', sys.version)

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=50000, type=int)
parser.add_argument('--ip', default="192.168.100.220",type=str)

# Parsing the arguments
args = parser.parse_args()

hsz = 12;           # packet head size (bytes)
nchpp = 2;          # number of channels per packet
sppch = 5*62;       # samples per packet per channel = 310
bps = 2;            # bytes per sample
dsz = sppch * nchpp * bps;         # packet data size (bytes) = 1240
psz = hsz + dsz;    # packet size (bytes) = 1252
num_packs_detect = 5 # the number of data packets that are needed to perform energy detection 
blkinterval = 1550; # block/packet/datagram size microseconds = 1e6 * sppch/200e3

UDP_IP = args.ip
UDP_PORT = args.port

print(UDP_IP)
print(UDP_PORT)

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

# UDP listener function to receive data and write to the buffer
def udp_listener(udp_socket,buffer):

    while True:

        dataB, addr1 = udp_socket.recvfrom(psz)  # bytes object
        dataI = struct.unpack('>' + 'B'*len(dataB),dataB) # convert bytes to unsigned char list
        lenJ = int(len(dataB) / 2)
        dataJ = struct.unpack('>' + 'H'*lenJ,dataB) # convert bytes to short integer list
        yy = dataI[0]
        mm = dataI[1]
        dd = dataI[2]
        HH = dataI[3]
        MM = dataI[4]
        SS = dataI[5]
        us = (dataB[6],dataB[7],dataB[8],dataB[9])
        usec = int.from_bytes(us,'big')                                  # get micro seconds from dataI (unsigned char ist)
        time1 = yy, mm, dd, HH, MM, SS, usec
        #print(dataI6],dataI[7],dataI[8],dataI[9])
        print('recieved: ', usec) 
        ch1 = np.array(dataJ[6:lenJ-5:4]) - 2**15  # shift for two complement
        packet_data = {
            "usec": usec,
            "data":ch1 
        }
        buffer.put(packet_data)  # Put received data into the buffer
        buffer_length = data_buffer.qsize()
        print("Recieve Length of the buffer:", buffer_length)
# Function to process data from the buffer
def data_processor(buffer):
    while True:
        #if not buffer.empty():  # Check if the buffer is not empty
        if data_buffer.qsize() > num_packs_detect -1:
            for i in range(num_packs_detect): 
                data = buffer.get()  # Get data from the buffer
                # Process the data (Replace this with your processing logic)
                print("Processing data:", data["usec"])  # Example: Print decoded data


# Create a buffer (Queue) for communication between threads
data_buffer = queue.Queue()

# Create and start the UDP listener thread
udp_thread = threading.Thread(target=udp_listener, args=(sock,data_buffer,))
udp_thread.daemon = True  # Daemonize the thread to exit when the main program exits
udp_thread.start()

# Create and start the data processor thread
processor_thread = threading.Thread(target=data_processor, args=(data_buffer,))
processor_thread.daemon = True
processor_thread.start()

# Keep the main thread alive to allow other threads to run
try:
    while True:
        pass
except KeyboardInterrupt:
    print("\nExiting...")
