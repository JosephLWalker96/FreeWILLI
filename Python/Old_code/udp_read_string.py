import socket

UDP_IP = "192.168.7.2"
UDP_PORT =1045 

sock = socket.socket(socket.AF_INET,  # Internet
                     socket.SOCK_DGRAM)  # UDP
sock.bind((UDP_IP, UDP_PORT))

print("Waiting for messages...")

while True:
    data, addr = sock.recvfrom(1024)  # buffer size is 1024 bytes
    decoded_data = data.decode()
    print(f"Received message from {addr}: {decoded_data}")
