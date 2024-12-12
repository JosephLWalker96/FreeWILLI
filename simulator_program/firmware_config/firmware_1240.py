SAMPLE_RATE = 100000                 # sample rate (Hz)
HEAD_SIZE = 12                     # packet head size (bytes)
MICRO_INCR = 1240               # time between packets
NUM_CHAN = 4;                      # number of channels per packet
SAMPS_PER_CHANNEL = 124;           # samples per packet per channel, for 2 channels, this value is 5*62  = 310
BYTES_PER_SAMP = 2;                                             # bytes per sample
DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP;   # packet data size (bytes) = 1240
PACKET_SIZE = HEAD_SIZE + DATA_SIZE;                            # packet size (bytes) = 1252

REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE
DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP       # number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
