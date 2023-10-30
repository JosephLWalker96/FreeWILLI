import struct
import socket
s= socket.socket()
host = socket.gethostname()
port = 1045
s.bind(('',port))

s.listen(1)
while True:
    c , addr = s.accept()
    print("got connection from ", addr)
    c.send("Thank you for connecting!")

    
    while True:
        chunk = c.recv(8)  # Receive up to 1024 bytes of data
        if not chunk:
            break  # No more data to receive, break the loop
        decoded_double = struct.unpack('d', chunk)[0]
        print("Received data:",  decoded_double)#the byte string to a string and print it

    c.close()
