#include "buffer_writer.h"

ObservationBuffer::ObservationBuffer()
    : mFlushInterval(std::chrono::seconds(30)),
      mBufferSizeThreshold(1000),
      mLastFlushTime(std::chrono::steady_clock::now()) {}

/**
 * @brief Writes the buffer data to the output file and resets the flush time.
 */
void ObservationBuffer::write(const std::string &outputFile)
{
    auto beforeFlush = std::chrono::steady_clock::now();
    appendBufferToFile(outputFile);
    auto afterFlush = std::chrono::steady_clock::now();
    std::chrono::duration<double> durationFlush = afterFlush - beforeFlush;

    mLastFlushTime = std::chrono::steady_clock::now();
}

/**
 * @brief Clears all data from the buffer.
 */
void ObservationBuffer::clearBuffer()
{
    mBuffer.mAmps.clear();
    mBuffer.mDoaX.clear();
    mBuffer.mDoaY.clear();
    mBuffer.mDoaZ.clear();
    mBuffer.mTdoaVector.clear();
    mBuffer.mXCorrAmps.clear();
    mBuffer.mPeakTimes.clear();
}

/**
 * @brief Appends a single data row to the buffer.
 */
void ObservationBuffer::appendToBuffer(const float peakAmp, const float doaX, const float doaY,
                                       const float doaZ, const Eigen::VectorXf &tdoaVector,
                                       const Eigen::VectorXf &xCorrAmps, const TimePoint &peakTime)
{
    mBuffer.mAmps.push_back(peakAmp);
    mBuffer.mDoaX.push_back(doaX);
    mBuffer.mDoaY.push_back(doaY);
    mBuffer.mDoaZ.push_back(doaZ);
    mBuffer.mTdoaVector.push_back(tdoaVector);
    mBuffer.mXCorrAmps.push_back(xCorrAmps);
    mBuffer.mPeakTimes.push_back(peakTime);
}

/**
 * @brief Appends buffer data to the specified file.
 */
void ObservationBuffer::appendBufferToFile(const std::string &outputFile)
{
    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::app);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Unable to open file for appending: " + outputFile);
    }

    size_t dataSize = mBuffer.mAmps.size();
    if (mBuffer.mDoaX.size() != dataSize || mBuffer.mDoaY.size() != dataSize ||
        mBuffer.mDoaZ.size() != dataSize || mBuffer.mTdoaVector.size() != dataSize ||
        mBuffer.mXCorrAmps.size() != dataSize || mBuffer.mPeakTimes.size() != dataSize)
    {
        throw std::runtime_error("Error: Mismatched buffer sizes in BufferStruct.");
    }

    int numChannelPairs = mBuffer.mTdoaVector[0].size();

    for (size_t i = 0; i < dataSize; ++i)
    {
        std::vector<std::string> rowData;

        auto timePoint = mBuffer.mPeakTimes[i];
        auto timeSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(timePoint.time_since_epoch());
        rowData.emplace_back(std::to_string(timeSinceEpoch.count()));

        rowData.push_back(std::to_string(mBuffer.mAmps[i]));
        rowData.push_back(std::to_string(mBuffer.mDoaX[i]));
        rowData.push_back(std::to_string(mBuffer.mDoaY[i]));
        rowData.push_back(std::to_string(mBuffer.mDoaZ[i]));

        Eigen::VectorXf tdoaVec = mBuffer.mTdoaVector[i];
        if (tdoaVec.size() != numChannelPairs)
        {
            throw std::runtime_error("Error: Inconsistent TDOA vector size at index " + std::to_string(i));
        }
        for (int j = 0; j < tdoaVec.size(); ++j)
        {
            rowData.push_back(std::to_string(tdoaVec[j]));
        }

        Eigen::VectorXf xcorrVec = mBuffer.mXCorrAmps[i];
        if (xcorrVec.size() != numChannelPairs)
        {
            throw std::runtime_error("Error: Inconsistent XCorr vector size at index " + std::to_string(i));
        }
        for (int j = 0; j < xcorrVec.size(); ++j)
        {
            rowData.push_back(std::to_string(xcorrVec[j]));
        }

        for (size_t k = 0; k < rowData.size(); ++k)
        {
            file << rowData[k];
            if (k < rowData.size() - 1)
                file << ",";
        }
        file << std::endl;
    }

    file.close();
}

/**
 * @brief Determines if the buffer should be flushed based on its size or time since the last flush.
 */
bool ObservationBuffer::timeToFlushBuffer(RuntimeConfig &runtimeConfig, const TimePoint &startLoop)
{
    int bufferSize = mBuffer.mPeakTimes.size();

    if (bufferSize == 0)
    {
        return false;
    }

    auto timeSinceLastFlush = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - mLastFlushTime);

    return (bufferSize >= mBufferSizeThreshold || mFlushInterval <= timeSinceLastFlush);
}