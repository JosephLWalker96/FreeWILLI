import struct
import socket
import numpy as np
import time
UDP_IP = "192.168.7.2"  # Replace with the IP address of the destination
UDP_PORT = 1045  # Replace with the port number of the destination

# Sample list of 5 doubles
for i in range(1000):
    double_list = [np.sin(1.23+i), np.sin(4.56+i), np.sin(7.89+i), np.sin(5.79+i), np.sin(10.66+i)]  # Add your 5 doubles
    # Convert the list of doubles to bytes
    data_bytes = struct.pack('!5d', *double_list)
    print(data_bytes)
    
    # Create a UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Send the data
    sock.sendto(data_bytes, (UDP_IP, UDP_PORT))

    time.sleep(1)
# Close the socket
sock.close()

