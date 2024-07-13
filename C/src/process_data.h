/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

#include "pch.h"
#include "utils.h"

using TimePoint = std::chrono::system_clock::time_point;


void ConvertData(std::vector<float>& dataSegment,std::vector<uint8_t>& dataBytes,unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE);

void GenerateTimestamps(std::vector<TimePoint>& dataTimes, vector<uint8_t>& dataBytes,unsigned int MICRO_INCR, bool& previousTimeSet, std::chrono::time_point<std::chrono::system_clock>& previousTime, string& detectionOutputFile, string& tdoaOutputFile, string& doaOutputFile);

void ProcessSegmentInterleaved(std::vector<float>& data, Eigen::VectorXf& ch1, Eigen::VectorXf& ch2, Eigen::VectorXf& ch3, Eigen::VectorXf& ch4, unsigned int& NUM_CHAN);
DetectionResult ThresholdDetect(Eigen::VectorXf& data, std::vector<TimePoint>& times, const double& threshold, const unsigned int& SAMPLE_RATE);
