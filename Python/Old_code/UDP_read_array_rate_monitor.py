import struct
import socket
import time
# UDP settings
UDP_IP = "192.168.7.2"  # Replace with the IP address of the receiver
UDP_PORT = 1045  # Replace with the port number on the receiver

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the IP address and port
sock.bind((UDP_IP, UDP_PORT))

data_buffer = bytearray(2*1002*500*3)
while True:
    data, addr = sock.recvfrom(2*8 * 1002)  # Assuming each double is 8 bytes

    # Unpack the received bytes into a list of doubles
    double_list = struct.unpack('1002h', data)
    
    struct.pack_into('1002h', data_buffer, 0, *double_list)
    burst_ind = double_list[-3:]
    print(burst_ind) 
    #print(len(data_buffer))
    
    # Print the received list of doubles
    #print("Received doubles:", double_list)
    print(time.time())
# Close the socket
sock.close()
