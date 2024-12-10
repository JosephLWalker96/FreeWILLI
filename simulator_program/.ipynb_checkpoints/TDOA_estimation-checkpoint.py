import numpy as np
import math
from scipy.signal import firwin, ellip, freqz


def EllipticFilter(order, ripple_db, cutoff_freq, sample_rate):
    normalized_cutoff_freq = cutoff_freq * 2 / sample_rate
    # Design the Elliptic filter
    b, a = ellip(order, ripple_db, 40, normalized_cutoff_freq, 'high')
    return b, a

def GenerateFIR_Filter(cutoff_freq_hz, num_taps, sampling_freq_hz):
    # Design the FIR filter
    nyquist_freq = sampling_freq_hz / 2.0  # Nyquist frequency is half the sampling frequency
    #normalized_cutoff_freq = cutoff_freq_hz / nyquist_freq
    taps = firwin(num_taps, cutoff_freq_hz, window='hamming', pass_zero='highpass', fs=sampling_freq_hz)

    # Apply the FIR filter
    return taps



def GCC_PHAT(channel_matrix, fs, NUM_CHAN, max_tau=None, interp=16):
    '''
    This function computes the offset between the signal sig and the reference signal refsig
    using the Generalized Cross Correlation - Phase Transform (GCC-PHAT)method.
    https://github.com/xiongyihui/tdoa/blob/master/gcc_phat.py
    '''
    
    numUniquePairings = math.comb(NUM_CHAN, 2)
    tau_array = np.zeros(numUniquePairings)
    tau_matrix = np.zeros((NUM_CHAN,NUM_CHAN))
    n = channel_matrix[0].shape[0]# + refsig.shape[0]
    #cc_matrix = np.zeros((4,4))
    
    pairing = 0
    for sig_ind in range(len(channel_matrix)-1):
        for ref_ind in range(sig_ind+1,len(channel_matrix)):
            sig = channel_matrix[sig_ind]
            refsig = channel_matrix[ref_ind]
            #print("sig len: ", len(sig))
            
            #return
            # make sure the length for the FFT is larger or equal than len(sig) + len(refsig)

            # Generalized Cross Correlation Phase Transform
            SIG = np.fft.rfft(sig, n=n)
            REFSIG = np.fft.rfft(refsig, n=n)
            R = SIG * np.conj(REFSIG)

            cc = np.fft.irfft(R / np.abs(R), n=(interp * n))

            max_shift = int(interp * n / 2)
            if max_tau:
                max_shift = np.minimum(int(interp * fs * max_tau), max_shift)

            cc = np.concatenate((cc[-max_shift:], cc[:max_shift+1]))

            # find max cross correlation index
            shift = np.argmax(np.abs(cc)) - max_shift

            tau = shift / float(interp * fs)
            
            tau_matrix[ref_ind,sig_ind] = tau
            tau_array[pairing] = tau
            pairing += 1
            #cc_matrix[ref_ind,sig_ind] = cc
    
    return tau_array #, cc_matrix

def CrossCorr(channel_matrix, fs, max_tau=None, interp=16):
    '''
    This function computes the offset between the signal sig and the reference signal refsig
    using the Cross Correlation method.
    '''
    
    tau_matrix = np.zeros((4,4))
    
    
    for sig_ind in range(len(channel_matrix)-1):
        for ref_ind in range(sig_ind+1,len(channel_matrix)):
            sig = np.abs(channel_matrix[sig_ind])
            refsig = np.abs(channel_matrix[ref_ind])
    
    
    
            # Cross-correlation of the two signals
            cross_corr = np.correlate(sig, refsig, mode='full')
            # Time axis for the cross-correlation
            time_axis = np.arange(-len(sig) + 1, len(refsig)) / fs
            # Find the index of the maximum value in the cross-correlation

            #plt.plot(np.abs(cross_corr))
            #plt.show()

            max_index = np.argmax(np.abs(cross_corr))
            #print(max_index)

            # Calculate the TDOA in seconds
            tdoa = time_axis[max_index]
            
            tau_matrix[ref_ind,sig_ind] = tdoa
            
    return tau_matrix

def DOA_EstimateVerticalArray(TDOAs,soundSpeed,chanSpacing):
    """
    Estimates the vertical direction of arrival (DOA) using time difference of arrivals (TDOAs) 
    between microphone channels in an array.

    Parameters:
    - TDOAs (numpy.ndarray): Array of time difference of arrivals (TDOAs) between microphone pairs.
    - soundSpeed (double): Speed of sound in the medium, in meters per second.
    - chanSpacing (numpy.ndarray): Array of vertical separation between pairwise microphone channels, in meters.

    Returns:
    - numpy.ndarray: Array containing the estimated vertical DOA angles in degrees.
    
    """

    dd = TDOAs * soundSpeed                  # calculate the distance differences 
    vals = dd / chanSpacing                  # normalize the distnace differences by the vertical separation 
    
    if ((np.abs(vals) > 1).any()):
        print("Out of bounds values: ")
        print(vals)
    
    vals = np.clip(vals, -1, 1)              # ensure that values are between -1 and 1 for arcsin

    return np.arcsin(vals) * 180 / np.pi     # convert angle to degrees 

