/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

#include "custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;


void ConvertData(std::vector<float>& dataSegment,std::vector<uint8_t>& dataBytes,unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE);

void GenerateTimestamps(std::vector<TimePoint>& dataTimes, vector<uint8_t>& dataBytes,unsigned int MICRO_INCR, bool& previousTimeSet, std::chrono::time_point<std::chrono::system_clock>& previousTime, string& detectionOutputFile, string& tdoaOutputFile, string& doaOutputFile);

DetectionResult ThresholdDetect(const Eigen::VectorXf& data, const std::vector<TimePoint>& times, const double& threshold, const unsigned int& SAMPLE_RATE);
void ProcessSegmentInterleaved(std::vector<float>& data, Eigen::MatrixXf& channelData, unsigned int NUM_CHAN);
