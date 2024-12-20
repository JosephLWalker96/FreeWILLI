#include "shared_data_manager.h"

/**
 * @brief Push data into the shared buffer in a thread-safe manner.
 * @param data Reference to a vector of bytes representing the incoming data.
 * @return The size of the data buffer after pushing.
 */
int SharedDataManager::pushDataToBuffer(const std::vector<uint8_t> &data)
{
    std::lock_guard<std::mutex> lock(dataBufferLock);
    dataBuffer.push(data);
    return static_cast<int>(dataBuffer.size());
}

/**
 * @brief Pop data from the shared buffer in a thread-safe manner.
 * @param data Reference to a vector where the popped data will be placed.
 * @return True if data was successfully popped, false if the buffer was empty.
 */
bool SharedDataManager::popDataFromBuffer(std::vector<std::vector<uint8_t>> &data, int numPacksToGet)
{
    std::lock_guard<std::mutex> lock(dataBufferLock);
    int i = 0;
    if (dataBuffer.size() >= numPacksToGet)
    {
        while(i < numPacksToGet){
            data[i] = dataBuffer.front();
            dataBuffer.pop();
            i++;
        }
        return true;
    }
    return false;
}