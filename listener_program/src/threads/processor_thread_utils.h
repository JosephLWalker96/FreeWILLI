#pragma once
#include "../pch.h"

class ObservationBuffer;
class SharedDataManager;
class RuntimeConfig;
class Tracker;
class BufferStruct;

using TimePoint = std::chrono::system_clock::time_point;

Eigen::MatrixXf loadHydrophonePositionsFromFile(const std::string &filename);

Eigen::MatrixXf calculateRelativePositions(const Eigen::MatrixXf &positions);

Eigen::MatrixXf getHydrophoneRelativePositions(const std::string &filename);

void hydrophoneMatrixDecomposition(const Eigen::MatrixXf hydrophonePositions, Eigen::MatrixXf &precomputedP,
                                   Eigen::MatrixXf &basisMatrixU, int &rankOfHydrophoneMatrix);

bool shouldTerminateProgram(const RuntimeConfig &runtimeConfig, const TimePoint &startLoop);

void waitForData(SharedDataManager &sess, ObservationBuffer &observationBuffer, RuntimeConfig &runtimeConfig,
                 std::vector<uint8_t> &dataBytes);

TimePoint generateTimestamp(std::vector<uint8_t> &dataBytes, const int numChannels);

bool checkForDataErrors(SharedDataManager &session, std::vector<uint8_t> &dataBytes, const int microIncrement,
                        bool &isPreviousTimeSetr, TimePoint &previousTime, TimePoint &currentTime, const int packetSize);

void writeDataToCerr(std::span<TimePoint> errorTimestamps, std::vector<std::vector<uint8_t>> erroredDataBytes);

void initializeOutputFiles(std::string &outputFile, std::unique_ptr<Tracker> &tracker, TimePoint &currentTime, const int numChannels);

std::string convertTimePointToString(const TimePoint &timePoint);

std::vector<std::string> generateChannelComboLabels(const std::string &labelPrefix, int numChannels);

auto readFirFilterFile(const std::string &filePath) -> std::vector<float>;

void saveSpectraForTraining(const std::string &filename, int label, const Eigen::VectorXcf &frequencyDomainData);