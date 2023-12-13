import numpy as np


def detect_click(ch1):
    samps = ch1.astype('float64') **2
    #print(np.amax(samps),np.amin(samps))
    samps = np.sqrt(samps)
    pulse_filter = np.ones(256) / 256
    filtered_signal = np.convolve(samps, pulse_filter, mode='valid')
    filtered_signal[filtered_signal < 80] = 0  
    filt = np.ones(256)
    output = np.convolve(filtered_signal,filt)
    output[output > 0] = 1
    
    diff = np.diff(output)
    output = np.where(diff != 0)[0]
    #print("output ",output)
    non_zero_regions = np.split(filtered_signal, output)
    #print(len(non_zero_regions))
    max_values_in_regions = [(index,np.argmax(region),np.max(region)) for index,region in enumerate(non_zero_regions) if region.any()]
    #print('MADE IT HEREE')
    #adsfasdf
    if len(non_zero_regions) > 1:
        print('Clicks detected!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!')
        print(max_values_in_regions) 
