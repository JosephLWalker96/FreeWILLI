#pragma once

#include "../pch.h"
#include "imu_processor_interface.h"

/**
 * @brief Base class for firmware 1240 configuration, providing constants and utility methods.
 */
class Firmware1240
{
   public:
    // UDP packet information
    static constexpr int HEAD_SIZE = 12;  // Packet head size (bytes)
    static constexpr int NUM_CHAN = 4;  // Number of channels per packet
    static constexpr int SAMPS_PER_CHANNEL = 124;  // Samples per packet per channel
    static constexpr int BYTES_PER_SAMP = 2;  // Bytes per sample
    static constexpr int MICRO_INCR = 1240;  // Time between packets
    static constexpr int sampleRate() { return 1e5; }  // Sample rate in Hz - WASD changed to function to allow override
    static constexpr float SAMPLE_OFFSET = 32768.0f;  // Define a named constant
                                                      // for the sample offset

    static constexpr int SAMPS_PER_PACKET = SAMPS_PER_CHANNEL * NUM_CHAN;  // Samples per packet
    static constexpr int DATA_SIZE = SAMPS_PER_PACKET * BYTES_PER_SAMP;  // Packet data size (bytes)
    static constexpr int REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    static constexpr int DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP;  // Data bytes per channel

    static constexpr float TIME_WINDOW = 0.01;  // Fraction of a second for cross-correlation
    static constexpr int NUM_PACKS_DETECT = static_cast<int>(TIME_WINDOW * sampleRate() / SAMPS_PER_CHANNEL);
    static constexpr int DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN;
    static constexpr int CHANNEL_SIZE = DATA_SEGMENT_LENGTH / NUM_CHAN;

    virtual int imuByteSize() const { return 0; }

    constexpr int packetSize() const { return DATA_SIZE + HEAD_SIZE + imuByteSize(); }

    void insertDataIntoChannelMatrix(
        Eigen::MatrixXf& channelMatrix, const std::vector<std::vector<uint8_t>>& dataBytes, int headSize = HEAD_SIZE,
        int bytesPerSamp = BYTES_PER_SAMP, float sampleOffset = SAMPLE_OFFSET) const;

    auto generateTimestamp(std::vector<std::vector<uint8_t>>& dataBytes) const -> std::vector<TimePoint>;

    void throwIfDataErrors(
        const std::vector<std::vector<uint8_t>>& dataBytes, const int microIncrement, bool& isPreviousTimeSet,
        TimePoint& previousTime, const std::vector<TimePoint>& currentTime, const int packetSize) const;

    std::unique_ptr<IImuProcessor> imuManager = nullptr;
};
