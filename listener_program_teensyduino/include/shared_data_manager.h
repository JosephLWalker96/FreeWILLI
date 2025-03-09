
#pragma once
#include "pch.h"

/**
 * @class SharedDataManager
 * @brief A thread-safe class that manages resources shared across threads.
 */
class SharedDataManager
{
   private:
    std::mutex dataBufferLock;
    std::queue<std::vector<uint8_t>> dataBuffer;  ///< Holds incoming data packets (thread-safe via mutex).

    bool popDataFromBuffer(std::vector<std::vector<uint8_t>>& data, int numPacksToGet);

   public:
    std::atomic<bool> errorOccurred = false;  ///< Indicates an error has occurred in processing or I/O operations.
    std::atomic<int> detectionCounter = 0;  ///< Tracks the number of successful detections.

    int pushDataToBuffer(const std::vector<uint8_t>& data);

    void waitForData(std::vector<std::vector<uint8_t>>& dataBytes, int numPacksToGet);
};