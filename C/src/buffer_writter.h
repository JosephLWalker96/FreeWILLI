
#pragma once

#include "pch.h"
#include "utils.h"

//void WriteArray(const std::span<Eigen::VectorXf> array, const std::span<TimePoint> timestamps, const std::string &filename);

using TimePoint = std::chrono::system_clock::time_point;

struct BufferStruct{
    std::vector<float> amps;
    std::vector<float> DOA_x;
    std::vector<float> DOA_y;
    std::vector<float> DOA_z;
    std::vector<Eigen::VectorXf> tdoaVector;
    std::vector<Eigen::VectorXf> XCorrAmps;
    std::vector<TimePoint> peakTimes;
};
void AppendBufferToFile(const std::string& outputFile, const BufferStruct& buffer);

class ObservationBuffer
{
private:
    void ClearBuffer()
    {
        Buffer.amps.clear();
        Buffer.DOA_x.clear();
        Buffer.DOA_y.clear();
        Buffer.DOA_z.clear();
        Buffer.tdoaVector.clear();
        Buffer.XCorrAmps.clear();
        Buffer.peakTimes.clear();
    }

public:
    std::chrono::milliseconds                          _flushInterval;
    size_t                                             _bufferSizeThreshold; // Adjust as needed
    std::chrono::time_point<std::chrono::steady_clock> _lastFlushTime;
    //std::vector<Eigen::VectorXf>                       Buffer;
    BufferStruct                       Buffer;

    void write(const std::string& outputFile)
    {

        auto beforeW3 = std::chrono::steady_clock::now();
        //WriteArray(Buffer, Buffer.peakTimes, outputFile);
        AppendBufferToFile(outputFile, Buffer);
        auto afterW3 = std::chrono::steady_clock::now();
        std::chrono::duration<double> durationW3 = afterW3 - beforeW3;

        _lastFlushTime = std::chrono::steady_clock::now();
        ClearBuffer();
    }
};