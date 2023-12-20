import numpy as np

def process_segment(segment):
    segment = segment.astype('float64') **2
    segment = np.sqrt(segment)
    pulse_filter = np.ones(256) / 256
    filtered_signal = np.convolve(segment, pulse_filter, mode='valid')
    filtered_signal[filtered_signal < 80] = 0
    filt = np.ones(256)
    output = np.convolve(filtered_signal,filt)
    output[output > 0] = 1

    diff = np.diff(output)
    output = np.where(diff != 0)[0]
    non_zero_regions = np.split(filtered_signal, output)
    max_values_in_regions = [[index,np.argmax(region),np.max(region)] for index,region in enumerate(non_zero_regions) if region.any()]
    if len(non_zero_regions) > 1:
        print('Clicks detected!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!')
        print(max_values_in_regions)
        write_clicks(max_values_in_regions)

def process_segment_1550(segment):
    ch1 = segment[0::4] # get first channel data by getting every 4th element starting with the first
    process_segment(ch1)


def process_segment_1240(segment):
    segment = segment.reshape(-1,4,124)  # Split the flattened array into original components
    segment = np.hstack(segment)
    ch1 = segment[0]
    process_segment(ch1)

def write_clicks(clicks):
    file_name = "clicks_data.txt"

    # Open the file in write mode and write each row from 'clicks' to the file
    with open(file_name, 'a') as file:
        for row in clicks:
            file.write(f"{row[0]}, {row[1]}\n")
