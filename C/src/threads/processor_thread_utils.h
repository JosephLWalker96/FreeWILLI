#pragma once
#include "../pch.h"

class ObservationBuffer;
class Session;
class RuntimeConfig;
class Tracker;
class BufferStruct;

using TimePoint = std::chrono::system_clock::time_point;

void initializeFilterWeights(const std::string &filterWeightsPath, const int CHANNEL_SIZE,
                             Eigen::VectorXcf &filterFreq, std::vector<float> &paddedFilterWeights);

Eigen::MatrixXf loadHydrophonePositionsFromFile(const std::string &filename);

Eigen::MatrixXf calculateRelativePositions(const Eigen::MatrixXf &positions);

Eigen::MatrixXf getHydrophoneRelativePositions(const std::string &filename);

void setupFftPlans(int paddedLength, int fftOutputSize, int numChannels,
                   Eigen::MatrixXf &channelData, Eigen::MatrixXcf &savedFFTs,
                   fftwf_plan &forwardFftPlan);

void hydrophoneMatrixDecomposition(const Eigen::MatrixXf hydrophonePositions, Eigen::MatrixXf &precomputedP,
                                   Eigen::MatrixXf &basisMatrixU, int &rankOfHydrophoneMatrix);

bool shouldTerminateProgram(const RuntimeConfig &runtimeConfig, const TimePoint &startLoop);

void waitForData(Session &sess, ObservationBuffer &observationBuffer, RuntimeConfig &runtimeConfig,
                 std::vector<uint8_t> &dataBytes);

void convertAndAppend(std::vector<float> &dataSegment, std::span<uint8_t> dataBytes, const int &dataSize, const int &headerSize);

TimePoint generateTimestamp(std::vector<uint8_t> &dataBytes, const int numChannels);

bool checkForDataErrors(Session &session, std::vector<uint8_t> &dataBytes, const int microIncrement,
                        bool &isPreviousTimeSet, TimePoint &previousTime, const int packetSize);

void processSegmentInterleaved(std::span<float> interleavedData, Eigen::MatrixXf &channelMatrix, const int numChannels);

void writeDataToCerr(std::span<TimePoint> errorTimestamps, std::vector<std::vector<uint8_t>> erroredDataBytes);

void initializeOutputFiles(std::string &outputFile, std::unique_ptr<Tracker> &tracker, TimePoint &currentTime, const int numChannels);

std::string convertTimePointToString(const TimePoint &timePoint);

std::vector<std::string> generateChannelComboLabels(const std::string &labelPrefix, int numChannels);

auto readFirFilterFile(const std::string &filePath) -> std::vector<float>;