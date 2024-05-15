/*

This file contains all function prototypes for process_data.cpp

*/

#ifndef PROCESS_DATA
#define PROCESS_DATA

#include <armadillo>
using TimePoint = std::chrono::system_clock::time_point;

struct DetectionResult {
    std::vector<std::chrono::system_clock::time_point> peakTimes;
    std::vector<double> peakAmplitude;
    int minPeakIndex = -1;
    int maxPeakIndex = -1;
};

void ConvertData(std::vector<double>& dataSegment,std::vector<uint8_t>& dataBytes,unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE);
void PrintTimes(const std::vector<std::chrono::system_clock::time_point>& timestamps);
void ProcessSegment(arma::Col<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegmentStacked(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegmentInterleaved(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE,
                               arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4);
DetectionResult ThresholdDetect(arma::Col<double>& data, std::vector<TimePoint>& times, double threshold);
void WritePulseAmplitudes(const std::vector<double>& click_amps, const std::vector<std::chrono::system_clock::time_point>& timestamps, const std::string& filename);
#endif
