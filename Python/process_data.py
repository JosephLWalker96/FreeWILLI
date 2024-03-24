import numpy as np
import time
import datetime
from datetime import timedelta


def RestartListener():
    print("Function not defined")


def IntegrityCheck(data, times, MICRO_INCR):
    for i in range(len(times) - 1):
        if (times[i+1] - times[i]).microseconds != MICRO_INCR:
            print("Error: time stamps not evenly spaced by "+ str(MICRO_INCR) + " microseconds")
            for time in times:
                print(time)
            return 0
    return 1

def ProcessSegment(data, times, outputFile):
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
    
    #output = np.insert(output, 0, 0)
    #print(output)
    if len(nonZeroRegions) > 1:
        clicks = []
        output = np.insert(output, 0, 0)
        #print(output)
        for index, region in enumerate(nonZeroMaskRegions):
            #print(region.any(),nonZeroRegions[index].any())
            if region.any():
                #print('This should be true, ', nonZeroRegions[index].any())
                seconds = (output[index] + np.argmax(nonZeroRegions[index])) * 1e-5                 # calculate the fraction of a second that has passed
                clickTime = times[0] + timedelta(microseconds=int(seconds * 1e6))  # convert seconds to microseconds
                peakAmp = np.max(nonZeroRegions[index])
                clicks.append([clickTime, peakAmp])
            #else:
            #    print('ALL zero')
        print(clicks)
        WriteClicks(clicks, outputFile)


    ########## New code
    output = np.where(diff != 0)[0]
    output = np.insert(output, 0, 0)
    clicks = []
    for index in range(len(output)-1):
        startPoint = output[index]
        endPoint = output[index+1]
        regionMask = filteredSignal[startPoint:endPoint]
        if regionMask.any():
            regionData = data[startPoint:endPoint]
            seconds = (output[index] + np.argmax(regionData)) * 1e-5
            clickTime = times[0] + timedelta(microseconds=int(seconds * 1e6))  # convert seconds to microseconds
            peakAmp = np.max(regionData)
            clicks.append([clickTime, peakAmp])
    print(clicks)
    WriteClicks(clicks, 'INDEX_clicks.txt')

   
def ProcessSegment1550(data, times, outputFile):
    ch1 = data[0::4] # get first channel data by getting every 4th element starting with the first
    print("Start: ",ch1[:15])
    print("End: ",ch1[-4:])
    ProcessSegment(ch1, times, outputFile)


def ProcessSegment1240(data, times, outputFile):
    #ch1 = data[0::4] # This is incorrect!!!!
    data = data.reshape(-1,4,124)  # Split the flattened array into original components
    data = np.hstack(data)
    ch1 = data[0]
    #for i in range(10):
    #    print(ch1[i])
    print("Ch1 Start: ",ch1[:15])
    print("Ch1 End: ",ch1[-4:])
    ProcessSegment(ch1, times, outputFile)

def WriteClicks(clicks, outputFile):
    # Open the file in write mode and write each row from 'clicks' to the file
    with open(outputFile, 'a') as file:
        for row in clicks:
            file.write(f"{row[0]}, {row[1]}\n")
