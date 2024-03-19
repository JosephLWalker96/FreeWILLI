/*

This file contains all function prototypes for process_data.cpp

*/

#ifndef PROCESS_DATA
#define PROCESS_DATA

#include <armadillo>
using TimePoint = std::chrono::system_clock::time_point;

void PrintTimes(const std::vector<std::chrono::system_clock::time_point>& timestamps);
void ProcessSegment(arma::Row<int16_t>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);
void ProcessSegment1240(std::vector<int16_t>& data, std::vector<TimePoint>& times, const std::string& OUTPUT_FILE);

#endif
