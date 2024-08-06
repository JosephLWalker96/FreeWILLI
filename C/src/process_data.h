/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

#include "custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;


void ConvertData(std::vector<float>& dataSegment,std::span<uint8_t> dataBytes,unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE);

void GenerateTimestamps(std::vector<TimePoint>& dataTimes, std::span<uint8_t> dataBytes,unsigned int MICRO_INCR, bool& previousTimeSet, std::chrono::time_point<std::chrono::system_clock>& previousTime, std::string& detectionOutputFile, std::string& tdoaOutputFile, std::string& doaOutputFile);

DetectionResult ThresholdDetect(const Eigen::VectorXf& data, const std::span<TimePoint> times, const double& threshold, const unsigned int& SAMPLE_RATE);
void ProcessSegmentInterleaved(std::span<float> data, Eigen::MatrixXf& channelData, unsigned int NUM_CHAN);
