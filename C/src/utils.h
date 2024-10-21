/*

This file contains all function prototypes for utils.cpp

*/
#pragma once

#include "custom_types.h"
#include "session.h"
#include "socket_manager.h"
#include "buffer_writter.h"

// Type aliases
using TimePoint = std::chrono::system_clock::time_point;

// File I/O functions
auto ReadFIRFilterFile(const std::string& fileName) -> std::vector<float>;
void InitiateOutputFile(std::string &outputFile, TimePoint& currentTimestamp, const int NUM_CHAN);
void WritePulseAmplitudes(std::span<float> click_amps, std::span<TimePoint> timestamps, const std::string &filename);
void WriteArray(const std::span<Eigen::VectorXf> array, const std::span<TimePoint> timestamps, const std::string &filename);
void WriteDataToCerr(std::span<TimePoint> dataTimes, std::vector<std::vector<uint8_t>> dataBytesSaved);
void ShouldFlushBuffer(BufferWriter &bufferWriter, Session &sess, ExperimentRuntime &expRuntime, const std::chrono::system_clock::time_point& startLoop);
void ParseJSONConfig(SocketManager& sess, ExperimentRuntime& expRuntime, char* argv[]);

// Data handling functions
auto LoadHydrophonePositions(const std::string& filename)-> Eigen::MatrixXd;
void ClearQueue(std::queue<std::vector<uint8_t>> &q);
bool WithProbability(double probability);
auto GetExampleClick() -> std::vector<float>;
auto TimePointToString(const TimePoint& timePoint) -> std::string;

// Debug and print functions
void PrintTimes(const std::span<TimePoint> timestamps);
void PrintFirstFiveValues(const Eigen::MatrixXf &savedFFTs, const Eigen::MatrixXf &invFFT);
void PrintMode();