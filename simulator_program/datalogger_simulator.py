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
import multiprocessing

# Local utilities (assumes these functions exist in utils.py)
from utils import SetHighPriority, ReadBinaryData, DuplicateAndShiftChannels, InterleaveData, ScaleData, ConvertToBytes, Sleep, TDOASimAction

# Ensure this process runs with high priority.
SetHighPriority(15)  # nice value = -15

class ArgumentParserService:
    """Handles command-line argument parsing, providing a single responsibility."""
    @staticmethod
    def parseArguments():
        parser = argparse.ArgumentParser(description='Program command line arguments')
        parser.add_argument('--port', default=1045, type=int, help='UDP port to send data to')
        parser.add_argument('--ip', default="192.168.7.2", type=str, help='IP address to send data to')
        parser.add_argument('--data_dir', default="simulator_data/track132_5minchunks/", type=str, help='Directory containing .npy data files')
        parser.add_argument('--fw', default=1240, type=int, help='Firmware version to simulate')
        parser.add_argument('--loop', action='store_true', help='Enable looping over the data')
        parser.add_argument('--stretch', action='store_true', help='Normalize data values to the min/max range of 16-bit unsigned int')
        parser.add_argument('--high_act', action='store_true', help='Enable high activity mode')
        parser.add_argument('--cos_shift', action='store_true', help='Pad data variably according to a cosine wave')
        parser.add_argument('--time_glitch', default=0, type=int, help='Simulate time glitch at specific data chunk index')
        parser.add_argument('--data_glitch', default=0, type=int, help='Simulate data glitch at specific data chunk index')
        parser.add_argument('--tdoa_sim', nargs='?', const=0, type=int, default=False, action=TDOASimAction, help='Channel offset amount')
        parser.add_argument('--imu', action='store_true', help='Read in IMU data from file')
        parsedArgs = parser.parse_args()
        return parsedArgs

class NpyFileProcessor:
    """
    Responsible for loading and transforming .npy files into byte data for streaming.
    Complies with Single Responsibility Principle by focusing on data processing only.
    """
    @staticmethod
    def processNpyFile(npyFile, arguments, returnDict, 
                       packetSize, numChan, dataScale, 
                       stretch, tdoaSim, cosShift, fw):
        """
        Load the .npy file and prepare it for UDP transmission.
        The processed bytes are stored in a shared returnDict under 'dataBytes'.
        """
        print("Loading file:", npyFile)
        rawDataMatrix = np.load(npyFile, mmap_mode='r').T  # Transpose after loading

        # Apply channel duplication and shifting if TDOA or cos_shift is specified.
        if tdoaSim is not False:
            shiftedDataMatrix = DuplicateAndShiftChannels(np.copy(rawDataMatrix), tdoaSim, numChan)
        elif cosShift:
            # Example shift calculation; feel free to expand logic as needed
            shiftVal = int(66 * np.cos(0 / 5))
            shiftedDataMatrix = DuplicateAndShiftChannels(np.copy(rawDataMatrix), shiftVal, numChan)
        else:
            shiftedDataMatrix = np.copy(rawDataMatrix)

        # Only firmware 1240 interleaving logic is implemented
        if fw == 1240:
            interleavedData = InterleaveData(shiftedDataMatrix)
        else:
            print("Error: Only firmware 1240 interleaving is implemented in this script.")
            sys.exit()

        scaledInterleavedData = ScaleData(interleavedData, dataScale, stretch)
        processedDataBytes = ConvertToBytes(scaledInterleavedData)

        returnDict['dataBytes'] = processedDataBytes

