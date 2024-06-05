/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

#include <armadillo>
#include "custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;


void ConvertData(std::vector<double>& dataSegment,std::vector<uint8_t>& dataBytes,unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE);
void PrintTimes(const std::vector<TimePoint>& timestamps);
void ProcessSegment(arma::Col<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegmentStacked(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE, unsigned int& NUM_CHAN, unsigned int& SAMPS_PER_CHANNEL, unsigned int& NUM_PACKS_DETECT);
void ProcessSegmentInterleaved(std::vector<double>& data,arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, unsigned int& NUM_CHAN);
DetectionResult ThresholdDetect(arma::Col<double>& data, std::vector<TimePoint>& times, const double& threshold,const unsigned int& SAMPLE_RATE);
void WritePulseAmplitudes(const std::vector<double>& click_amps, const std::vector<TimePoint>& timestamps, const std::string& filename);
