

#ifndef GLOBAL_VARS
#define GLOBAL_VARS

//#include <cstring>
//#include <cstdio>
//#include <string>

extern int HEAD_SIZE;                      //packet head size (bytes)
extern double MICRO_INCR;            // time between packets
extern int NUM_CHAN;                      //number of channels per packet
extern int SAMPS_PER_CHANNEL;            //samples per packet per channel, for 2 channels, this value is 5*62  = 310
extern int BYTES_PER_SAMP;                                             //bytes per sample

extern int DATA_SIZE;       //packet data size (bytes) = 1240
extern int PACKET_SIZE;                             //packet size (bytes) = 1252
extern int REQUIRED_BYTES;
extern int DATA_BYTES_PER_CHANNEL;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
extern int NUM_PACKS_DETECT;
extern const float TIME_WINDOW;// = 0.5;                                                    // fraction of a second to consider  
extern const std::string OUTPUT_FILE;// = "clicks_data.txt";
//extern int packet_counter = 0;
extern int DATA_SEGMENT_LENGTH;
#endif
