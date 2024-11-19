/*

This file contains all function prototypes for utils.cpp

*/
#pragma once

#include "pch.h"
//#include "custom_types.h"
//#include "session.h"
//#include "socket_manager.h"
//#include "buffer_writter.h"

// forward declarations
class ExperimentRuntime;
class SocketManager;
class ObservationBuffer;
class ExperimentRuntime;
class BufferStruct;
class Tracker;

// Type aliases
using TimePoint = std::chrono::system_clock::time_point;

// File I/O functions
auto ReadFIRFilterFile(const std::string &fileName) -> std::vector<float>;
// void InitiateOutputFile(std::string &outputFile, TimePoint& currentTimestamp, const int NUM_CHAN);
void WritePulseAmplitudes(std::span<float> click_amps, std::span<TimePoint> timestamps, const std::string &filename);
void WriteArray(const std::span<Eigen::VectorXf> array, const std::span<TimePoint> timestamps, const std::string &filename);
void WriteDataToCerr(std::span<TimePoint> dataTimes, std::vector<std::vector<uint8_t>> dataBytesSaved);
void ShouldFlushBuffer(ObservationBuffer &observationBuffer, ExperimentRuntime &expRuntime, const std::chrono::system_clock::time_point &startLoop);
void ParseJSONConfig(SocketManager &sess, ExperimentRuntime &expRuntime, char *argv[]);

// Data handling functions
auto LoadHydrophonePositions(const std::string &filename) -> Eigen::MatrixXf;
void ClearQueue(std::queue<std::vector<uint8_t>> &q);
bool WithProbability(double probability);
auto GetExampleClick() -> std::vector<float>;
auto TimePointToString(const TimePoint &timePoint) -> std::string;

// Debug and print functions
void PrintTimes(const std::span<TimePoint> timestamps);
void PrintFirstFiveValues(const Eigen::MatrixXf &savedFFTs, const Eigen::MatrixXf &invFFT);
void PrintMode();

// Function to initialize the output CSV files and write column headers
void InitiateOutputFiles(std::string &outputFile, std::unique_ptr<Tracker> &tracker, TimePoint &currentTime, const int NUM_CHAN);

// Function to append data from BufferStruct to the CSV file
void AppendBufferToFile(const std::string &outputFile, const BufferStruct &buffer);

// Helper function to generate labels with a given prefix
std::vector<std::string> GenerateLabels(const std::string &prefix, int NUM_CHAN);

// Helper function to calculate the number of unique channel pairs
int GetNumberOfChannelPairs(int NUM_CHAN);