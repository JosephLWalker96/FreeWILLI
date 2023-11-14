import struct              # library for converting data to binary
import socket              # library for establishing UDP communication 
import numpy as np
import time

UDP_IP = "192.168.7.2"     # IP address of the destination
UDP_PORT = 1045            # Port number of the destination

BIT_RATE = 7e6             # Desired Megabits per second e.g. 7e6 is 7 megabits per second
PACKETS_PER_BURST = 500    # send this many data packets as fast as possible
SAMPLES_PER_PACKET = 1000  # number of measurements in each data packet
BITS_PER_SAMPLE = 16       # bit depth

RATIO = (PACKETS_PER_BURST*SAMPLES_PER_PACKET * BITS_PER_SAMPLE) / BIT_RATE

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

for BURST_IND in range(100000):
    start_time = time.time()
    
    for PACK_IND in range(PACKETS_PER_BURST):
    
        # send a data packet consisting of SAMPLES_PER_PACKET shorts (16 bit)
        data_payload = np.array(np.arange(0,SAMPLES_PER_PACKET,1), dtype="uint16")
        tracking_info = np.array(np.array([BURST_IND,PACK_IND]), dtype="uint16")
        data_list = np.append(data_payload,tracking_info)

        # Convert the list of doubles to bytes.. add len(tracking_info) for tracking info
        data_bytes = struct.pack(str(SAMPLES_PER_PACKET+len(tracking_info)) + "h", *data_list)
        #print(data_bytes)

        # Send the data
        sock.sendto(data_bytes, (UDP_IP, UDP_PORT))
    
    finish_time = time.time()                             # completion time to send PACKETS_PER_BURST packets
    delay_time = (1 - (finish_time - start_time))*RATIO   # calculate required delay time to average BIT_RATE
    print('delay time: ',delay_time)
    time.sleep(delay_time)
    print((time.time() - start_time) / (RATIO) *BIT_RATE)
# Close the socket
sock.close()

