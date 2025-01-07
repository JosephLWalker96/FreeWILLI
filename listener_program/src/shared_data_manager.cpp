#include "shared_data_manager.h"

using namespace std::chrono_literals;
/**
 * @brief Push data into the shared buffer in a thread-safe manner.
 * @param data Reference to a vector of bytes representing the incoming data.
 * @return The size of the data buffer after pushing.
 */
int SharedDataManager::pushDataToBuffer(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(dataBufferLock);
    dataBuffer.push(data);
    return static_cast<int>(dataBuffer.size());
}

/**
 * @brief Waits for and retrieves data from the shared buffer, with periodic buffer flushing.
 *
 */
void SharedDataManager::waitForData(std::vector<std::vector<uint8_t>>& dataBytes, int numPacksToGet)
{
    while (true)
    {
        bool gotData = popDataFromBuffer(dataBytes, numPacksToGet);
        if (gotData)
        {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15ms));
    }
}

/**
 * @brief Pop data from the shared buffer in a thread-safe manner.
 * @param data Reference to a vector where the popped data will be placed.
 * @return True if data was successfully popped, false if the buffer was empty.
 */
bool SharedDataManager::popDataFromBuffer(std::vector<std::vector<uint8_t>>& data, int numPacksToGet)
{
    std::lock_guard<std::mutex> lock(dataBufferLock);
    if (dataBuffer.size() >= numPacksToGet)
    {
        for (int i = 0; i < numPacksToGet; i++)
        {
            data[i] = dataBuffer.front();
            dataBuffer.pop();
        }
        return true;
    }
    return false;
}
