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
from utils import * 
import multiprocessing  # Import multiprocessing module

SetHighPriority(15)  # Set this process to run the program at high priority (nice value = -15)

### Command-line argument parsing
parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=1045, type=int, help='UDP port to send data to')
parser.add_argument('--ip', default="192.168.7.2", type=str, help='IP address to send data to')
parser.add_argument('--data_dir', default="simulator_data/track132_5minchunks/", type=str, help='Directory containing .npy data files')
parser.add_argument('--fw', default=1240, type=int, help='Firmware version to simulate')
parser.add_argument('--loop', action='store_true', help='Enable looping over the data')
parser.add_argument('--stretch', action='store_true', help='Normalize data values to min and max range of unsigned 16-bit int')
parser.add_argument('--high_act', action='store_true', help='Enable high activity mode')
parser.add_argument('--cos_shift', action='store_true', help='Pad data variably according to a cosine wave')
parser.add_argument('--time_glitch', default=0, type=int, help='Simulate time glitch at specific flag')
parser.add_argument('--data_glitch', default=0, type=int, help='Simulate data glitch at specific flag')
parser.add_argument('--tdoa_sim', nargs='?', const=0, type=int, default=False, action=TDOASimAction, help='Channel offset amount')
parser.add_argument('--imu', action='store_true', help='Read in IMU data from file')

args = parser.parse_args()  # Parsing the arguments

print("TDOA_SIM val: ", args.tdoa_sim)

DATA_SCALE = 2**15

if args.ip == "self":
    args.ip = "127.0.0.1"  # Replace "self" with the loopback address

print('Simulating firmware version: ', args.fw)
print("Sending data to " + args.ip + " on port " + str(args.port))

### Import global variables according to firmware version specified
if args.fw == 1550:
    from firmware_config.firmware_1550 import *
elif args.fw == 1240:
    from firmware_config.firmware_1240 import *
else:
    print('ERROR: Unknown firmware version')
    sys.exit()  # Exiting the program

# List all .npy files in the specified directory
npy_files = sorted([os.path.join(args.data_dir, f) for f in os.listdir(args.data_dir) if f.endswith('.npy')])

IMU_byte_data = 0
if (args.imu):
    IMU_byte_data = ReadBinaryData("../../IMU_matlab/202518_stationary.imu")   
    print("packet size before IMU: ", PACKET_SIZE)
    PACKET_SIZE = PACKET_SIZE + len(IMU_byte_data[0])
    print("packet size After IMU: ", PACKET_SIZE)
    print(len(IMU_byte_data))

# Function to process each file
def process_npy_file(npy_file, args, return_dict):
    print("Loading file: ", npy_file)
    dataMatrix = np.load(npy_file, mmap_mode='r').T  # Load the .npy file with memory mapping

    if args.tdoa_sim is not False:
        dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), args.tdoa_sim, NUM_CHAN)
    elif args.cos_shift:
        shift = int(66 * np.cos((0) / 5))
        dataMatrixShifted = DuplicateAndShiftChannels(np.copy(dataMatrix), shift, NUM_CHAN)
    else:
        dataMatrixShifted = np.copy(dataMatrix)

    if args.fw == 1240:
        dataFlattened = InterleaveData(dataMatrixShifted)
    else:
        print("Error: only interleaving method for firmware version 1240 is implemented")
        sys.exit()

    dataFlattenedScaled = ScaleData(dataFlattened, DATA_SCALE, args.stretch)
    dataBytes = ConvertToBytes(dataFlattenedScaled)

    return_dict['dataBytes'] = dataBytes  # Pass data back using the shared dictionary

# Initialize socket and datetime
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
dateTime = datetime.datetime(2000 + 23, 11, 5, 1, 1, 1, tzinfo=datetime.timezone.utc)

manager = multiprocessing.Manager()
return_dict = manager.dict()

# Preload the first file
process_npy_file(npy_files[0], args, return_dict)
dataBytes_current = return_dict['dataBytes']

# Start preloading the next file if available
print("here")
if len(npy_files) > 1:
    next_file_index = 1
    return_dict_next = manager.dict()
    process_next = multiprocessing.Process(target=process_npy_file, args=(npy_files[next_file_index], args, return_dict_next))
    process_next.start()
else:
    process_next = None

current_file_index = 0

#print("starting in 1 sec: ")
#time.sleep(1)
while True:
    firstRead = 1
    startTime = time.time()
    flag = 0
    loopCounter = 0

    while True:
        if firstRead == 0:
            startTime = time.time()

        atEnd = len(dataBytes_current) // DATA_SIZE == flag  # Check if at the end of the data
        if args.loop and atEnd:
            flag = 0
        elif args.cos_shift and atEnd:
            # Handle cos_shift case if needed
            pass  # Add appropriate handling if required
        else:
            pass  # Continue normally

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
            dataPacket = dataBytes_current[(byteIndex - offset):(byteIndex - offset) + DATA_SIZE]
        else:
            dataPacket = dataBytes_current[flag * DATA_SIZE:(flag + 1) * DATA_SIZE]

        packet = timeHeader + dataPacket
        
        if (args.imu):
            imu_data = IMU_byte_data[flag % len(IMU_byte_data)]
            imuDataPack = struct.pack("B"*len(imu_data), *imu_data)
            print(imuDataPack)
            packet = packet + imuDataPack

        if args.time_glitch > 0:
            if args.time_glitch == flag:
                dateTime = dateTime + timedelta(microseconds=int(103))
        elif args.data_glitch > 0:
            if args.data_glitch == flag:
                packet = packet + packet[:30]

        if len(packet) != PACKET_SIZE and args.data_glitch == 0:
            print('ERROR: packet length error')
            print('FLAG: ', flag)
            break

        sock.sendto(packet, (args.ip, args.port))  # Send the packet
        if firstRead == 1:
            firstRead = 0
            print("Transfer time: ", time.time() - startTime)

        dateTime = dateTime + timedelta(microseconds=int(MICRO_INCR))  # Increment the time for the next packet
        runTime = time.time() - startTime
        sleepTime = (MICRO_INCR) * 1e-6 - runTime
        Sleep(sleepTime)

        flag += 1

        # using this to test IMU functionality
        #if flag == 150:
        #    adsfa

        # Exit the loop if the end is reached and not looping
        if atEnd and not args.loop and not args.cos_shift:
            break

    # Wait for the next file to be loaded
    if process_next is not None:
        process_next.join()

    # Move to the next file
    current_file_index += 1
    if current_file_index >= len(npy_files):
        break  # No more files to process

    # Set dataBytes_current to dataBytes_next
    dataBytes_current = return_dict_next['dataBytes']

    # Start preloading the next file if available
    next_file_index = current_file_index + 1
    if next_file_index < len(npy_files):
        return_dict_next = manager.dict()
        process_next = multiprocessing.Process(target=process_npy_file, args=(npy_files[next_file_index], args, return_dict_next))
        process_next.start()
    else:
        process_next = None

sock.close()  # Close the socket
