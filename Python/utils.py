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
        
def narrow_gaussian_vector(vector_length, gaussian_width):
    # Generate a vector with zeros
    vector = np.zeros(vector_length)
    
    # Calculate the mean position for the Gaussian
    mean_position = np.random.randint(0, vector_length)
    
    # Generate Gaussian kernel
    gaussian_kernel = np.exp(-0.5 * ((np.arange(vector_length) - mean_position) / gaussian_width) ** 2)
    
    # Place the Gaussian in the vector
    vector += gaussian_kernel
