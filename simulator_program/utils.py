import time
import numpy as np
import psutil
import os
from scipy.io import loadmat
import sys
import struct
import argparse
import scipy
print("scipy", scipy.__version__)




class TDOASimAction(argparse.Action):
    """
    Custom argparse action to handle the --tdoa_sim argument.

    This action class provides custom behavior for the --tdoa_sim command-line argument:
    - If the --tdoa_sim argument is not provided, it defaults to False.
    - If the --tdoa_sim argument is provided without a value, it sets tdoa_sim to 0.
    - If the --tdoa_sim argument is provided with a value, it sets tdoa_sim to that integer value.
    """

    def __call__(self, parser, namespace, values, option_string=None):
        # Check if the argument is given without a value
        if values is None:
            setattr(namespace, self.dest, 0)
        else:
            setattr(namespace, self.dest, int(values))

def LoadHydrophonePositions(filePath):
    #hyd_data = loadmat(filePath)  # Path to hydrophone data
    hp = np.loadtxt(filePath, delimiter=',')
    #hp = hyd_data['recPos']

    H = np.array([  # Reorder hydrophones to fit new TDOA order
        hp[1,:] - hp[0,:],
        hp[2,:] - hp[0,:],
        hp[3,:] - hp[0,:],
        hp[2,:] - hp[1,:],
        hp[3,:] - hp[1,:],
        hp[3,:] - hp[2,:],
    ])

    return H


def CheckSystem():
    """
    Checks to see if computing platform is a Unix based system. Unix is needed to set "nice" priority

    Also, check if computing platform is Windows-based system.

    """
    # List of common UNIX platform names
    unix_platforms = ['linux', 'darwin', 'freebsd']
    
    # Check if the current platform is in the list of UNIX platforms
    if any(platform in sys.platform for platform in unix_platforms):
        return "Unix"
    elif sys.platform.startswith('win'):
        return "Win"

def SetHighPriority(NICE_VAL):
    thisSystem = CheckSystem()
    if thisSystem == "Unix":
        print("You are using a UNIX-based system.")
        try:
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



def Normalize(dataMat):
    '''
    Normalize data to be between 0 and 65535 (range of unsigned 16 bit int)

    '''

    dataMat = np.array(dataMat,dtype=np.float64)      # convert data to unsigned 16 bit integers
    dataMat = dataMat - np.min(dataMat)
    dataMat = dataMat / np.max(dataMat)
    dataMat = dataMat * 65535
    dataMat = np.array(dataMat,dtype=np.uint16)      # convert data to unsigned 16 bit integers
    return dataMat


def IntegrityCheck(data, times, NUM_PACKS_DETECT, NUM_CHAN, SAMPS_PER_CHANNEL, MICRO_INCR):
    """
    Check the integrity of data size and time stamps to ensure they are evenly spaced by a specified microsecond increment.
    
    Returns:
        int: 1 if the data is of correct length and time stamps are evenly spaced by MICRO_INCR microseconds, 0 otherwise.

    """

    if len(data) != NUM_PACKS_DETECT * NUM_CHAN * SAMPS_PER_CHANNEL:
        return 0
    for i in range(len(times) - 1):
        if (times[i+1] - times[i]).microseconds != MICRO_INCR:
            print("Error: time stamps not evenly spaced by "+ str(MICRO_INCR) + " microseconds")
            for time in times:
                print(time)
            return 0
    return 1

def Sleep(duration, getNow=time.perf_counter):
    """
    Custom sleep function. Checks the current time against the disired wake up time.

    Args:
        duration (float): How long the program should sleep
        getNow (float):  The current time
    """
    now = getNow()
    end = now + duration
    while now < end:
        now = getNow()
        
def SyntheticClickGenerator(signalLength, clickDuration):
    """
    Function to generate synthetic click/impulse data

    Args:
        signalLength (string):   The path to the data to read in
        clickDuration (integer): A scaling parameter to shift the data by

    Returns:
        float: The result of the division.
    """
        
    mu, sigma = 0, 10                                             # mean and standard deviation of normal distribution 
    s = np.abs(np.random.normal(mu, sigma, signalLength))
    #randNum = np.random.rand(1)
    startPosition = int(np.random.rand(1)*signalLength)
    
    s[startPosition:startPosition + clickDuration] = (s[startPosition:startPosition + clickDuration] * 1.7) * (np.hamming(clickDuration) + 1)
    return s

