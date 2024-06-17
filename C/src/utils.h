/*

This file contains all function prototypes for utils.cpp

*/

#pragma once

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <armadillo>
#include "custom_types.h"

using std::string;
using std::vector;
using TimePoint = std::chrono::system_clock::time_point;


arma::Col<double> ReadFIRFilterFile(const string& fileName);
int ProcessFile(Experiment& exp, const string fileName); 
void InitiateOutputFile(string& outputFile, std::tm& timeStruct, int64_t microSec, string& feature);
void PrintTimes(const std::vector<TimePoint>& timestamps);
void RestartListener(Session& sess);
void ClearQueue(std::queue<vector<uint8_t>>& q);
bool WithProbability(double probability);
void WritePulseAmplitudes(const std::vector<double>& click_amps, const std::vector<TimePoint>& timestamps, const std::string& filename);
void WriteArray(const arma::Col<double>& array, const vector<TimePoint>& timestamps, const string& filename);
void WriteDataToCerr(vector<TimePoint>& dataTimes,vector<double>& dataSegment, vector<vector<uint8_t>>& dataBytesSaved);
