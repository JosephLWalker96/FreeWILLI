import time
import numpy as np
import psutil
import os

NICE_VAL = -15                    # set the "nice" value (priority). Ranges between [-20, 19] lower value 

pid = os.getpid()
process = psutil.Process(pid)     # Get the process object for the current process
process.nice(NICE_VAL)                 # Set the process priority to high

trials = []
os.nice(NICE_VAL)

def sleep(duration, get_now=time.perf_counter):
    now = get_now()
    end = now + duration
    while now < end:
        now = get_now()
        
def synthetic_click_generator(signal_length, click_dur):
    mu, sigma = 0, 10 # mean and standard deviation
    s = np.abs(np.random.normal(mu, sigma, signal_length))
    rand_num = np.random.rand(1)
    start_pos = int(np.random.rand(1)*signal_length)
    
    s[start_pos:start_pos + click_dur] = (s[start_pos:start_pos + click_dur] * 1.7) * (np.hamming(click_dur) + 1)
    return s