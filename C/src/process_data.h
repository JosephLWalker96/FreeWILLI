/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

#include "custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;

void FrequencyDomainFIRFiltering(
    const Eigen::MatrixXf &channelData, // Zero-padded time-domain data
    const Eigen::VectorXcf &filterFreq, // Frequency domain filter (FIR taps in freq domain)
    fftwf_plan &FFTPlan,                // FFT plan
    Eigen::MatrixXcf &savedFFTs);       // Output of FFT transformed time-domain data

void ConvertData(std::vector<float> &dataSegment, std::span<uint8_t> dataBytes, const int &DATA_SIZE, const int &HEAD_SIZE);
void GenerateTimestamps(std::vector<TimePoint> &dataTimes, std::span<uint8_t> dataBytes, const int MICRO_INCR,
                        bool &previousTimeSet, std::chrono::time_point<std::chrono::system_clock> &previousTime,
                        std::string &detectionOutputFile, const int NUM_CHAN);

DetectionResult ThresholdDetect(const Eigen::VectorXf &data, const std::span<TimePoint> times, const float &threshold, const int &SAMPLE_RATE);
DetectionResult ThresholdDetectFD(const Eigen::VectorXcf &data, const std::span<TimePoint> times, const float &threshold, const int &SAMPLE_RATE);

void ProcessSegmentInterleaved(std::span<float> data, Eigen::MatrixXf &channelData, const int NUM_CHAN);
