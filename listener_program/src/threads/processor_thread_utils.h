#pragma once
#include "../pch.h"

class SharedDataManager;
class RuntimeConfig;

using TimePoint = std::chrono::system_clock::time_point;

//bool shouldTerminateProgram(const RuntimeConfig &runtimeConfig);

void waitForData(SharedDataManager &sess, std::vector<std::vector<uint8_t>> &dataBytes,int numPacksToGet);

std::string convertTimePointToString(const TimePoint &timePoint);

void writeDataToCerr(std::span<TimePoint> errorTimestamps, std::vector<std::vector<uint8_t>> erroredDataBytes);

void saveSpectraForTraining(const std::string &filename, int label, const Eigen::VectorXcf &frequencyDomainData);