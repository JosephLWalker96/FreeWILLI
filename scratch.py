import numpy as np
import struct
import time 


start_time = time.time()

array = np.array(np.arange(0,10000,1), dtype="uint16")

byte_stream = struct.pack('10000h',*array)

print(struct.calcsize("10000h"))

end_time = time.time()

print(end_time - start_time)


