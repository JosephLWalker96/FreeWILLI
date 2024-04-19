import time
import numpy as np
import psutil
import os
from scipy.io import loadmat
import sys

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

def SetHighPriority():
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
        
def SyntheticClickGenerator(signalLength, clickDur):
    """
    Function to generate synthetic click/impulse data

    Args:
        signalLength (string):   The path to the data to read in
        clickDur (integer): A scaling parameter to shift the data by

    Returns:
        float: The result of the division.
    """
        
    mu, sigma = 0, 10                                             # mean and standard deviation of normal distribution 
    s = np.abs(np.random.normal(mu, sigma, signalLength))
    #randNum = np.random.rand(1)
    startPosition = int(np.random.rand(1)*signalLength)
    
    s[startPosition:startPosition + clickDur] = (s[startPosition:startPosition + clickDur] * 1.7) * (np.hamming(clickDur) + 1)
    return s

def LoadTest4chData1550(filePath = '../Data/joesdata.mat', scale = 2**15):
    """
    Function to read in real 4 channel data and format the data according to firmware version 1550

    Args:
        path (string):   The path to the data to read in
        scale (integer): A scaling parameter to shift the data by

    Returns:
        array of integers: The result of the division.
    """
    
    print("Loading data from file: ",filePath)
    dataMatrix = loadmat(filePath)['DATA'].T
    print("Shape of loaded data: ", dataMatrix.shape)
    dataMatrixReshaped = dataMatrix.reshape(-1, order='F')                   # Interleave the values of the rows uniformly
    print("REAL dataMatrix: ",dataMatrixReshaped[:30])
    dataMatrixReshaped = dataMatrixReshaped + scale                         
    return dataMatrixReshaped

def LoadTest4chData1240(filePath, scale,  NUM_CHAN, SAMPS_PER_CHANNEL, simulateTDOA):
    """
    Function to read in real 4 channel data and format the data according to firmware version 1240

    Args:
        filePath (string):   The path to the data to read in
        scale (integer): A scaling parameter to shift the data by

    Returns:
        array of integers: The result of the division.
    """
    
    print("Loading data from file: ",filePath)
    dataMatrix = loadmat(filePath)['DATA'].T
    print("Shape of loaded data: ", dataMatrix.shape)

    divisor = dataMatrix.shape[1] // SAMPS_PER_CHANNEL
    dataMatrix = dataMatrix[:,:int(SAMPS_PER_CHANNEL*divisor)]                      # truncate data that doesn't evenly fit into the packets
    if simulateTDOA:
        print("Simulating TDOA")
        for chan in range(1,NUM_CHAN):
            shift = 10*chan
            print(chan, shift, dataMatrix[0,0])
            dataMatrix[chan, :] = np.roll(dataMatrix[0, :], shift)                 # set all channels to be the same as the first channel
            dataMatrix[chan, :shift] = dataMatrix[0, 0]
        #dataMatrix = np.hstack(dataMatrix)
    #else:
    dataMatrix = dataMatrix.reshape(NUM_CHAN,divisor,SAMPS_PER_CHANNEL)     # divide each each into 'divisor' segments of length SAMPS_PER_CHANNEL
    dataMatrix = np.hstack(np.hstack(dataMatrix))
    
    print(dataMatrix[:SAMPS_PER_CHANNEL*3])
    
    dataMatrix = dataMatrix + scale                         
    return dataMatrix

