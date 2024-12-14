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

// void convertAndAppend(std::span<float> dataSegment, std::span<uint8_t> dataBytes, const int &dataSize, const int &headerSize);
/*
template <typename Alloc>
void convertAndAppend(std::vector<float, Alloc> &dataSegment, std::span<uint8_t> dataBytes, const int &dataSize, const int &headerSize)
{
// Hint the compiler to vectorize
#pragma omp simd
    for (size_t i = 0; i < dataSize; i += 2)
    {
        float value = static_cast<float>(static_cast<uint16_t>(dataBytes[headerSize + i]) << 8) +
                      static_cast<float>(dataBytes[headerSize + i + 1]);
        value -= 32768.0f;
        dataSegment.push_back(value);
    }
}
*/
/*
template <typename Alloc>
void convertAndAppend(std::vector<float, Alloc> &dataSegment, std::span<uint8_t> dataBytes,
                      const int &dataSize, const int &headerSize)
{
    // Each sample consists of 2 bytes
    size_t numSamples = dataSize / 2;
    size_t oldSize = dataSegment.size();
    dataSegment.resize(oldSize + numSamples);

    // Get raw pointers and use restrict
    float *__restrict__ outPtr = &dataSegment[oldSize];
    const uint8_t *__restrict__ inPtr = dataBytes.data() + headerSize;

    // Assume alignment if you know your data is aligned (16 bytes as an example)
    // These assumptions are just hints to the compiler
    __builtin_assume_aligned(outPtr, 16);
    __builtin_assume_aligned(inPtr, 16);

// Hint the compiler to vectorize
#pragma omp simd
    for (size_t i = 0; i < numSamples; ++i)
    {
        // Load two bytes
        uint16_t sample = (static_cast<uint16_t>(inPtr[2 * i]) << 8) | inPtr[2 * i + 1];
        float value = static_cast<float>(sample) - 32768.0f;
        outPtr[i] = value;
    }
}
*/
void convertAndAppend(Eigen::MatrixXf &channelMatrix, 
                      std::span<uint8_t> dataBytes, 
                      int dataSize, 
                      int headerSize,
                      int& counter);

TimePoint generateTimestamp(std::vector<uint8_t> &dataBytes, const int numChannels);

bool checkForDataErrors(Session &session, std::vector<uint8_t> &dataBytes, const int microIncrement,
                        bool &isPreviousTimeSet, TimePoint &previousTime, const int packetSize);

void processSegmentInterleaved(std::span<float> interleavedData, Eigen::MatrixXf &channelMatrix, const int numChannels);

void writeDataToCerr(std::span<TimePoint> errorTimestamps, std::vector<std::vector<uint8_t>> erroredDataBytes);

void initializeOutputFiles(std::string &outputFile, std::unique_ptr<Tracker> &tracker, TimePoint &currentTime, const int numChannels);

std::string convertTimePointToString(const TimePoint &timePoint);

std::vector<std::string> generateChannelComboLabels(const std::string &labelPrefix, int numChannels);

auto readFirFilterFile(const std::string &filePath) -> std::vector<float>;

void saveSpectraForTraining(const std::string &filename, int label, const Eigen::VectorXcf &frequencyDomainData);