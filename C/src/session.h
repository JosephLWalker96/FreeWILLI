
#pragma once
#include "pch.h"

using namespace std::chrono_literals;

//(focuses on managing shared resources and thread safety):
class Session {
public:
    std::atomic<bool> errorOccurred = false;
    std::queue<std::vector<uint8_t>> dataBuffer;
    std::vector<std::vector<uint8_t>> dataBytesSaved;
    std::vector<float> dataSegment;
    std::vector<std::chrono::system_clock::time_point> dataTimes; // timestamps of UDP packet
    std::mutex dataBufferLock;
    int detectionCounter = 0;

    // Add methods for buffer management
    int pushDataToBuffer(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(dataBufferLock);
        dataBuffer.push(data);
        return dataBuffer.size();
    }
    
    bool popDataFromBuffer(std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(dataBufferLock);
        if (!dataBuffer.empty()) {
            data = dataBuffer.front();
            dataBuffer.pop();
            return true;
        }
        return false;
    }
};