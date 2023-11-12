import struct
import socket

# UDP settings
UDP_IP = "192.168.7.2"  # Replace with the IP address of the receiver
UDP_PORT = 1045  # Replace with the port number on the receiver

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the IP address and port
sock.bind((UDP_IP, UDP_PORT))
while True:
    data, addr = sock.recvfrom(8 * 5)  # Assuming each double is 8 bytes

    # Unpack the received bytes into a list of doubles
    double_list = struct.unpack('!5d', data)

    # Print the received list of doubles
    print("Received doubles:", double_list)

# Close the socket
sock.close()
