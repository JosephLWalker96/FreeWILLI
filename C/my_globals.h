

#ifndef GLOBAL_VARS
#define GLOBAL_VARS

//#include <cstring>
//#include <cstdio>
//#include <string>

int HEAD_SIZE;                      //packet head size (bytes)
double MICRO_INCR;            // time between packets
int NUM_CHAN;                      //number of channels per packet
int SAMPS_PER_CHANNEL;            //samples per packet per channel, for 2 channels, this value is 5*62  = 310
int BYTES_PER_SAMP;                                             //bytes per sample

int DATA_SIZE;       //packet data size (bytes) = 1240
int PACKET_SIZE;                             //packet size (bytes) = 1252
int REQUIRED_BYTES;
int DATA_BYTES_PER_CHANNEL;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
int NUM_PACKS_DETECT;
const float TIME_WINDOW = 0.5;                                                    // fraction of a second to consider  
const std::string OUTPUT_FILE = "clicks_data.txt";
int packet_counter = 0;
extern int DATA_SEGMENT_LENGTH;
#endif
