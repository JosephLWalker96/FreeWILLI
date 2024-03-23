import time
import numpy as np
import psutil
import os
from scipy.io import loadmat

NICE_VAL = -15                        # set the "nice" value (priority). Ranges between [-20, 19] lower value 

pid = os.getpid()
process = psutil.Process(pid)         # Get the process object for the current process
process.nice(NICE_VAL)                # Set the process priority to high

trials = []
os.nice(NICE_VAL)

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

def LoadTest4chData1240(filePath = '../Data/joesdata.mat', scale = 2**15, chunkInterval = None):
    """
    Function to read in real 4 channel data and format the data according to firmware version 1240

    Args:
        path (string):   The path to the data to read in
        scale (integer): A scaling parameter to shift the data by

    Returns:
        array of integers: The result of the division.
    """
    numChannels = 4
    
    print("Loading data from file: ",filePath)
    dataMatrix = loadmat(filePath)['DATA'].T
    print("Shape of loaded data: ", dataMatrix.shape)

    divisor = dataMatrix.shape[1] // chunkInterval
    dataMatrix = dataMatrix[:,:int(chunkInterval*divisor)]
    dataMatrixReshaped = dataMatrix.reshape(numChannels,divisor,chunkInterval)
    dataMatrixReshaped = np.hstack(np.hstack(dataMatrixReshaped))
    print(dataMatrixReshaped[:chunkInterval*3])
    
    
    dataMatrixReshaped = dataMatrixReshaped + scale                         
    return dataMatrixReshaped
