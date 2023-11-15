# This code is Joe's version of udpGetTimes2.py
#
# For running Joe's Simulator:
#    python udpGetTimes2_liveplot.py --port 1045 --ip 192.168.7.2
#
# For running in HARP lab:
#    python udpGetTimes2_liveplot.py
#
# testing program for getting data from 4 ch HARP 3B04 230307
# two channels at 200kHz/ch
# UDP 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
# 1 datagram = 1 packet
#
# based on udpGetTimes1.m
#
# 230920 smw


import socket
import numpy as np
import struct
import matplotlib.pyplot as plt
import sys
import argparse


print('HERE *********',sys.version)

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--port', default=50000, type=int)
parser.add_argument('--ip', default="192.168.100.220",type=str)

# Parsing the arguments
args = parser.parse_args()


hsz = 12;           # packet head size (bytes)
nchpp = 2;          # number of channels per packet
sppch = 5*62;       # samples per packet per channel = 310
bps = 2;            # bytes per sample
dsz = sppch * nchpp * bps;         # packet data size (bytes) = 1240
psz = hsz + dsz;    # packet size (bytes) = 1252

blkinterval = 1550; # block/packet/datagram size microseconds = 1e6 * sppch/200e3

# Create a udpport object udpportObj that uses IPV4 and communicates in byte mode. The
# object is bound to the local host at "192.168.100.100" and the local port 50000 with
# port sharing disabled.
#udpportObj = udpport("LocalHost","192.168.100.100","LocalPort",50000);

UDP_IP = args.ip
UDP_PORT = args.port

print(UDP_IP)
print(UDP_PORT)


# need 100 bytes to get Open command through
# Write the data ['Open',zeros(1,96)] as a uint8 using the udpport object udpportObj to the device with
# address or host name "192.168.100.220" and port 50000.
#write(udpportObj,['Open',zeros(1,96)],"uint8","192.168.100.220",50000);

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
if UDP_IP == "192.168.100.220":
    m1 = b'Open'
    m2 = bytearray(np.zeros((1,96),dtype=int))
    
    sock.sendto(m1, (UDP_IP, UDP_PORT))
    sock.sendto(m2, (UDP_IP, UDP_PORT))
elif UDP_IP == "192.168.7.2":
    sock.bind((UDP_IP, UDP_PORT))
else:
    print("ERROR: Unknown IP address" )

print('UDP from HARP, check timestamps\n')
pcnt = 0;
lcnt = 0;
flag1 = 1;
k = 0
while(1):
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
    us = (dataB[6],dataB[7],dataB[8],dataB[9])
    usec = int.from_bytes(us,'big')                                  # get micro seconds from dataI (unsigned char ist)
    time1 = yy, mm, dd, HH, MM, SS, usec
    print(time1)
    #print(dataI[6],dataI[7],dataI[8],dataI[9])
    
    k = k+ 1
    if k == 9:
        break
    continue
    ch1 = np.array(dataJ[6:lenJ-5:4]) - 2**15  # shift for two complement
    print(ch1)

    if flag1 == 1:
        usec0 = usec
        flag1 = 2
        pcnt = pcnt + 1
        print("first time: ", *time1)
        continue
    else:
        dusec = usec - usec0
        if dusec < 0:
            dusec = dusec + 1e6
    if dusec != blkinterval:
        time2 = yy, mm, dd, HH, MM, SS, usec, int(dusec)
        print("Time Glitch:", *time2)
        if dusec == 0 | abs(usec) > 1e6 | mm == 0 | dd == 0:
            print("Error: Close socket and try again")
            sock.close
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.sendto(m1, (UDP_IP, UDP_PORT))
            sock.sendto(m2, (UDP_IP, UDP_PORT))
    usec0 = usec

    if pcnt >= 1000:
        pcnt = 0
        #print(".", end= "")
        print('.',end = "")
        lcnt = lcnt + 1
        if lcnt >= 50:
            lcnt = 0
            print("time: ", *time1)
    

# close the connection
#write(udpportObj,['Close',zeros(1,95)],"uint8","192.168.100.220",50000);
sock.close
