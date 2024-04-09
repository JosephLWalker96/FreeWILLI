import numpy as np

def GenerateSinPulses(sampleRate, duration, offsets, pulseFrequency, numPulseCycles, otherFreqs, otherAmps, noiseScale):

    """
    parameters:
    
    sampleRate       - sampling frequency in Hz
    duration         - duration of the segment in seconds
    offsets          - spacing (samples) between signal1 and signalN (excluding signal1)
    pulseFrequency   - frequency of the pulse to generate
    numPulseCycles   - approximate number of pulseFrequency cycles to preserve. Filter out remaining with gaussian
    otherFreqs       - other frequencies to apply that are not offset and therefore can potentially 
                       confuse TDOA estimates. These frequencies must be filtered out
    otherAmps        - amplitudes of otherFreqs
    noiseScale       - scale of gaussian nouse to add
    
    returns:
    
    signal_1         - a sine wave signal that has been windowed by a gaussianWindow1
    signal_1         - a sine wave signal that has been windowed by a gaussianWindow2
    signal_1         - a sine wave signal that has been windowed by a gaussianWindow3
    signal_1         - a sine wave signal that has been windowed by a gaussianWindow4
    trueTDOAs        - matrix of true time difference between each pulse 
    """

    # Time array, of length duration spaced by sampleRate
    x1 = np.linspace(0, duration, int(duration * sampleRate), endpoint=False)
    
    # sinusoid to detect
    sineWave = np.sin(2 * np.pi * pulseFrequency * x1)

    # calculate time offsets for different pulses
    x2 = x1 - offsets[0]/sampleRate 
    x3 = x1 - offsets[1]/sampleRate
    x4 = x1 - offsets[2]/sampleRate    

    # create gaussian windows that are cenetered at different points in time
    segmentMidpoint = duration / 2
    sigma = numPulseCycles / pulseFrequency  # Standard deviation for the Gaussian window
    gaussianWindow1 = np.exp(-0.5 * ((x1 - segmentMidpoint) / sigma) ** 2)
    gaussianWindow2 = np.exp(-0.5 * ((x2 - segmentMidpoint) / sigma) ** 2)
    gaussianWindow3 = np.exp(-0.5 * ((x3 - segmentMidpoint) / sigma) ** 2)
    gaussianWindow4 = np.exp(-0.5 * ((x4 - segmentMidpoint) / sigma) ** 2)

    # mask the sinusoids with the different gaussian windows
    signal1 = sineWave * gaussianWindow1
    signal2 = sineWave * gaussianWindow2
    signal3 = sineWave * gaussianWindow3
    signal4 = sineWave * gaussianWindow4
    
    """
    plt.figure(figsize=(10,4))
    plt.plot(signal_1[startPoint:endPoint])
    plt.plot(gaussian_window1[startPoint:endPoint])
    plt.show()
    """

    """
    plt.plot(signal_1)
    plt.show()
    """
    
    # apply otherFreqs to signals
    for index in range(len(otherFreqs)):
        signal1 = signal1 + otherAmps[index]*np.sin(x1*otherFreqs[index])
        signal2 = signal2 + otherAmps[index]*np.sin(x1*otherFreqs[index])
        signal3 = signal3 + otherAmps[index]*np.sin(x1*otherFreqs[index])
        signal4 = signal4 + otherAmps[index]*np.sin(x1*otherFreqs[index])
        
    # add gaussian noise
    if noiseScale:
        signal1 = signal1 + np.random.normal(scale=noiseScale, size=len(signal1))
        signal2 = signal2 + np.random.normal(scale=noiseScale, size=len(signal1))
        signal3 = signal3 + np.random.normal(scale=noiseScale, size=len(signal1))
        signal4 = signal4 + np.random.normal(scale=noiseScale, size=len(signal1))


    # create true TDOA matrix
    trueTDOAs = np.array([[0,0,0,0],
                          [ - offsets[0],  0,  0,  0],
                          [- offsets[1], -(offsets[1] - offsets[0]),  0,  0],
                          [-offsets[2], - (offsets[2] - offsets[0]), -(offsets[2] - offsets[1]),  0]]) / sampleRate

    return signal1, signal2, signal3, signal4, trueTDOAs