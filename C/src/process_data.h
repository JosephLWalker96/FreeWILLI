/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

using TimePoint = std::chrono::system_clock::time_point;

struct DetectionResult
{
    int minPeakIndex = -1;
    int maxPeakIndex = -1;
    std::chrono::system_clock::time_point peakTimes;
    float peakAmplitude;
};

class Session;

void FrequencyDomainFIRFiltering(
    const Eigen::MatrixXf &channelData, // Zero-padded time-domain data
    const Eigen::VectorXcf &filterFreq, // Frequency domain filter (FIR taps in freq domain)
    fftwf_plan &FFTPlan,                // FFT plan
    Eigen::MatrixXcf &savedFFTs);       // Output of FFT transformed time-domain data

void ConvertAndAppend(std::vector<float> &dataSegment, std::span<uint8_t> dataBytes, const int &DATA_SIZE, const int &HEAD_SIZE);
TimePoint GenerateTimestamp(std::vector<uint8_t>& dataBytes, const int NUM_CHAN);
bool CheckForDataErrors(Session& sess, std::vector<uint8_t>& dataBytes, const int MICRO_INCR, 
                        bool &previousTimeSet, TimePoint &previousTime, const int PACKET_SIZE);

DetectionResult ThresholdDetect(const Eigen::VectorXf &data, const std::span<TimePoint> times, const float &threshold, const int &SAMPLE_RATE);
DetectionResult ThresholdDetectFD(const Eigen::VectorXcf &data, const std::span<TimePoint> times, const float &threshold, const int &SAMPLE_RATE);

void ProcessSegmentInterleaved(std::span<float> data, Eigen::MatrixXf &channelData, const int NUM_CHAN);
