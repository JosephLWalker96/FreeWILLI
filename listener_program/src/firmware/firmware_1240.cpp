#include "firmware_1240.h"

/**
 * @brief Inserts data into a channel matrix by decoding raw byte data.
 *
 * This function extracts 16-bit sample values from raw byte arrays, converts them to floating-point format,
 * and stores them in an Eigen matrix. It also adjusts the samples based on a specified offset. Additionally,
 * if an IMU manager is available, it updates the IMU rotation matrix using the input data.
 *
 * @param channelMatrix Reference to an Eigen::MatrixXf where the extracted samples will be stored.
 * @param dataBytes A vector of byte arrays, each containing raw data including a header.
 *
 */
void Firmware1240::insertDataIntoChannelMatrix(
    Eigen::MatrixXf& channelMatrix, const std::vector<std::vector<uint8_t>>& dataBytes) const
{
    for (int i = 0; i < dataBytes.size(); i++)
    {
        float* __restrict__ matrixPtr = channelMatrix.data();
        const uint8_t* __restrict__ inPtr = dataBytes[i].data() + HEAD_SIZE;
        const size_t startOffset = i * SAMPS_PER_CHANNEL * NUM_CHAN;

        for (size_t j = 0; j < SAMPS_PER_PACKET; ++j)
        {
            uint16_t sample = (static_cast<uint16_t>(inPtr[BYTES_PER_SAMP * j]) << 8) | inPtr[BYTES_PER_SAMP * j + 1];
            matrixPtr[startOffset + j] = static_cast<float>(sample) - SAMPLE_OFFSET;
        }

        if (getImuManager())
        {
            getImuManager()->processIMUData(dataBytes[i]);
        }
    }
}

/**
 * @brief Extracts timestamps from raw data bytes and converts them into a vector of `TimePoint` objects.
 *
 * This function parses timestamp information from the given raw data packets. It interprets the date and
 * time components (year, month, day, hour, minute, second) and constructs `std::chrono::system_clock::time_point`
 * values, including microsecond-level precision. The parsed timestamps are returned as a vector.
 *
 * @param dataBytes A vector of byte arrays, each containing timestamp information in the first 10 bytes.
 *
 * @return A vector of `TimePoint` objects representing the extracted timestamps.
 */
std::vector<TimePoint> Firmware1240::generateTimestamp(std::vector<std::vector<uint8_t>>& dataBytes) const
{
    const size_t dataSize = dataBytes.size();
    std::vector<std::chrono::system_clock::time_point> outputTimes(dataSize);

    for (size_t i = 0; i < dataSize; ++i)
    {
        int year = static_cast<int>(dataBytes[i][0]) + 2000 - 1900;
        int month = static_cast<int>(dataBytes[i][1]) - 1;
        int day = static_cast<int>(dataBytes[i][2]);
        int hour = static_cast<int>(dataBytes[i][3]);
        int min = static_cast<int>(dataBytes[i][4]);
        int sec = static_cast<int>(dataBytes[i][5]);

        int64_t microseconds = (static_cast<int64_t>(dataBytes[i][6]) << 24) |
                               (static_cast<int64_t>(dataBytes[i][7]) << 16) |
                               (static_cast<int64_t>(dataBytes[i][8]) << 8) | (static_cast<int64_t>(dataBytes[i][9]));

        std::tm timeStruct{};
        timeStruct.tm_year = year;
        timeStruct.tm_mon = month;
        timeStruct.tm_mday = day;
        timeStruct.tm_hour = hour;
        timeStruct.tm_min = min;
        timeStruct.tm_sec = sec;

        std::time_t timeResult = std::mktime(&timeStruct);
        auto currentTime = std::chrono::system_clock::from_time_t(timeResult) + std::chrono::microseconds(microseconds);

        outputTimes[i] = currentTime;
    }

    return outputTimes;
}

/**
 * @brief Validates timestamp consistency and packet size in a session.
 *
 * This function checks for data integrity issues by ensuring that:
 *  - Timestamps in `dataVector` increment correctly by `microIncrement`.
 *  - Each packet in `dataBytes` has the expected `packetSize`.
 *
 * If an inconsistency is found, the function throws a `std::runtime_error`.
 *
 * @param dataBytes A vector of byte arrays, each representing a received data packet.
 * @param isPreviousTimeSet A flag indicating whether `previousTime` has been set. It is updated within the function.
 * @param previousTime The last valid timestamp. It is updated to the latest processed timestamp.
 * @param dataVector A vector of `TimePoint` timestamps associated with each data packet.
 *
 * @throws std::runtime_error If timestamps are not incrementing as expected or if a packet has an incorrect size.
 */
void Firmware1240::throwIfDataErrors(
    const std::vector<std::vector<uint8_t>>& dataBytes, bool& isPreviousTimeSet, TimePoint& previousTime,
    const std::vector<TimePoint>& dataVector) const
{
    for (int i = 0; i < dataVector.size(); i++)
    {
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(dataVector[i] - previousTime).count();

        if (isPreviousTimeSet && (elapsedTime != MICRO_INCR))
        {
            std::stringstream errorMsg;
            errorMsg << "Error: Time not incremented by " << MICRO_INCR << std::endl;
            throw std::runtime_error(errorMsg.str());
        }
        else if (dataBytes[i].size() != packetSize())
        {
            std::stringstream errorMsg;
            errorMsg << "Error: Incorrect number of bytes in packet. Expected: " << packetSize()
                     << ", Received: " << dataBytes.size() << std::endl;
            throw std::runtime_error(errorMsg.str());
        }
        previousTime = dataVector[i];
        isPreviousTimeSet = true;
    }
}