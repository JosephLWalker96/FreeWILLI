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
import traceback
from scipy.signal import filtfilt, ellip, freqz, lfilter
from process_data import  ThresholdDetect, SegmentPulses, PreprocessSegmentInterleaved, PreprocessSegmentStacked, SaveDataSegment, WritePulseAmplitudes
from TDOA_estimation import EllipticFilter, GCC_PHAT, CrossCorr, GenerateFIR_Filter
from utils import SetHighPriority

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

SetHighPriority()            # set the program to run at a high priority on the system (nice value = -15)

### import variables according to firmware version specified ###
print('Assuming firmware version: ', args.fw)
if args.fw == 1550:
    from Firmware_config.firmware_1550 import *
    PreprocessSegment = PreprocessSegmentInterleaved
elif args.fw == 1240:
    from Firmware_config.firmware_1240 import *
    PreprocessSegment = PreprocessSegmentInterleaved
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

TIME_WINDOW = .01                                                         # fraction of a second to consider  
NUM_PACKS_DETECT = round(TIME_WINDOW * SAMPLE_RATE / SAMPS_PER_CHANNEL)  # the number of data packets that are needed to perform energy detection 

print('Bytes per packet:       ',         REQUIRED_BYTES)
print('Microseconds between packets:   ', MICRO_INCR)
print('Number of channels:     ',         NUM_CHAN)
print('Data bytes per channel: ',         DATA_BYTES_PER_CHANNEL)
print("Detecting over a time window of ", TIME_WINDOW," seconds, using ",NUM_PACKS_DETECT, " packets") 

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
        elif UDP_IP == "192.168.7.7": # IP address of Joe's simulator
            udpSocket.bind((UDP_IP, UDP_PORT))
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
    try:
        global dataBuffer, packetCounter, udpSocket
        startPacketTime = time.time()
        printInterval = 500
        while not errorEvent.is_set():
            
            
            with udpSocketLock:
                dataBytes, addr1 = udpSocket.recvfrom(PACKET_SIZE + 1)  # + 1 to detect if more bytes are received
                #print(dataBytes)
                #break
            packetCounter += 1
            if packetCounter % printInterval == 0:
                with dataBufferLock:
                    qSize = dataBuffer.qsize()
                print("Num packets received is ", packetCounter, (time.time() - startPacketTime)/printInterval,qSize,packetCounter - qSize)
                startPacketTime = time.time()
            with dataBufferLock:
                dataBuffer.put(dataBytes)  # Put received data into the buffer
    except Exception as e:
        print(f"Error occurred in UDP listener thread: {e}")
        traceback.print_exc()  # Print detailed traceback information
        errorEvent.set()

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
    try:
        global dataBuffer, dataSegment, dataTimes
        
        ### define IIR filter - should be passed as function arguement
        '''
        order = 4
        ripple_dB = 0.1
        cutoffFrequency = 10000
        b, a = EllipticFilter(order, ripple_dB, cutoffFrequency, SAMPLE_RATE)
        print('b: ', b)
        print('a: ', a)
        '''

        ### define FIR filter - should be passed as function arguement 
        cutoffFrequency = 10000  # Cutoff frequency for highpass filter
        numTaps = 31  # Number of taps for the FIR filter
        taps = GenerateFIR_Filter(cutoffFrequency, numTaps, SAMPLE_RATE)
        print(taps)


        previousTime = False # initalize the previous packet time to False, update with every new packet
        while not errorEvent.is_set():

            # begin new data segment 
            with dataSegmentLock:
                dataSegment = np.array([])
            with dataTimesLock:
                dataTimes = np.array([])
            
            while len(dataSegment) < (NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN):
                
                with dataBufferLock:
                    qSize = dataBuffer.qsize()
                if qSize < 1:
                    print("sleeping")
                    time.sleep(.2)
                    continue
                
                dataBytes = dataBuffer.get()
                
                ### check packet length
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
                
                ### compare the current packet's time to the previous packet time
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
            
            #if np.random.random() < .001:
            #    adsfasdfsaf

            with dataSegmentLock:
                ch1, ch2, ch3, ch4 = PreprocessSegment(dataSegment, NUM_PACKS_DETECT, NUM_CHAN, SAMPS_PER_CHANNEL)
            with dataTimesLock:
                #values = SegmentPulses(ch1, dataTimes, SAMPLE_RATE, 2500, False) # Set true to save segmented pulses
                values = ThresholdDetect(ch1,dataTimes, SAMPLE_RATE, 80)
            if values == None: # if no pulses were detected to segment, then get next segment
                continue
           
            clickTimes, clickAmplitudes, clickStartPoints, clickEndPoints = values
            #SaveDataSegment(clickTimes[0], dataSegment, ch1, ch2, ch3, ch4) 
            #print("pulse detected! ", clickAmplitudes[0])
            WritePulseAmplitudes(clickTimes, clickAmplitudes, args.output_file)
            
            ### IIR code
            '''
            ch1Filtered = filtfilt(b, a, ch1)
            ch2Filtered = filtfilt(b, a, ch2)
            ch3Filtered = filtfilt(b, a, ch3)
            ch4Filtered = filtfilt(b, a, ch4)
            '''

            ### FIR code
            #print('ch1 examples: ', ch1[:10])
            prefiltTime = time.time()            
            ch1 = lfilter(taps, 1.0, ch1)
            ch2 = lfilter(taps, 1.0, ch2)
            ch3 = lfilter(taps, 1.0, ch3)
            ch4 = lfilter(taps, 1.0, ch4)
            #print('P FIR: ', time.time() - prefiltTime)
            #print('ch1 filt examples: ', ch1[:10])

            dataMatrixFiltered = np.vstack(np.array([ch1, ch2, ch3, ch4]))
            
            gccStart = time.time()
            tdoaEstimates = GCC_PHAT(dataMatrixFiltered, SAMPLE_RATE, max_tau=None, interp=1)
            print("P GCC: ", time.time() - gccStart)
            #print('here')
            #print(tdoaEstimates)
            
    except Exception as e:
        print(f"Error occurred in Data Processor thread: {e}")
        traceback.print_exc()  # Print detailed traceback information
        errorEvent.set()

