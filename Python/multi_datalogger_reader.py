"""

 For running with Joe's Simulator:
    sudo python multi_datalogger_reader.py --port 1045 --ip 192.168.7.2

 For running in HARP lab:
    sudo python multi_datalogger_reader.py

 testing program for getting data from 4 ch HARP 3B04 230307
 two channels at 200kHz/ch


 In order for the changes that this function makes be observable across both threads, this function relies on global variables

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
from scipy.signal import filtfilt, ellip, freqz, lfilter
from process_data import  ThresholdDetect, SegmentPulses, PreprocessSegment1550, PreprocessSegment1240, WritePulseAmplitudes
from TDOA_estimation import EllipticFilter, GCC_PHAT, CrossCorr
from utils import CheckSystem, IntegrityCheck

print('This code has been tested for python version 3.11.6, your version is:', sys.version)

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=50000, type=int)                           # port to listen to for UDP packets
parser.add_argument('--ip', default="192.168.100.220",type=str)                  # IP address of data logger (simulator)
parser.add_argument('--fw', default = 1550, type=int)                            # firmware version
parser.add_argument('--output_file', default = "clicks_data.txt", type=str)      # output file for logging time and peak amp. of pulses
args = parser.parse_args()
output_file = open(args.output_file, 'w')                 # clear contents in output file

UDP_IP = args.ip                                          # IP address of data logger or simulator 
UDP_PORT = args.port                                      # Port to listen for UDP packets

print('Listening to IP address, ', UDP_IP,' and port ',UDP_PORT)

### Set the program to run at high priority on the system ###

thisSystem = CheckSystem()
if thisSystem == "Unix":
    print("You are using a UNIX-based system.")
    try:
        NICE_VAL = 15                    # set the "nice" value (OS priority) of the program. [-20, 19], lower gives more priority 
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

TIME_WINDOW = .01                                                 # fraction of a second to consider  
NUM_PACKS_DETECT = round(TIME_WINDOW * SAMPLE_RATE / SAMPS_PER_CHANNEL)  # the number of data packets that are needed to perform energy detection 

print('Bytes per packet:       ', REQUIRED_BYTES)
print('Microseconds between packets:   ', MICRO_INCR)
print('Number of channels:     ', NUM_CHAN)
print('Data bytes per channel: ', DATA_BYTES_PER_CHANNEL)
print("Detecting over a time window of ",TIME_WINDOW," seconds, using ",NUM_PACKS_DETECT, " packets") 

def restartListener():
    """
    Functionality:
    This function is responsible for (re)starting the listener.
    The socket connection is re(set). The  buffer (dataBuffer) and segment to be processed (dataSegment) are cleared.

    """
    
    global dataBuffer, dataSegment, dataTimes, udpSocket
    
    with udpSocketLock:
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
    
    with dataBufferLock:
        dataBuffer = queue.Queue()
    with dataSegmentLock:
        dataSegment = np.array([])
    with dataTimesLock:
        dataTimes = np.array([])

def UdpListener():
    """
    Functionality:
    This function serves as a UDP listener that continuously listens for incoming UDP packets.
    It receives UDP packets of a specified size, checks their length, and if they meet the expected size, 
    increments a packet counter. The received data is then placed into a shared buffer for further processing.

    Global Variables:
    - dataBuffer: shared buffer to store received data packets.
    - packetCounter: counter to keep track of the number of received packets.
    - udpSocket: the UDP socket used for listening to incoming packets.    
    """

    global dataBuffer, packetCounter, udpSocket
    startPacketTime = time.time()
    printInterval = 500
    while True:
        with udpSocketLock:
            dataBytes, addr1 = udpSocket.recvfrom(PACKET_SIZE + 1)  # + 1 to detect if more bytes are received
        packetCounter += 1
        if packetCounter % printInterval == 0:
            with dataBufferLock:
                qSize = dataBuffer.qsize()
            print("Num packets received is ", packetCounter, (time.time() - startPacketTime)/printInterval,qSize,packetCounter - qSize)
            startPacketTime = time.time()
        with dataBufferLock:
            dataBuffer.put(dataBytes)  # Put received data into the buffer

def DataProcessor():
    """
    Functionality:
    This function serves as a data processor that continuously processes data segments retrieved from a shared buffer (dataBuffer).
    It extracts the necessary information from the received data, such as timestamps and sample values, performs adjustments,
    and stores the processed data into a segment (dataSegment). This segment is then processed.

    Global Variables:
    - dataBuffer: Shared buffer containing received data packets.
    - dataSegment: Segment of data to be processed.
    - dataTimes: Array containing timestamps associated with data segments. 
    """
    global dataBuffer, dataSegment, dataTimes
    
    ### define filter - should be passed as function arguement
    order = 4
    ripple_dB = 0.1
    cutoffFrequency = 10000
    b, a = EllipticFilter(order, ripple_dB, cutoffFrequency, SAMPLE_RATE)

    previousTime = False # initalize the previous packet time to False, update with every new packet
    while True:

        # begin new data segment 
        with dataSegmentLock:
            dataSegment = np.array([])
        with dataTimesLock:
            dataTimes = np.array([])
        
        while len(dataSegment) < (NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN):
            with dataBufferLock:
                if dataBuffer.qsize() > 0: # if buffer is not empty, get byte packet from buffer 
                    dataBytes = dataBuffer.get()
                else:
                    continue
            
            # check packet length
            if len(dataBytes) != PACKET_SIZE:
                print('Error: recieved incorrect number of packets')
                previousTime = False
                restartListener()
                continue

            ####
            # Unpacks the binary dataBytes into a tuple according to a specified format,
            # where 'B' denotes unsigned char (1 byte) and 'H' denotes unsigned short (2 bytes),
            # with the format dynamically constructed based on the length of dataBytes and HEAD_SIZE.
            # '>' is a format character that specifies big-endian byte order.
            dataUnpacked = struct.unpack('>' + 'B'*HEAD_SIZE + 'H'*((len(dataBytes) - HEAD_SIZE) // 2), dataBytes)
            ####

            dataTime = dataUnpacked[:HEAD_SIZE]
            dataSamples = np.array(dataUnpacked[HEAD_SIZE:]) - 2**15  # Extract and adjust dataSamples
            #print("dataSamples: ",dataSamples)
            #return
            us = (dataBytes[6],dataBytes[7],dataBytes[8],dataBytes[9])
            microSeconds = int.from_bytes(us,'big')         # get micro seconds from dataTime (unsigned char ist)
            dateTime = datetime.datetime(2000+dataTime[0], dataTime[1], dataTime[2], dataTime[3], dataTime[4], dataTime[5], microSeconds, tzinfo=datetime.timezone.utc)
            
            # compare the current packet's time to the previous packet time
            if previousTime and ((dateTime - previousTime).microseconds != MICRO_INCR):
                print("Error: Time not incremented by ", MICRO_INCR)
                previousTime = False
                restartListener()
                continue

            with dataTimesLock:
                dataTimes = np.append(dataTimes, dateTime) 
            with dataSegmentLock:
                dataSegment = np.append(dataSegment,dataSamples)

            previousTime = dateTime
        
        with dataSegmentLock:
            ch1, ch2, ch3, ch4 = PreprocessSegment(dataSegment, NUM_PACKS_DETECT, NUM_CHAN, SAMPS_PER_CHANNEL)
        with dataTimesLock:
            #values = SegmentPulses(ch1, dataTimes, SAMPLE_RATE, 2500, False) # Set true to save segmented pulses
            values = ThresholdDetect(ch1,dataTimes, SAMPLE_RATE, 300)
        if values == None: # if no pulses were detected to segment, then get next segment
            continue
       
        clickTimes, clickAmplitudes, clickStartPoints, clickEndPoints = values
        #WritePulseAmplitudes(clickTimes, clickAmplitudes, args.output_file)
        ### detection code
        ch1Filtered = filtfilt(b, a, ch1)
        ch2Filtered = filtfilt(b, a, ch2)
        ch3Filtered = filtfilt(b, a, ch3)
        ch4Filtered = filtfilt(b, a, ch4)

        dataMatrixFiltered = np.vstack(np.array([ch1Filtered, ch2Filtered,
                             ch3Filtered, ch4Filtered]))
        
        tdoaEstimates = GCC_PHAT(dataMatrixFiltered, SAMPLE_RATE, max_tau=None, interp=16)
        #print('here')
        print(tdoaEstimates)
### In order for the changes that one thread makes to shared variables be observable across both threads, global variables are needed

# Global Variables: counter and socket 
packetCounter = 0                                              # counter for the number of packets received
udpSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)   # udp socket that uses IPV4

# Global variables: data containers
dataBuffer = queue.Queue()                                     # buffer (Queue) for communication between threads
dataSegment = np.array([])                                     # decoded data from buffer to be processed
dataTimes = np.array([])                                       # timestamps associated with data inside dataSegment

# Global Variables: locks
dataBufferLock = threading.Lock()                                  # a lock for safe thread access to variable "dataBuffer"
dataSegmentLock = threading.Lock()                                 # a lock for safe thread access to variable "dataSegment"
dataTimesLock = threading.Lock()                                   # a lock for safe thread access to variable "dataTimes"
udpSocketLock = threading.Lock()                                   # a lock for safe thread access to variable "udpSocket"

restartListener()

udpThread = threading.Thread(target=UdpListener)
processorThread = threading.Thread(target=DataProcessor)

udpThread.start()
processorThread.start()

udpThread.join()
processorThread.join()
