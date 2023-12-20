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

def sleep(duration, get_now=time.perf_counter):
    """
    Custom sleep function. Checks the current time against the disired wake up time.

    Args:
        duration (float): How long the program should sleep
        get_now (float):  The current time
    """
    now = get_now()
    end = now + duration
    while now < end:
        now = get_now()
        
def synthetic_click_generator(signal_length, click_dur):
    """
    Function to generate synthetic click/impulse data

    Args:
        signal_length (string):   The path to the data to read in
        click_dur (integer): A scaling parameter to shift the data by

    Returns:
        float: The result of the division.
    """
        
    mu, sigma = 0, 10                                             # mean and standard deviation of normal distribution 
    s = np.abs(np.random.normal(mu, sigma, signal_length))
    rand_num = np.random.rand(1)
    start_pos = int(np.random.rand(1)*signal_length)
    
    s[start_pos:start_pos + click_dur] = (s[start_pos:start_pos + click_dur] * 1.7) * (np.hamming(click_dur) + 1)
    return s

def load_test_4ch_data_1550(file_path = '../Data/joesdata.mat', scale = 2**15):
    """
    Function to read in real 4 channel data and format the data according to firmware version 1550

    Args:
        path (string):   The path to the data to read in
        scale (integer): A scaling parameter to shift the data by

    Returns:
        array of integers: The result of the division.
    """
    
    print("Loading data from file: ",file_path)
    DATA = loadmat(file_path)['DATA'].T
    print("Shape of loaded data: ", DATA.shape)
    DATA_reshaped = DATA.reshape(-1, order='F')                   # Interleave the values of the rows uniformly
    print("REAL DATA: ",DATA_reshaped[:30])
    DATA_reshaped = DATA_reshaped + scale                         
    return DATA_reshaped

def load_test_4ch_data_1240(file_path = '../Data/joesdata.mat', scale = 2**15, chunk_interval = None):
    """
    Function to read in real 4 channel data and format the data according to firmware version 1240

    Args:
        path (string):   The path to the data to read in
        scale (integer): A scaling parameter to shift the data by

    Returns:
        array of integers: The result of the division.
    """
    Num_channels = 4
    
    print("Loading data from file: ",file_path)
    DATA = loadmat(file_path)['DATA'].T
    print("Shape of loaded data: ", DATA.shape)

    divisor = DATA.shape[1] // chunk_interval
    DATA = DATA[:,:int(chunk_interval*divisor)]
    DATA_reshaped = DATA.reshape(Num_channels,divisor,chunk_interval)
    DATA_reshaped = np.hstack(np.hstack(DATA_reshaped))
    print(DATA_reshaped[:chunk_interval*3])
    
    
    DATA_reshaped = DATA_reshaped + scale                         
    return DATA_reshaped