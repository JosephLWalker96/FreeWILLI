import struct
import socket
import numpy as np
import time
import datetime
from datetime import timedelta
import argparse
import psutil
import os
import sys
from utils import SetHighPriority, Sleep, Normalize, DuplicateAndShiftChannels, InterleaveData, ScaleData, ConvertToBytes, TDOASimAction

SetHighPriority(15)  # Set this process to run the program at high priority (nice value = -15)

### Command-line argument parsing
parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=1045, type=int, help='UDP port to send data to')
parser.add_argument('--ip', default="192.168.7.2", type=str, help='IP address to send data to')
parser.add_argument('--data_dir', default="../Data/track132_5minchunks/", type=str, help='Directory containing .npy data files')
parser.add_argument('--fw', default=1240, type=int, help='Firmware version to simulate')
parser.add_argument('--loop', action='store_true', help='Enable looping over the data')
parser.add_argument('--stretch', action='store_true', help='Normalize data values to min and max range of unsigned 16-bit int')
parser.add_argument('--high_act', action='store_true', help='Enable high activity mode')
parser.add_argument('--cos_shift', action='store_true', help='Pad data variably according to a cosine wave')
parser.add_argument('--time_glitch', default=0, type=int, help='Simulate time glitch at specific flag')
parser.add_argument('--data_glitch', default=0, type=int, help='Simulate data glitch at specific flag')
parser.add_argument('--tdoa_sim', nargs='?', const=0, type=int, default=False, action=TDOASimAction, help='Channel offset amount')

args = parser.parse_args()  # Parsing the arguments

print("TDOA_SIM val: ", args.tdoa_sim)

DATA_SCALE = 2**15

if args.ip == "self":
    args.ip = "127.0.0.1"  # Replace "self" with the loopback address

print('Simulating firmware version: ', args.fw)
print("Sending data to " + args.ip + " on port " + str(args.port))

### Import global variables according to firmware version specified
if args.fw == 1550:
    from Firmware_config.firmware_1550 import *
elif args.fw == 1240:
    from Firmware_config.firmware_1240 import *
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

# List all .npy files in the specified directory
npy_files = sorted([os.path.join(args.data_dir, f) for f in os.listdir(args.data_dir) if f.endswith('.npy')])

# Function to process each file
def process_npy_file(npy_file):
    print("Loading file: ",npy_file )
    dataMatrix = np.load(npy_file, mmap_mode='r').T  # Load the .npy file with memory mapping
    #print("dataMatrix size: ", dataMatrix.shape())
    
    if args.tdoa_sim is not False:
        dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), args.tdoa_sim, NUM_CHAN)
    elif args.cos_shift:
        shift = int(66*np.cos((0) / 5))
        dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), shift, NUM_CHAN)
    else:
        dataMatrixShifted = np.copy(dataMatrix)

    if args.fw == 1240:
        dataFlattened = InterleaveData(dataMatrixShifted)
    else:
        print("Error: only interleaving method for firmware version 1240 is implemented")
        sys.exit()

    #del dataMatrixShifted
    dataFlattenedScaled = ScaleData(dataFlattened, DATA_SCALE, args.stretch)
    #del dataFlattened
    dataBytes = ConvertToBytes(dataFlattenedScaled)
    #del dataFlattenedScaled

    return dataBytes

# Processing files sequentially
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
dateTime = datetime.datetime.now()

for npy_file in npy_files[2:]:
    firstRead = 1
    startTime = time.time()
    dataBytes = process_npy_file(npy_file)
    flag = 0
    loopCounter = 0

    while True:
        if firstRead == 0:
            startTime = time.time()

        atEnd = len(dataBytes) // DATA_SIZE == flag  # Check if at the end of the data
        if args.loop and atEnd:
            flag = 0
        elif args.cos_shift and atEnd:
            cosLoadStart = time.time()
            loopCounter += 1
            shift = int(66 * np.cos((loopCounter + 1) / 5))
            print("Loading new data: ", shift)
            dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), shift, NUM_CHAN)
            if args.fw == 1240:
                dataFlattened, _ = InterleaveData(dataMatrixShifted)
            else:
                print("Error: only interleaving method for firmware version 1240 is implemented")
                sys.exit()

            del dataMatrixShifted
            dataFlattenedScaled = ScaleData(dataFlattened, DATA_SCALE, args.stretch)
            del dataFlattened
            dataBytes = ConvertToBytes(dataFlattenedScaled)
            del dataFlattenedScaled

            flag = 0
            startTime = time.time()
            print("Applying new shift time: ", startTime - cosLoadStart)

        year = int(dateTime.year - 2000)
        month = int(dateTime.month)
        day = int(dateTime.day)
        hour = int(dateTime.hour)
        minute = int(dateTime.minute)
        second = int(dateTime.second)
        microseconds = int(dateTime.microsecond)


        timePack = struct.pack("BBBBBB", *np.array([year, month, day, hour, minute, second]))
        microPack = microseconds.to_bytes(4, byteorder='big')
        zeroPack = struct.pack("BB", 0, 0)
        timeHeader = timePack + microPack + zeroPack

        if args.high_act:
            byteIndex = highAmplitudeIndex * NUM_CHAN * BYTES_PER_SAMP
            offset = 10 * NUM_CHAN * BYTES_PER_SAMP
            dataPacket = dataBytes[(byteIndex - offset):(byteIndex - offset) + DATA_SIZE]
        else:
            dataPacket = dataBytes[flag * DATA_SIZE:(flag + 1) * DATA_SIZE]

        packet = timeHeader + dataPacket

        if args.time_glitch > 0:
            if args.time_glitch == flag:
                dateTime = dateTime + timedelta(microseconds=int(103))
        elif args.data_glitch > 0:
            if args.data_glitch == flag:
                packet = packet + packet[:30]

        if len(packet) != PACKET_SIZE and args.data_glitch == 0:
            print('ERROR: packet length error')
            print('FLAG: ', flag)
            #sys.exit()
            break

        sock.sendto(packet, (args.ip, args.port))  # Send the packet
        if firstRead ==1:
            firstRead = 0
            print("Transfer time: ", time.time() - startTime)

        dateTime = dateTime + timedelta(microseconds=int(MICRO_INCR))  # Increment the time for the next packet
        runTime = time.time() - startTime
        sleepTime = (MICRO_INCR) * 1e-6 - runTime
        Sleep(sleepTime)
        #print("sleep time: ",sleepTime)

        flag += 1

        # Exit the loop if the end is reached and not looping
        if atEnd and not args.loop and not args.cos_shift:
            break

sock.close()  # Close the socket

