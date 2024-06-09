"""

This script simulates the operation of a 4 channel data logger by reading prerecorded 
4 channel data from a specified file, formatting it according to a chosen firmware version, 
and sending it as UDP packets to a specified IP address and port.

The script performs the following steps:

1. **Read and Process Data:**
   - The data is read from a .mat file (e.g., ../Data/joesdata.mat).
   - The data is then scaled and formatted based on the firmware version specified by the user
     through the command-line argument `--fw`. Supported firmware versions include 1550 and 1240 (default).
   - If `--tdoa_sim` argument is provided, the script will either duplicate the channel 1 data across 
     the remaining 3 channels but stagger the data with a constant offset (--tdoa_sim const), or stagger
     the data according to a sine wave that varies with the 'flag' variablei (--tdoa_sim sin).

2. **Create UDP Packets:**
   - Each UDP packet consists of a header and a data payload.
   - The header contains timestamp information, with the initial time set arbitrarily.
   - The data payload is extracted from the processed data and appended to the header.

3. **Send UDP Packets:**
   - UDP packets are sent to the specified IP address and port at regular intervals.
   - The interval between packets is defined by the MICRO_INCR constant (microsecond increment).

4. **Simulate Conditions:**
   - The script can simulate continuous data streaming by looping over the data if the `--loop` argument is set.
   - High activity mode can be enabled with the `--high_act` argument, which modifies the data payloads.
   - The script can introduce intentional glitches in the timestamp (`--time_glitch`) or data (`--data_glitch`)
     at specified intervals.

5. **Logging and Debugging:**
   - Key information such as firmware version, destination IP, and port, as well as packet and channel details, are printed for reference.
   - The script ensures accurate timing for packet transmission, adjusting for runtime variations.

Note: Running as root can improve the quality of the simulation by providing more precise control over 
      sleep times used to control packet send timing.
Note: Some global variables are imported from Firmware_config.firmware_* where * depends on the value of --fw.

EXAMPLES:

To run this script on macOS/Linux and send UDP packets to computer with IPv4 192.168.100.225 with constant channel offset:
$ sudo python datalogger_simulator.py --ip 192.168.225 --tdoa_sim const

To run this script on macOS/Linux and send UDP packets to host computer in high activity mode and loop through data repeatedly:
$ sudo python datalogger_simulator.py --ip self --high_act --loop

COMMAND-LINE ARGUMENTS:

- `--port`        : UDP port to send data to (default: 1045).
- `--ip`          : IP address to send data to (default: "192.168.7.2"). Use "self" for localhost.
- `--data_path`   : Path of prerecorded data to use (default: "../Data/joesdata.mat").
- `--fw`          : Firmware version to simulate (default: 1550). Supported versions: 1550, 1240.
- `--loop`        : Enable looping over the data (default: False).
- `--high_act`    : Enable high activity mode (default: False).
- `--time_glitch` : Simulate a time glitch at a specific flag (default: 0).
- `--data_glitch` : Simulate a data glitch at a specific flag (default: 0).
- `--tdoa_sim`    : TDOA simulation method ("sin" or "const").

"""

import struct                      # For converting data to binary
import socket                      # For establishing UDP communication 
import numpy as np                 # For numerical operations
import time                        # For time-related functions
import datetime                    # For datetime manipulations
from datetime import timedelta     # For incrementing timestamps
import argparse                    # For parsing command-line arguments
import psutil                      # For system and process utilities
import os                          # For operating system interfaces
import sys                         # For system-specific parameters and functions
from utils import SetHighPriority, Sleep, Normalize, Load4ChannelDataset, DuplicateAndShiftChannels, InterleaveData, ScaleData, ConvertToBytes

SetHighPriority(15) # set this process to run the program at high priority (nice value = -15)

### Command-line argument parsing
parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=1045, type=int, help='UDP port to send data to')
parser.add_argument('--ip', default="192.168.7.2", type=str, help='IP address to send data to')
parser.add_argument('--data_path', default="../Data/joesdata.mat", type=str, help='prerecorded data to use')
parser.add_argument('--fw', default=1240, type=int, help='Firmware version to simulate')
parser.add_argument('--loop', action='store_true', help='Enable looping over the data')
parser.add_argument('--stretch', action='store_true', help='normalize data values min and max range of unsigned 16 bit int')
parser.add_argument('--high_act', action='store_true', help='Enable high activity mode')
parser.add_argument('--cos_shift', action='store_true', help='pad data variably according to a cosine wave')
parser.add_argument('--time_glitch', default=0, type=int, help='Simulate time glitch at specific flag')
parser.add_argument('--data_glitch', default=0, type=int, help='Simulate data glitch at specific flag')
parser.add_argument('--tdoa_sim', default = 0, type=int, help='channel offset amount') # IN DEVELOPMENT
args = parser.parse_args() # Parsing the arguments

