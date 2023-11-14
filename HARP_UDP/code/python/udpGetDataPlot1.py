# udpGetDataPlot1.py => start with udpGetTimes2.py and udpGetDataPlot1.m
#
# testing program for getting data from 4 ch HARP 3B04 230307
# two channels at 200kHz/ch
# UDP 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
# 1 datagram = 1 packet
#
# 230921 smw

import socket
import numpy as np
import matplotlib.pyplot as plt
import array as arr
import struct

hsz = 12;           # packet head size (bytes)
nchpp = 2;          # number of channels per packet
sppch = 5*62;       # samples per packet per channel = 310
bps = 2;            # bytes per sample
dsz = sppch * nchpp * bps;         # packet data size (bytes) = 1240
psz = hsz + dsz;    # packet size (bytes) = 1252

blkinterval = 1550; # block/packet/datagram size microseconds = 1e6 * sppch/200e3

#  uses IPV4 and communicates in byte mode. 
# address or host name "192.168.100.220" and port 50000.
UDP_IP = "192.168.100.220"
UDP_PORT = 50000

# need 100 bytes to get Open command through


m1 = b'Open'
m2 = bytearray(np.zeros((1,96),dtype=int))
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

sock.sendto(m1, (UDP_IP, UDP_PORT))
sock.sendto(m2, (UDP_IP, UDP_PORT))

print('UDP from HARP, get data and plot\n')
pcnt = 0;
lcnt = 0;
flag1 = 1;


# plot
fig, ax = plt.subplots()

    # 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data

#while 1:
dataB, addr1 = sock.recvfrom(psz)  # bytes object
dataI = struct.unpack('>' + 'B'*len(dataB),dataB) # convert bytes to unsigned char list
lenJ = int(len(dataB) / 2)
dataJ = struct.unpack('>' + 'H'*lenJ,dataB) # convert bytes to short integer list

pcnt = pcnt + 1
yy = dataI[0]
mm = dataI[1]
dd = dataI[2]
HH = dataI[3]
MM = dataI[4]
SS = dataI[5]
us = (dataI[6],dataI[7],dataI[8],dataI[9])
usec = int.from_bytes(us,'big')
time1 = yy, mm, dd, HH, MM, SS, usec
print("time: ", *time1)

ch1 = np.array(dataJ[6:lenJ-5:4]) - 2**15  # shift for two complement

# plot
y = ch1
ylen = len(ch1)
x = range(ylen)
ax.plot(x, y, linewidth=2.0)
plt.show()

# close the connection
sock.close
