#pragma once

#include "pch.h"
#include "algorithms/IMU_processor.h"

class FirmwareConfig
{
public:
    // UDP packet information
    static constexpr int HEAD_SIZE = 12;          // packet head size (bytes)
    static constexpr int NUM_CHAN = 4;            // number of channels per packet
    static constexpr int SAMPS_PER_CHANNEL = 124; // samples per packet per channel
    static constexpr int BYTES_PER_SAMP = 2;      // bytes per sample
    static constexpr int MICRO_INCR = 1240;       // time between packets
    static constexpr int SAMPLE_RATE = 1e5;       // sample rate in Hz
    
    static constexpr float SAMPLE_OFFSET = 32768.0f; // Define a named constant for the sample offset

    static constexpr int DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP; // packet data size (bytes)
    static constexpr int REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    static constexpr int DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP; // number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels

    static constexpr float TIME_WINDOW = 0.01; // fraction of a second to consider when performing cross correlation
    static constexpr int NUM_PACKS_DETECT = static_cast<int>(TIME_WINDOW * SAMPLE_RATE / SAMPS_PER_CHANNEL);
    static constexpr int DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN;

    static constexpr int CHANNEL_SIZE = DATA_SEGMENT_LENGTH / NUM_CHAN;

    static constexpr int IMU_BYTE_SIZE = 0;

    static constexpr int PACKET_SIZE = DATA_SIZE + HEAD_SIZE + IMU_BYTE_SIZE; // the total number of bytes in a UDP packet

    std::unique_ptr<ImuProcessor> imuManager = nullptr;

    /*
    * counter: how many times we’ve already called this function 
    */
    void convertAndInsertData(Eigen::MatrixXf &channelMatrix, 
                        std::span<uint8_t> dataBytes, 
                        int &counter,
                        int headSize = HEAD_SIZE,
                        int dataSize = DATA_SIZE,
                        int bytesPerSamp = BYTES_PER_SAMP,
                        float sampleOffset = SAMPLE_OFFSET)
    {
        // Each sample = 2 bytes
        const size_t numSamples = dataSize / bytesPerSamp;
        const size_t numRows = channelMatrix.rows();

        // Calculate how many columns the new data will occupy
        const size_t newCols = (numSamples + numRows - 1) / numRows;

        // Pointer to the start of the Eigen matrix's data (column-major layout)
        float* __restrict__ matrixPtr = channelMatrix.data();

        // Pointer to incoming samples (skipping the header)
        const uint8_t* __restrict__ inPtr = dataBytes.data() + headSize;

        // Compute the offset where the new columns begin based on the counter
        const size_t startOffset = counter * newCols * numRows;

        // Single-pass conversion & storage
        for (size_t i = 0; i < numSamples; ++i)
        {
            // Convert 2-byte sample to float. Bytes are in big-endian order in the buffer (high byte first)
            uint16_t sample = (static_cast<uint16_t>(inPtr[bytesPerSamp * i]) << 8) | inPtr[bytesPerSamp * i + 1];
            float value = static_cast<float>(sample) - sampleOffset;

            matrixPtr[startOffset + i] = value;
        }
    }



    /**
     * @brief Generates a timestamp from raw data bytes.
     *
     * @param dataBytes A vector of bytes representing the timestamp data.
     * @param numChannels The number of data channels (not directly used in this function but passed for compatibility).
     * @return A `TimePoint` representing the timestamp.
     * @throws std::runtime_error if `std::mktime` fails to convert the timestamp.
     */
    TimePoint generateTimestamp(std::vector<uint8_t> &dataBytes, const int numChannels)
    {
        std::tm timeStruct{};
        timeStruct.tm_year = static_cast<int>(dataBytes[0]) + 2000 - 1900;
        timeStruct.tm_mon = static_cast<int>(dataBytes[1]) - 1;
        timeStruct.tm_mday = static_cast<int>(dataBytes[2]);
        timeStruct.tm_hour = static_cast<int>(dataBytes[3]);
        timeStruct.tm_min = static_cast<int>(dataBytes[4]);
        timeStruct.tm_sec = static_cast<int>(dataBytes[5]);

        int64_t microseconds = (static_cast<int64_t>(dataBytes[6]) << 24) +
                            (static_cast<int64_t>(dataBytes[7]) << 16) +
                            (static_cast<int64_t>(dataBytes[8]) << 8) +
                            static_cast<int64_t>(dataBytes[9]);

        std::time_t timeResult = std::mktime(&timeStruct);

        if (timeResult == std::time_t(-1))
        {
            throw std::runtime_error("Error: failure in mktime.");
        }

        auto currentTime = std::chrono::system_clock::from_time_t(timeResult);
        currentTime += std::chrono::microseconds(microseconds);

        return currentTime;
    }

    /**
     * @brief Checks for data errors in a session based on timing and packet size.
     *
     * @param dataBytes A vector of bytes representing the received data.
     * @param microIncrement The expected microsecond increment between timestamps.
     * @param isPreviousTimeSet A reference to a boolean indicating if the previous timestamp was set.
     * @param previousTime A reference to the previous timestamp for comparison.
     * @param currentTime A reference to the current timestamp for comparison.
     * @param packetSize The expected size of a data packet.
     * @throws std::runtime_error if the data packet size is incorrect or if there are timing errors.
     */
    void throwIfDataErrors(const std::vector<uint8_t> &dataBytes, const int microIncrement, bool isPreviousTimeSet,
                            const TimePoint &previousTime, const TimePoint &currentTime,const int packetSize)
    {
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count();

        if (isPreviousTimeSet && (elapsedTime != microIncrement))
        {
            std::stringstream errorMsg;
            errorMsg << "Error: Time not incremented by " << microIncrement << std::endl;
            throw std::runtime_error(errorMsg.str());
        }
        else if (dataBytes.size() != packetSize)
        {
            std::stringstream errorMsg;
            errorMsg << "Error: Incorrect number of bytes in packet. Expected: " << packetSize
                    << ", Received: " << dataBytes.size() << std::endl;
            throw std::runtime_error(errorMsg.str());
        }
    }
};
