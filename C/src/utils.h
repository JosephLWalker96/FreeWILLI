/*

This file contains all function prototypes for utils.cpp

*/

#pragma once

#include "custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;

std::vector<double> ReadFIRFilterFile(const std::string& fileName);
bool ProcessFile(Experiment& exp, const std::string& fileName); 
void InitiateOutputFile(std::string& outputFile, std::tm& timeStruct, int64_t microSec, std::string& feature);
void PrintTimes(const std::span<TimePoint> timestamps);
void RestartListener(Session& sess);
void ClearQueue(std::queue<std::vector<uint8_t>>& q);
bool WithProbability(double probability);
void WritePulseAmplitudes(std::span<float> click_amps, std::span<TimePoint> timestamps, const std::string& filename);
void WriteArray(const std::span<Eigen::VectorXf> array, const std::span<TimePoint> timestamps, const std::string& filename);
void WriteDataToCerr(std::span<TimePoint> dataTimes, std::vector<std::vector<uint8_t>> dataBytesSaved);