class DataSimulator:
    """
    Manages the main simulation loop, sending data packets via UDP, handling IMU data (if any), 
    and orchestrating time or data glitches. 
    """
    def __init__(self, arguments):
        self.arguments = arguments

        # Handle IP override
        if self.arguments.ip == "self":
            self.arguments.ip = "127.0.0.1"

        # Firmware-specific imports
        if self.arguments.fw == 1550:
            import firmware_config.firmware_1550 as fwConfig
        elif self.arguments.fw == 1240:
            import firmware_config.firmware_1240 as fwConfig
        else:
            print('ERROR: Unknown firmware version')
            sys.exit()

        # Store firmware constants for use throughout
        self.packetSize = fwConfig.PACKET_SIZE
        self.numChannels = fwConfig.NUM_CHAN
        self.dataSize = fwConfig.DATA_SIZE
        self.bytesPerSample = fwConfig.BYTES_PER_SAMP
        self.microIncrement = fwConfig.MICRO_INCR

        # If certain firmware files define additional constants, reference them conditionally:
        # E.g. firmware_1550 or firmware_1240 might define `highAmplitudeIndex`
        if hasattr(fwConfig, 'highAmplitudeIndex'):
            self.highAmplitudeIndex = fwConfig.highAmplitudeIndex
        else:
            self.highAmplitudeIndex = 0  # or define another sensible default if not present

        # For scaling data
        self.DATA_SCALE = 2**15

        # Print initial settings
        print(f"Simulating firmware version: {self.arguments.fw}")
        print(f"Sending data to {self.arguments.ip} on port {self.arguments.port}")

        # If requested, read IMU data and update PACKET_SIZE accordingly
        self.imuByteData = None
        if self.arguments.imu:
            self.imuByteData = ReadBinaryData("../../IMU_matlab/202518_stationary.imu")
            print("Packet size before IMU:", self.packetSize)
            self.packetSize += len(self.imuByteData[0])
            print("Packet size after IMU:", self.packetSize)
            print("IMU samples loaded:", len(self.imuByteData))

        # Prepare list of .npy data files
        self.npyFiles = sorted([
            os.path.join(self.arguments.data_dir, f) 
            for f in os.listdir(self.arguments.data_dir) 
            if f.endswith('.npy')
        ])

        # Initialize multiprocessing manager and dictionary for data exchange
        self.manager = multiprocessing.Manager()
        self.returnDict = self.manager.dict()

        # Initialize socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Start date/time
        self.currentDateTime = datetime.datetime(2000 + 23, 11, 5, 1, 1, 1, tzinfo=datetime.timezone.utc)

    def preloadFirstFile(self):
        """
        Preload the first file's data synchronously.
        """
        NpyFileProcessor.processNpyFile(
            npyFile=self.npyFiles[0],
            arguments=self.arguments,
            returnDict=self.returnDict,
            packetSize=self.packetSize,
            numChan=self.numChannels,
            dataScale=self.DATA_SCALE,
            stretch=self.arguments.stretch,
            tdoaSim=self.arguments.tdoa_sim,
            cosShift=self.arguments.cos_shift,
            fw=self.arguments.fw
        )
        return self.returnDict['dataBytes']

    def startFilePreloadProcess(self, fileIndex, returnDict):
        """
        Launch a separate process to preload the next file's data.
        """
        processNext = multiprocessing.Process(
            target=NpyFileProcessor.processNpyFile,
            args=(
                self.npyFiles[fileIndex],
                self.arguments,
                returnDict,
                self.packetSize,
                self.numChannels,
                self.DATA_SCALE,
                self.arguments.stretch,
                self.arguments.tdoa_sim,
                self.arguments.cos_shift,
                self.arguments.fw
            )
        )
        processNext.start()
        return processNext

    def run(self):
        """
        Main loop that streams data from .npy files over UDP, handles optional looping, 
        time/data glitches, and IMU data concatenation.
        """
        currentDataBytes = self.preloadFirstFile()

        # Start preloading the next file if available
        nextProcess = None
        nextReturnDict = None
        if len(self.npyFiles) > 1:
            nextReturnDict = self.manager.dict()
            nextFileIndex = 1
            nextProcess = self.startFilePreloadProcess(nextFileIndex, nextReturnDict)

        currentFileIndex = 0

        while True:
            isFirstRead = True
            startTime = time.time()
            dataChunkIndex = 0  # renamed 'flag' -> 'dataChunkIndex'

            k = 0
            while True:
                k+=1
                # If not the first read, reset the start time for measuring interval
                if not isFirstRead:
                    startTime = time.time()
                atEnd = (len(currentDataBytes) // self.dataSize == dataChunkIndex)

                # Loop logic
                if self.arguments.loop and atEnd:
                    dataChunkIndex = 0
                elif self.arguments.cos_shift and atEnd:
                    # If cos_shift is being used and we've hit the end, handle here if needed
                    pass
                elif atEnd and not self.arguments.loop and not self.arguments.cos_shift:
                    # End of file reached
                    break

                # Prepare timestamp fields
                year = self.currentDateTime.year - 2000
                month = self.currentDateTime.month
                day = self.currentDateTime.day
                hour = self.currentDateTime.hour
                minute = self.currentDateTime.minute
                second = self.currentDateTime.second
                microseconds = self.currentDateTime.microsecond

                # Build the time header
                timePack = struct.pack("BBBBBB", year, month, day, hour, minute, second)
                microPack = microseconds.to_bytes(4, byteorder='big')
                zeroPack = struct.pack("BB", 0, 0)
                timeHeader = timePack + microPack + zeroPack

                # Select data slice (handle high activity mode if specified)
                if self.arguments.high_act:
                    byteIndex = self.highAmplitudeIndex * self.numChannels * self.bytesPerSample
                    offset = 10 * self.numChannels * self.bytesPerSample
                    dataPacket = currentDataBytes[(byteIndex - offset):(byteIndex - offset) + self.dataSize]
                else:
                    startByte = dataChunkIndex * self.dataSize
                    endByte = (dataChunkIndex + 1) * self.dataSize
                    dataPacket = currentDataBytes[startByte:endByte]

                # Combine time header + data
                packet = timeHeader + dataPacket

                # If IMU data is enabled, append the corresponding IMU data to the packet
                if self.arguments.imu and self.imuByteData is not None:
                    imuSample = self.imuByteData[dataChunkIndex % len(self.imuByteData)]
                    imuPacket = struct.pack(f"{len(imuSample)}B", *imuSample)
                    packet += imuPacket

                # Time glitch manipulation
                if self.arguments.time_glitch > 0 and self.arguments.time_glitch == dataChunkIndex:
                    # Example glitch: add 103 microseconds
                    self.currentDateTime += timedelta(microseconds=103)

                # Data glitch manipulation
                if self.arguments.data_glitch > 0 and self.arguments.data_glitch == dataChunkIndex:
                    # Duplicate first 30 bytes of the packet onto the end
                    packet += packet[:30]

                # Sanity check on packet size, unless a data glitch was intentionally introduced
                if len(packet) != self.packetSize and self.arguments.data_glitch == 0:
                    print('ERROR: Packet length mismatch.')
                    print('Data chunk index:', dataChunkIndex)
                    break
                

                #if k==130:
                #    adsfasdf
                

                # Send the UDP packet
                self.socket.sendto(packet, (self.arguments.ip, self.arguments.port))

                if isFirstRead:
                    isFirstRead = False
                    print("Transfer time (first packet):", time.time() - startTime)

                # Increment time for next packet
                self.currentDateTime += timedelta(microseconds=int(self.microIncrement))

                # Attempt to maintain real-time pacing
                elapsedRuntime = time.time() - startTime
                sleepTime = (self.microIncrement * 1e-6) - elapsedRuntime
                Sleep(sleepTime)

                dataChunkIndex += 1

            # Wait for the next file to finish preloading
            if nextProcess is not None:
                nextProcess.join()

            currentFileIndex += 1
            if currentFileIndex >= len(self.npyFiles):
                break  # No more files to process

            # Move to the newly loaded data
            currentDataBytes = nextReturnDict['dataBytes'] if nextReturnDict else None

            # Start preloading the following file (if any)
            nextFileIndex = currentFileIndex + 1
            if nextFileIndex < len(self.npyFiles):
                nextReturnDict = self.manager.dict()
                nextProcess = self.startFilePreloadProcess(nextFileIndex, nextReturnDict)
            else:
                nextProcess = None

        # Clean up
        self.socket.close()




if __name__ == "__main__":
    # Parse command-line arguments
    arguments = ArgumentParserService.parseArguments()
    dataSimulator = DataSimulator(arguments)
    dataSimulator.run()