def LoadChannelOne(DATA_PATH, DATA_SCALE, filePath):
    print("Loading data from file: ",filePath)
    dataMatrix = loadmat(filePath)['DATA'].T
    print("Shape of loaded data: ", dataMatrix.shape)
    return dataMatrix[0,:]

def Load4ChannelDataset(filePath):
    print("Loading data from file: ",filePath)
    dataMatrix = loadmat(filePath)['DATA']
    print("Shape of loaded data: ", dataMatrix.shape)
    assert dataMatrix.shape[1] == 4
    return dataMatrix.T

def DuplicateAndShiftChannels(dataMatrix, offset, NUM_CHAN):
    chanList = [2,3,4]
    if offset < 0:
        dataMatrix = dataMatrix[::-1]   # flip the rows in the matrix so that the largest offset is applied to channel 1
        chanList = [3,2,1]


    print("Shifting channels with offset: ", offset)
    for chan in range(1,NUM_CHAN):
        shift = int(abs(offset * chan)) # increase offset between channels linearly (assuming plane wave)
        print("Channel ", chanList[chan-1], " is shifted by: ",shift)
        dataMatrix[chan, :] = np.roll(dataMatrix[0, :], shift)                 # set all channels to be the same as the first channel
        dataMatrix[chan, :shift] = dataMatrix[0, 0]
    if offset < 0:
        dataMatrix = dataMatrix[::-1] # reorient the matrix
    
    print("first 10 values of channel 1 before scaling: ", dataMatrix[0, :10])
    print("first 10 values of channel 2 before scaling: ", dataMatrix[1, :10])
    print("first 10 values of channel 3 before scaling: ", dataMatrix[2, :10])
    print("first 10 values of channel 4 before scaling: ", dataMatrix[3, :10])
    return dataMatrix


def InterleaveData(dataMatrix):
    dataFlattened = dataMatrix.reshape(-1, order='F')   # Interleave the values of the rows uniformly
    #dataFlattened = dataMatrix.reshape(-1, order='F')   # Interleave the values of the rows uniformly
    return dataFlattened#, np.where(dataFlattened > 200)[0][0] # return the flattened matrix as well as index of first high amplitude value


#def StackData(dataMatrix, NUM_CHAN, SAMPS_PER_CHANNEL):
    # DEPRICATED
    # dataMatrix = dataMatrix.reshape(NUM_CHAN,divisor,SAMPS_PER_CHANNEL)     # divide each each into 'divisor' segments of length SAMPS_PER_CHANNEL
    #return np.hstack(np.hstack(dataMatrix))


def ScaleData(dataFlattened, scale, toStretch):
    nSample = 30
    print("first ",nSample ," values of dataMatrixInterleaved before scaling: ",dataFlattened[:nSample])
    dataFlattened = dataFlattened + scale                         
    
    if toStretch:
        dataFlattened = Normalize(dataFlattened)
        assert (dataFlattened.min() == 0) and (dataFlattened.max() == 65535)
    
    print("dataFlattened min and max values: ", np.min(dataFlattened), np.max(dataFlattened))
    return dataFlattened


def ConvertToBytes(dataFlattenedScaled):
    formatString = '>{}H'.format(len(dataFlattenedScaled))          # encode data as big-endian
    return struct.pack(formatString, *dataFlattenedScaled)


def ReadBinaryData(filename):
    """
    Reads binary data from a file and stores it into a list of byte lists.

    This function reads a binary file, identifies valid 32-byte records with the correct
    headers ('I' and 'M'), and saves each record as a list of bytes in a list of lists.

    Args:
        filename (str): Path to the binary file to be read.

    Returns:
        list[list[int]]: A list of 32-byte records, each represented as a list of integers.
    """
    byte_records = []

    try:
        with open(filename, "rb") as file:
            while True:
                # Read the first byte (X)
                X = file.read(1)
                if not X:
                    break  # EOF

                # Check for header 'I'
                if X == b'I':
                    # Read the second byte (Y)
                    Y = file.read(1)
                    if Y == b'M':
                        # Read the remaining 30 bytes
                        record = [ord(X), ord(Y)] + list(file.read(30))

                        # Ensure we read a full 30 bytes
                        if len(record) == 32:
                            byte_records.append(record)
    except FileNotFoundError:
        print("Error: File not found.")
    except IOError as e:
        print(f"Error reading file: {e}")

    return byte_records