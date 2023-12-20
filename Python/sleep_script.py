import time
import numpy as np
import matplotlib.pyplot as plt
import psutil
import os

NICE_VAL = -15
TRIAL_NUM = 50000
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

for i in range(TRIAL_NUM):
    start = time.time()
    sleep(.001250)
    trials.append(time.time()-start)
    
print(np.mean(trials))
print(np.max(trials))
print(np.min(trials))
