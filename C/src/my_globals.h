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
#include <armadillo>
//#define PRINT_DATA_PROCESSOR
#define PRINT_PROCESS_SEGMENT
#define PRINT_PROCESS_SEGMENT_1240
//#define PRINT_PROCESS_SEGMENT_1550

using std::vector;
using std::string;

struct Experiment {
    unsigned int HEAD_SIZE;                                 //packet head size (bytes)
    unsigned int NUM_CHAN;                                  //number of channels per packet
    unsigned int SAMPS_PER_CHANNEL;                         //samples per packet per channel, for 2 channels, this value is 5*62  = 310
    unsigned int BYTES_PER_SAMP;                            //bytes per sample
    unsigned int DATA_SIZE;                                 //packet data size (bytes)
    unsigned int PACKET_SIZE;                               //packet size (bytes)
    unsigned int REQUIRED_BYTES;
    unsigned int DATA_BYTES_PER_CHANNEL;                    //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
    unsigned int NUM_PACKS_DETECT;
    unsigned int DATA_SEGMENT_LENGTH;
    unsigned int MICRO_INCR;                                // time between packets
    const unsigned int SAMPLE_RATE = 1e5;
    const double TIME_WINDOW = 0.01;                 // fraction of a second to consider  
    const string OUTPUT_FILE = "clicks_data.txt";
    const string outputFile = "Ccode_clicks.txt"; // Change to your desired file name
    void(*ProcessFncPtr)(vector<double>&, arma::Col<double>&, arma::Col<double>&, arma::Col<double>&, arma::Col<double>&, unsigned int&) = nullptr;
    const string filterWeights = "filters/My_filter.txt";
    
    arma::Col<int> chanSpacing = {1,2,3,1,2,1};
    const double speedOfSound = 1500.0;
    const double energyDetThresh = 80.0; // energy detector threshold 
};

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

struct DetectionResult {
    std::vector<std::chrono::system_clock::time_point> peakTimes;
    std::vector<double> peakAmplitude;
    int minPeakIndex = -1;
    int maxPeakIndex = -1;
};

#endif
