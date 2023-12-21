import numpy as np
import time
import datetime
from datetime import timedelta

def process_segment(data, times, output_file):
    data_abs = data.astype('float64') **2
    data_abs = np.sqrt(data_abs)
    
    ### average data
    pulse_filter = np.ones(256) / 256
    filtered_signal = np.convolve(data_abs, pulse_filter, mode='valid')
    
    ### remove low amplitude values
    filtered_signal[filtered_signal < 80] = 0
    
    ### create a mask to segment click regions
    filt = np.ones(256)
    output = np.convolve(filtered_signal,filt)
    output[output > 0] = 1

    ### find index of click regions (click start: 0 -> 1, click end 1 -> 0) and split around these values
    diff = np.diff(output)
    output = np.where(diff != 0)[0]
    
    non_zero_mask_regions = np.split(filtered_signal, output)
    non_zero_regions = np.split(data, output)
    
    #output = np.insert(output, 0, 0)
    #print(output)
    if len(non_zero_regions) > 1:
        clicks = []
        output = np.insert(output, 0, 0)
        #print(output)
        for index, region in enumerate(non_zero_mask_regions):
            #print(region.any(),non_zero_regions[index].any())
            if region.any():
                #print('This should be true, ', non_zero_regions[index].any())
                seconds = (output[index] + np.argmax(non_zero_regions[index])) * 1e-5                 # calculate the fraction of a second that has passed
                click_time = times[0] + timedelta(microseconds=int(seconds * 1e6))  # convert seconds to microseconds
                peak_amp = np.max(non_zero_regions[index])
                clicks.append([click_time, peak_amp])
            #else:
            #    print('ALL zero')
        print(clicks)
        write_clicks(clicks, output_file)

def process_segment_1550(data, times, output_file):
    ch1 = datat[0::4] # get first channel data by getting every 4th element starting with the first
    process_segment(ch1, times, output_file)


def process_segment_1240(data, times, output_file):
    data = data.reshape(-1,4,124)  # Split the flattened array into original components
    data = np.hstack(data)
    ch1 = data[0]
    process_segment(ch1, times, output_file)

def write_clicks(clicks, output_file):
    # Open the file in write mode and write each row from 'clicks' to the file
    with open(output_file, 'a') as file:
        for row in clicks:
            file.write(f"{row[0]}, {row[1]}\n")
