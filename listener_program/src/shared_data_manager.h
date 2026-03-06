
#pragma once
#include "pch.h"

/**
 * @class SharedDataManager
 * @brief A thread-safe class that manages resources shared across threads.
 */
class SharedDataManager
{
   public:
    std::atomic<bool> errorOccurred = false;  ///< Indicates an error has occurred in processing or I/O operations.
    std::atomic<int> detectionCounter = 0;  ///< Tracks the number of successful detections.

    int pushDataToBuffer(const std::vector<uint8_t>& data);

    void waitForData(std::vector<std::vector<uint8_t>>& dataBytes, int numPacksToGet,
                     std::chrono::seconds timeout = std::chrono::seconds(10));

   private:
    std::mutex dataBufferLock;
    std::queue<std::vector<uint8_t>> mDataBuffer;  ///< Holds incoming data packets (thread-safe via mutex).

    bool popDataFromBuffer(std::vector<std::vector<uint8_t>>& data, int numPacksToGet);
    int mMaxBufferSize = 1000;
};