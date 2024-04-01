import numpy as np
import time
import datetime
from datetime import timedelta

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

def SegmentPulses(data, times, SAMPLE_RATE, saveSegment = False):
    dataAbs = data.astype('float64')**2
    dataAbs = np.sqrt(dataAbs)
    print("dataAbs Size ", len(dataAbs))
    
    ### average data
    pulseFilter = np.ones(256) / 256
    filteredSignal = np.convolve(dataAbs, pulseFilter, mode='valid')
    #filteredSignal = np.convolve(dataAbs, pulseFilter, mode='same')
    
    print("dataAbs ", dataAbs[:20])
    print("filter: ", pulseFilter[:10])
    print("filtered signal: ",len(filteredSignal), filteredSignal[:20])    
    print("end of filtered signal: ", filteredSignal[-4:])    
    ### remove low amplitude values
    
    filteredSignal[filteredSignal < 80] = 0
    
    ### create a mask to segment click regions
    filt = np.ones(256)
    #output = np.convolve(filteredSignal,filt, mode = 'same')
    output = np.convolve(filteredSignal,filt)
    print('###################### WHERE: ',np.where(output > 0)[0])
    output[output > 0] = 1
    
    ### find index of click regions (click start: 0 -> 1, click end 1 -> 0) and split around these values
    diff = np.diff(output)
    output = np.where(diff != 0)[0]
    
    nonZeroMaskRegions = np.split(filteredSignal, output)
    nonZeroRegions = np.split(data, output)
    print("OUTPUT: ", output)
   
    if len(nonZeroRegions) > 1:
        output = np.where(diff != 0)[0]
        output = np.insert(output, 0, 0)
        clickTimes = []
        clickAmplitudes = []
        clickStartPoints = []
        clickEndPoints = []
        for index in range(len(output)-1):
            startPoint = output[index]
            endPoint = output[index+1]
            regionMask = filteredSignal[startPoint:endPoint]
            if regionMask.any():
                regionData = data[startPoint:endPoint]
                seconds = (output[index] + np.argmax(regionData)) / SAMPLE_RATE
                clickTime = times[0] + timedelta(microseconds=int(seconds * 1e6))  # convert seconds to microseconds
                peakAmp = np.max(regionData)
                
                clickTimes.append(clickTime)
                clickAmplitudes.append(peakAmp)
                clickStartPoints.append(startPoint)
                clickEndPoints.append(endPoint)

                if saveSegment: # save segmented pulse to file
                    timestampString = clickTime.strftime("%Y-%m-%d %H:%M:%S.%f")
                    np.savez("Segmented_pulses/" + timestampString + ".npz", regionData=regionData, clickTime=timestampString)

        #WritePulses(clickTimes, clickAmplitudes, 'INDEX_clicks.txt')
        return clickTimes, clickAmplitudes, clickStartPoints, clickEndPoints
    else:
        return None
   
def PreprocessSegment1550(data, NUM_CHAN, SAMPS_PER_CHANNEL):#, times, outputFile):
    ch1 = data[0::NUM_CHAN] # get first channel data by getting every NUM_CHANth element starting with the first
    ch2 = data[1::NUM_CHAN]
    ch3 = data[2::NUM_CHAN]
    ch4 = data[3::NUM_CHAN]
    
    return ch1, ch2, ch3, ch4


def PreprocessSegment1240(data, NUM_PACKS_DETECT, NUM_CHAN, SAMPS_PER_CHANNEL):#, times, outputFile):
    
    data = data.reshape(NUM_PACKS_DETECT, NUM_CHAN, SAMPS_PER_CHANNEL)  # Split the flattened array into original components
    data = np.hstack(data)
    
    ch1 = data[0]
    ch2 = data[1]
    ch3 = data[2]
    ch4 = data[3]
    
    return ch1, ch2, ch3, ch4

def WritePulseAmplitudes(times, amplitudes, outputFile):
    """
    Write pulse peak time and amplitude to a file.

    Returns:
        None
    """
    # Open the file in write mode and write each row from 'clicks' to the file
    with open(outputFile, 'a') as file:
        for index in range(len(times)):
            file.write(f"{times[index]}, {amplitudes[index]}\n")
