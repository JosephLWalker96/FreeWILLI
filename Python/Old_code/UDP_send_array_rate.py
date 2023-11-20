import struct
import socket
import numpy as np
import time

UDP_IP = "192.168.7.2"  # Replace with the IP address of the destination
UDP_PORT = 1045  # Replace with the port number of the destination

data_rate = 6e6 # 6 Megabits per second

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while(True):
    start_time = time.time()

    # send a data packet consisting of 100 shorts (16 bit)
    data_list = np.array(np.arange(0,1000,1), dtype="uint16")
    
    frac = (1000 * 2 * 8) / data_rate # calculate the amount of bits in this packet compared to the total
    # Convert the list of doubles to bytes
    #pack_format = f'!{payload_size // 8}h'
    data_bytes = struct.pack('1000h', *data_list)
    #print(data_bytes)
    

    # Send the data
    sock.sendto(data_bytes, (UDP_IP, UDP_PORT))
    
    finish_time = time.time()
    delay_time = (1 - (finish_time - start_time))*frac
    ##delay_time = (PACKET_SIZE * 2 * 8) / DATA_RATE - send_time
    #print(delay_time)
    time.sleep(delay_time)
    #print(frac)    
    print((time.time() - start_time) / frac *data_rate)
# Close the socket
sock.close()

