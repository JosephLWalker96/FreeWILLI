#include "firmware_1240.h"

#include "pch.h"

/**
 * @brief Inserts data into a channel matrix.
 */
void Firmware1240::insertDataIntoChannelMatrix(Eigen::MatrixXf& channelMatrix,
                                               const std::vector<std::vector<uint8_t>>& dataBytes, int headSize,
                                               int dataSize, int bytesPerSamp, float sampleOffset) const
{
    for (int i = 0; i < dataBytes.size(); i++)
    {
        float* __restrict__ matrixPtr = channelMatrix.data();
        const uint8_t* __restrict__ inPtr = dataBytes[i].data() + headSize;
        const size_t startOffset = i * SAMPS_PER_CHANNEL * NUM_CHAN;

        for (size_t j = 0; j < SAMPS_PER_PACKET; ++j)
        {
            uint16_t sample = (static_cast<uint16_t>(inPtr[bytesPerSamp * j]) << 8) | inPtr[bytesPerSamp * j + 1];
            matrixPtr[startOffset + j] = static_cast<float>(sample) - sampleOffset;
        }

        if (auto imu = getImuManager())
        {
            // Call a method on the IMU manager
            imu->setRotationMatrix(dataBytes[i]);
            // std::cout << imu->mRotationMatrix << std::endl;
            // Eigen::Quaternionf q(imu->mRotationMatrix);

            /* example correct conversion (matlab does the same result)
            Eigen::Matrix3d testMat;
            testMat << 1.0, 0.0, 0.0, 0.0, 1.73205 / 2.0, 0.5, 0.0, -0.5, 1.73205 / 2.0;
            Eigen::Quaterniond q(testMat);
            std::cout << "pipeline quat: " << q.conjugate() << std::endl;
            */
        }
    }
}

/**
 * @brief Generates a vector of timestamps from raw data bytes.
 */
std::vector<TimePoint> Firmware1240::generateTimestamp(std::vector<std::vector<uint8_t>>& dataBytes,
                                                       int numChannels) const
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
 * @brief Checks for data errors in a session.
 */
void Firmware1240::throwIfDataErrors(const std::vector<std::vector<uint8_t>>& dataBytes, const int microIncrement,
                                     bool& isPreviousTimeSet, TimePoint& previousTime,
                                     const std::vector<TimePoint>& dataVector, const int packetSize) const
{
    for (int i = 0; i < dataVector.size(); i++)
    {
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(dataVector[i] - previousTime).count();

        if (isPreviousTimeSet && (elapsedTime != microIncrement))
        {
            std::stringstream errorMsg;
            errorMsg << "Error: Time not incremented by " << microIncrement << std::endl;
            throw std::runtime_error(errorMsg.str());
        } else if (dataBytes[i].size() != packetSize)
        {
            std::stringstream errorMsg;
            errorMsg << "Error: Incorrect number of bytes in packet. Expected: " << packetSize
                     << ", Received: " << dataBytes.size() << std::endl;
            throw std::runtime_error(errorMsg.str());
        }
        previousTime = dataVector[i];
        isPreviousTimeSet = true;
    }
}