/*

This file contains all function prototypes for process_data.cpp

*/

#pragma once

#include <armadillo>
#include <eigen3/Eigen/Dense>
#include "custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;


void ConvertData(std::vector<double>& dataSegment,std::vector<uint8_t>& dataBytes,unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE);
void ProcessSegment(arma::Col<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegmentStacked(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE, unsigned int& NUM_CHAN, unsigned int& SAMPS_PER_CHANNEL, unsigned int& NUM_PACKS_DETECT);
void ProcessSegmentInterleaved(std::vector<double>& data, Eigen::VectorXd& ch1, Eigen::VectorXd& ch2, Eigen::VectorXd& ch3, Eigen::VectorXd& ch4, unsigned int& NUM_CHAN);
DetectionResult ThresholdDetect(Eigen::VectorXd& data, std::vector<TimePoint>& times, const double& threshold, const unsigned int& SAMPLE_RATE);
