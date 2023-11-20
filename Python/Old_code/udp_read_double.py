import struct
import socket

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ethernet_ip = "192.168.7.2"
port = 1045
udp_socket.bind((ethernet_ip, port))

while True:
    data, addr = udp_socket.recvfrom(8)  # Receive up to 400 bytes of data

    # Check the length of the data
#    if len(data) < 400:
#       print("Received partial data packet")
#       continue

    decoded_array = struct.unpack('d', data)[0]  # Decode the data as an array of 100 double-precision floating-point numbers

    print("Received data:", decoded_array)
