#pragma once

#include "algorithms/imu_processing.h"
#include "pch.h"

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Base class for firmware configuration, providing constants and utility methods.
 */
class Firmware1240
{
   public:
    // UDP packet information
    static constexpr int HEAD_SIZE = 12;              // Packet head size (bytes)
    static constexpr int NUM_CHAN = 4;                // Number of channels per packet
    static constexpr int SAMPS_PER_CHANNEL = 124;     // Samples per packet per channel
    static constexpr int BYTES_PER_SAMP = 2;          // Bytes per sample
    static constexpr int MICRO_INCR = 1240;           // Time between packets
    static constexpr int SAMPLE_RATE = 1e5;           // Sample rate in Hz
    static constexpr float SAMPLE_OFFSET = 32768.0f;  // Define a named constant
                                                      // for the sample offset

    static constexpr int DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP;  // Packet data size (bytes)
    static constexpr int REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    static constexpr int DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP;  // Data bytes per channel

    static constexpr float TIME_WINDOW = 0.01;  // Fraction of a second for cross-correlation
    static constexpr int NUM_PACKS_DETECT = static_cast<int>(TIME_WINDOW * SAMPLE_RATE / SAMPS_PER_CHANNEL);
    static constexpr int DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN;
    static constexpr int CHANNEL_SIZE = DATA_SEGMENT_LENGTH / NUM_CHAN;

    virtual constexpr int imuByteSize() const { return 0; }

    constexpr int packetSize() const { return DATA_SIZE + HEAD_SIZE + imuByteSize(); }

    /**
     * @brief Inserts data into a channel matrix.
     */
    void insertDataIntoChannelMatrix(Eigen::MatrixXf& channelMatrix, std::span<uint8_t> dataBytes, int& counter,
                                     int headSize = HEAD_SIZE, int dataSize = DATA_SIZE,
                                     int bytesPerSamp = BYTES_PER_SAMP, float sampleOffset = SAMPLE_OFFSET);

    /**
     * @brief Generates a vector of timestamps from raw data bytes.
     */
    std::vector<TimePoint> generateTimestamp(std::vector<std::vector<uint8_t>>& dataBytes, int numChannels);

    /**
     * @brief Checks for data errors in a session.
     */
    void throwIfDataErrors(const std::vector<uint8_t>& dataBytes, const int microIncrement, bool isPreviousTimeSet,
                           const TimePoint& previousTime, const TimePoint& currentTime, const int packetSize);

    /**
     * @brief Virtual function for IMU manager, returns nullptr
     * by default
     */
    virtual ImuProcessor* getImuManager() const { return nullptr; }
};

class Firmware1240IMU : public Firmware1240
{
   public:
    Firmware1240IMU() : mImuByteSize(32), imuManager(std::make_unique<ImuProcessor>(mImuByteSize)) {}

    // Override getImuManager to return the actual IMU manager
    ImuProcessor* getImuManager() const override { return imuManager.get(); }

    constexpr int imuByteSize() const override { return mImuByteSize; }

   private:
    int mImuByteSize;
    std::unique_ptr<ImuProcessor> imuManager = nullptr;  // Pointer to the IMU manager
};

class FirmwareFactory
{
   public:
    static std::unique_ptr<Firmware1240> create(bool useImu)
    {
        if (useImu)
        {
            return std::make_unique<Firmware1240IMU>();
        } else
        {
            return std::make_unique<Firmware1240>();
        }
    }
};