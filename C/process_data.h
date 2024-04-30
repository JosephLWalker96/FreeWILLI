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
    int minPeakIndex;
    int maxPeakIndex;
};

void PrintTimes(const std::vector<std::chrono::system_clock::time_point>& timestamps);
void ProcessSegment(arma::Row<int16_t>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegmentStacked(std::vector<int16_t>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegmentInterleaved(std::vector<int16_t>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE,
                               arma::Row<int16_t>& ch1, arma::Row<int16_t>& ch2, arma::Row<int16_t>& ch3, arma::Row<int16_t>& ch4);
DetectionResult ThresholdDetect(arma::Row<int16_t>& data, std::vector<TimePoint>& times, int threshold);
void WriteClicks(const std::vector<int16_t>& click_amps, const std::vector<std::chrono::system_clock::time_point>& timestamps, const std::string& filename);
#endif
