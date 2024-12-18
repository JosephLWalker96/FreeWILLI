#pragma once

#include "../runtime_config.h"
#include "../pch.h"

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief A struct to hold buffer data for detection and tracking.
 */
struct BufferStruct
{
    std::vector<float> mAmps;
    std::vector<float> mDoaX;
    std::vector<float> mDoaY;
    std::vector<float> mDoaZ;
    std::vector<Eigen::VectorXf> mTdoaVector;
    std::vector<Eigen::VectorXf> mXCorrAmps;
    std::vector<TimePoint> mPeakTimes;
};

/**
 * @brief A class to manage the observation buffer for detection and tracking.
 */
class ObservationBuffer
{
public:
    ObservationBuffer();

    void write(const std::string &outputFile);
    void clearBuffer();
    void appendToBuffer(const float peakAmp, const float doaX, const float doaY,
                        const float doaZ, const Eigen::VectorXf &tdoaVector,
                        const Eigen::VectorXf &xCorrAmps, const TimePoint &peakTime);
    void flushBufferIfNecessary(RuntimeConfig &runtimeConfig);

    void initializeOutputFile(std::string &outputFile, const int numChannels);
private:
    void appendBufferToFile(const std::string &outputFile);

std::vector<std::string> generateChannelComboLabels(const std::string &labelPrefix, int numChannels);

    std::chrono::milliseconds mFlushInterval;
    size_t mBufferSizeThreshold;
    std::chrono::time_point<std::chrono::steady_clock> mLastFlushTime;
    BufferStruct mBuffer;
};