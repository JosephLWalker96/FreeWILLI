#ifndef GLOBAL_VARS
#define GLOBAL_VARS

// Macros for preprocessor directives
#include <string>
#include <vector>
#include <queue>
#include <chrono>
#include <mutex>
#include <atomic>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
//#define PRINT_DATA_PROCESSOR
#define PRINT_PROCESS_SEGMENT
#define PRINT_PROCESS_SEGMENT_1240
//#define PRINT_PROCESS_SEGMENT_1550

using std::vector;
using std::string;

struct Session {
    std::queue<vector<uint8_t>> dataBuffer;
    vector<double> dataSegment;
    vector<std::chrono::system_clock::time_point> dataTimes;
    std::mutex dataBufferLock;                       // For thread-safe buffer access
    std::mutex dataSegmentLock;                       // For thread-safe buffer access
    std::mutex dataTimesLock;                       // For thread-safe buffer access
    std::mutex udpSocketLock;                       // For thread-safe buffer access
    int datagramSocket = socket(AF_INET, SOCK_DGRAM, 0); // udp socket
    int UDP_PORT;              // Listening port
    string UDP_IP;             // IP address of data logger or simulator
    std::atomic<bool> errorOccurred = false;
};
        
extern unsigned int HEAD_SIZE;                  //packet head size (bytes)
extern unsigned int NUM_CHAN;                   //number of channels per packet
extern unsigned int SAMPS_PER_CHANNEL;          //samples per packet per channel, for 2 channels, this value is 5*62  = 310
extern unsigned int BYTES_PER_SAMP;             //bytes per sample

extern unsigned int DATA_SIZE;                  //packet data size (bytes)
extern unsigned int PACKET_SIZE;                //packet size (bytes)
extern unsigned int REQUIRED_BYTES;
extern unsigned int DATA_BYTES_PER_CHANNEL;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
extern unsigned int NUM_PACKS_DETECT;
extern unsigned int DATA_SEGMENT_LENGTH;
extern unsigned int MICRO_INCR;              // time between packets

extern unsigned const int SAMPLE_RATE;
extern const double TIME_WINDOW;        // fraction of a second to consider  
extern const std::string OUTPUT_FILE;
#endif
