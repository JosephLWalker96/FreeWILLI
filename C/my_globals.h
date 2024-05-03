

#ifndef GLOBAL_VARS
#define GLOBAL_VARS

#include <string>
// Macros for preprocessor directives
//#define PRINT_DATA_PROCESSOR
#define PRINT_PROCESS_SEGMENT
#define PRINT_PROCESS_SEGMENT_1240
//#define PRINT_PROCESS_SEGMENT_1550
        
extern int HEAD_SIZE;                  //packet head size (bytes)
extern int NUM_CHAN;                   //number of channels per packet
extern int SAMPS_PER_CHANNEL;          //samples per packet per channel, for 2 channels, this value is 5*62  = 310
extern int BYTES_PER_SAMP;             //bytes per sample

extern int DATA_SIZE;                  //packet data size (bytes)
extern int PACKET_SIZE;                //packet size (bytes)
extern int REQUIRED_BYTES;
extern int DATA_BYTES_PER_CHANNEL;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
extern int NUM_PACKS_DETECT;

extern int MICRO_INCR;              // time between packets
extern int SAMPLE_RATE;

extern const double TIME_WINDOW;        // fraction of a second to consider  
extern const std::string OUTPUT_FILE;
extern int DATA_SEGMENT_LENGTH;
#endif
