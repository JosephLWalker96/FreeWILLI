#pragma once

#include "../pch.h"
#include "../tracker/tracker.h"

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
class OutputManager
{
   public:
    OutputManager(std::chrono::seconds programRuntime);

    void appendToBuffer(const float peakAmp, const float doaX, const float doaY, const float doaZ,
                        const Eigen::VectorXf& tdoaVector, const Eigen::VectorXf& xCorrAmps, const TimePoint& peakTime);
    void flushBufferIfNecessary();

    void writeDataToCerr(std::span<TimePoint> errorTimestamps,
                         const std::vector<std::vector<uint8_t>>& erroredDataBytes);

    void initializeOutputFile(const TimePoint& timestamp, const int numChannels);

    void saveSpectraForTraining(const std::string& filename, int label, const Eigen::VectorXcf& frequencyDomainData);

    void terminateProgramIfNecessary();

   private:
    void appendBufferToFile();
    void write();
    void clearBuffer();

    std::vector<std::string> generateChannelComboLabels(const std::string& labelPrefix, int numChannels);

    std::string mDetectionOutputFile;
    std::chrono::milliseconds mFlushInterval;
    size_t mBufferSizeThreshold;
    std::chrono::time_point<std::chrono::steady_clock> mLastFlushTime;
    BufferStruct mBuffer;

    std::chrono::seconds mProgramRuntime;
    TimePoint mProgramStartTime;
};