### In order for the changes that one thread makes to shared variables be observable across both threads, global variables are needed

# Global Variables: counter and socket 
packetCounter = 0                                                  # counter for the number of packets received
udpSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)       # udp socket that uses IPV4

# Global variables: data containers
dataBuffer = queue.Queue()                                         # buffer (Queue) for communication between threads
dataSegment = np.array([])                                         # decoded data pulled from buffer to be processed
dataTimes = np.array([])                                           # timestamps associated with data inside dataSegment

# Global Variables: locks
dataBufferLock = threading.Lock()                                  # a lock for safe thread access to variable "dataBuffer"
dataSegmentLock = threading.Lock()                                 # a lock for safe thread access to variable "dataSegment"
dataTimesLock = threading.Lock()                                   # a lock for safe thread access to variable "dataTimes"
udpSocketLock = threading.Lock()                                   # a lock for safe thread access to variable "udpSocket"

errorEvent = threading.Event()

while True: # Loop continuously so when a thread errors (errorEvent.set() occurs) threads will close and restart 
 
    restartListener()
    udpThread = threading.Thread(target=UdpListener)               # create a thread for listening for incoming UDP packets
    processorThread = threading.Thread(target=DataProcessor)       # create a thread for processing UDP packet data

    udpThread.start()
    processorThread.start()
    
    udpThread.join()
    processorThread.join()

    print("Restarting threads...")

    errorEvent = threading.Event()
    
    # Global variables: data containers
    dataBuffer = queue.Queue()                                     # buffer (Queue) for communication between threads
    dataSegment = np.array([])                                     # decoded data pulled from buffer to be processed
    dataTimes = np.array([])                                       # timestamps associated with data inside dataSegment