DATA_SCALE = 2**15

if args.ip == "self":
    args.ip = "127.0.0.1"  # replace "self" with the loopback address

print('Simulating firmware version: ', args.fw)
print("Sending data to " + args.ip + " on port " + str(args.port))

### import glabal variables according to firmware version specified
if args.fw == 1550:
    from Firmware_config.firmware_1550 import *
elif args.fw == 1240:
    from Firmware_config.firmware_1240 import *
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

dataMatrix = Load4ChannelDataset(args.data_path)

if args.tdoa_sim:
    dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), args.tdoa_sim, NUM_CHAN)
elif args.cos_shift:
    shift = int(66*np.cos((0) / 5))
    dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), shift, NUM_CHAN)
else:
    dataMatrixShifted = np.copy(dataMatrix)

if args.fw == 1240:
    dataFlattened, highAmplitudeIndex = InterleaveData(dataMatrixShifted)
else:
    print("Error: only interleaving method for firmware version 1240 is implemented")
    sys.exit()

del dataMatrixShifted
dataFlattenedScaled= ScaleData(dataFlattened, DATA_SCALE, args.stretch)
del dataFlattened
dataBytes = ConvertToBytes(dataFlattenedScaled)
del dataFlattenedScaled


print('Bytes per packet:       ', REQUIRED_BYTES)
print('Time between packets:   ', MICRO_INCR)
print('Number of channels:     ', NUM_CHAN)
print('Data bytes per channel: ', DATA_BYTES_PER_CHANNEL)

print("###################################################################")
sys.exit() if input("Are the above values correct? [Y/n]") != 'Y' else print("Running simulator...")

###Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
   
### Make a fake initial time for the data
dateTime = datetime.datetime(2000+23, 11, 5, 1, 1, 1, tzinfo=datetime.timezone.utc)
absStartTime = time.time() 

flag = 0
loopCounter = 0
while(True):

    startTime = time.time()
    
    atEnd = len(dataBytes) // DATA_SIZE == flag # boolean value, if flag is at end of data
    if args.loop and atEnd:
        flag = 0
    elif args.cos_shift and atEnd: # get new sine shifted data
        cosLoadStart = time.time()
        loopCounter +=1
        shift = int(66*np.cos((loopCounter+1) / 5))
        print("Loading new data: ", shift)
        dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), shift, NUM_CHAN)
        if args.fw == 1240:
            dataFlattened, _ = InterleaveData(dataMatrixShifted)
        else:
            print("Error: only interleaving method for firmware version 1240 is implemented")
            sys.exit()

        del dataMatrixShifted
        dataFlattenedScaled= ScaleData(dataFlattened, DATA_SCALE, args.stretch)
        del dataFlattened
        dataBytes = ConvertToBytes(dataFlattenedScaled)
        del dataFlattenedScaled

        flag = 0
        startTime = time.time()
        print("Applying new shift time: ", startTime - cosLoadStart)
    
    
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
    zeroPack = struct.pack("BB", 0, 0)
    timeHeader = timePack + microPack + zeroPack
    
    if args.high_act:
        byteIndex = highAmplitudeIndex * NUM_CHAN * BYTES_PER_SAMP
        offset = 10 * NUM_CHAN * BYTES_PER_SAMP
        dataPacket = dataBytes[(byteIndex-offset):(byteIndex-offset) + DATA_SIZE]
    else:
        dataPacket = dataBytes[flag * DATA_SIZE:(flag+1) * DATA_SIZE]
    
    packet  = timeHeader + dataPacket
    
    ###simulate datalogger glitches
    if args.time_glitch > 0:
        if args.time_glitch == flag:
            dateTime = dateTime + timedelta(microseconds=int(103))
    elif args.data_glitch > 0:
        if args.data_glitch == flag:
            packet = packet + packet[:30]
    
    ### check packet size ###
    if len(packet) != PACKET_SIZE and args.data_glitch == 0:
        print('ERROR: packet length error')
        print('FLAG: ',flag)
        sys.exit()

    sock.sendto(packet, (args.ip, args.port)) # send the packet
    
    ### Sleep for the correct time
    runTime = time.time() - startTime
    Sleep((MICRO_INCR -60) * 1e-6 - runTime)
    
    ### Sleep for an arbitrary time (debugging)
    #sleep(2*MICRO_INCR)
    #time.sleep(1)
    
    if flag == 8000 and not args.loop and not args.cos_shift:
        print('Reached flag ',flag,time.time() - absStartTime)
        break
    flag += 1


sock.close() # Close the socket

