import time
import numpy as np
import matplotlib.pyplot as plt
import psutil
import os

pid = os.getpid()

# Get the process object for the current process
process = psutil.Process(pid)

# Set the process priority to high
process.nice(-10)

trials = []
os.nice(-10)



def sleep(duration, get_now=time.perf_counter):
    now = get_now()
    end = now + duration
    while now < end:
        now = get_now()

for i in range(5000):
    start = time.time()
    sleep(.0014)
    trials.append(time.time()-start)
    
print(np.mean(trials))
print(np.max(trials))
print(np.min(trials))