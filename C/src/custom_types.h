#pragma once

#include "pch.h"

using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;

class GCC_Value_Error : public std::runtime_error {
public:
  GCC_Value_Error(const std::string& message) : std::runtime_error(message) {}
};

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
    const int interp = 1;
    const float TIME_WINDOW = 0.01;                 // fraction of a second to consider  
    
    string detectionOutputFile = ""; // Define at runtime
    string tdoaOutputFile      = "";
    string doaOutputFile       = "";

    const string filterWeights = "filters/My_filter.txt";
    
    const float speedOfSound = 1500.0;
    float energyDetThresh = 2500.0; // energy detector threshold - 2500.0 is default 
    
    vector<float> chanSpacing = {1.0, 2.0, 3.0, 1.0, 2.0, 1.0};
    
    void(*ProcessFncPtr)(std::vector<float>&, Eigen::MatrixXf&, unsigned int)= nullptr;

    // Define FFTWF plans during runtime 
    vector<fftwf_plan> fftForChannels; // fftwf object that points to channel 1

    fftwf_plan inverseFFT = nullptr;
};

struct Session {
    int datagramSocket = socket(AF_INET, SOCK_DGRAM, 0);  // udp socket
    int UDP_PORT;                                         // Listening port
    std::atomic<bool> errorOccurred = false;              // set true if error occurs in thread
    
    std::queue<vector<uint8_t>> dataBuffer;
    vector<vector<uint8_t>> dataBytesSaved;
    vector<float> dataSegment;
    vector<std::chrono::system_clock::time_point> dataTimes;
    std::mutex dataBufferLock;                       // For thread-safe buffer access
    
    vector<float>           peakAmplitudeBuffer;
    vector<TimePoint>       peakTimesBuffer;
    vector<Eigen::VectorXf> resultMatrixBuffer;
    vector<Eigen::VectorXf> DOAsBuffer;
    
    string UDP_IP;             // IP address of data logger or simulator
};


struct DetectionResult {
    int minPeakIndex = -1;
    int maxPeakIndex = -1;
    std::chrono::system_clock::time_point peakTimes;
    float peakAmplitude;
};

