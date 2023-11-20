import numpy as np
import struct
import time 
import sys

start_time = time.time()
m2 = bytearray(np.zeros((1, 96), dtype=int))
print(sys.getsizeof(m2))



yy = 23
mm = 11
dd = 13
HH = 7
MM = 10
SS = 1
us = (2,3,4,5)
usec = int.from_bytes(us,'big')
time1 = yy, mm, dd, HH, MM, SS, usec
print('time: ', *time1)

usec = int.from_bytes(us,'little')
time1 = yy, mm, dd, HH, MM, SS, usec
print('time: ', *time1)


print('while loop code:')

flag1 = 1;
for lll in range(5):
    if flag1 == 1:
        usec0 = 3
        flag1 = 2
    else:
        dusec = usec - usec0
        if dusec < 0:
            dusec = dusec + 1e6
    print(usec0)
