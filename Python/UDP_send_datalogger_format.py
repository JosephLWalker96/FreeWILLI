#  This is a program for simulating the data logger
#  This program may require root privileges 
#
#  Run this program by:
#  sudo python UDP_send_datalogger_format.py




import struct              # library for converting data to binary
import socket              # library for establishing UDP communication 
import numpy as np
import time
import datetime
from datetime import timedelta

UDP_IP = "192.168.7.2"     # IP address of the destination
UDP_PORT = 1045            # Port number of the destination

BIT_RATE = 7e6             # Desired Megabits per second e.g. 7e6 is 7 megabits per second
PACKETS_PER_BURST = 500    # send this many data packets as fast as possible
SAMPLES_PER_PACKET = 1000  # number of measurements in each data packet
BITS_PER_SAMPLE = 16       # bit depth

RATIO = (PACKETS_PER_BURST*SAMPLES_PER_PACKET * BITS_PER_SAMPLE) / BIT_RATE



REQUIRED_BYTES = 1252

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


yy = 23
mm = 11
dd = 5
HH = 1
MM = 1
SS = 1

micro_incr = 1550
date_time = datetime.datetime(2000+yy, mm, dd, HH, MM, SS, tzinfo=datetime.timezone.utc)



for BURST_IND in range(100000):
    start_time = time.time()
    
    for PACK_IND in range(PACKETS_PER_BURST):
        
        year = int(date_time.year - 2000)
        month = int(date_time.month)
        day = int(date_time.day)
        hour = int(date_time.hour)
        minute = int(date_time.minute)
        second = int(date_time.second)
        microseconds = int(date_time.microsecond)
        date_time = date_time + timedelta(microseconds=micro_incr)
        
        time_list = [year,month,day,hour,minute,second,microseconds]
        
        print('parsed: ', time_list)
        
        
        
        time_pack = struct.pack("BBBBBB", *np.array([year,month,day,hour,minute,second]))
        microseconds_pack = struct.pack("I", microseconds)
        zero_pack = struct.pack("H", 0)
        time_header = time_pack + microseconds_pack + zero_pack
        
        packet = time_header + bytes([0] * int(REQUIRED_BYTES-12))
        
        print(len(packet))
        # Send the data
        sock.sendto(packet, (UDP_IP, UDP_PORT))
        time.sleep(0.01)
    
    finish_time = time.time()                             # completion time to send PACKETS_PER_BURST packets
    delay_time = (1 - (finish_time - start_time))*RATIO   # calculate required delay time to average BIT_RATE
    print('delay time: ',delay_time)
    time.sleep(delay_time)
    print((time.time() - start_time) / (RATIO) *BIT_RATE)
# Close the socket
sock.close()

