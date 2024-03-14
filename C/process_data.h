#ifndef PROCESS_DATA
#define PROCESS_DATA

#include <armadillo>
using TimePoint = std::chrono::system_clock::time_point;

void PrintTimes(const std::vector<std::chrono::system_clock::time_point>& timestamps);
void process_segment(arma::vec& data, std::vector<TimePoint>& times, const std::string& output_file);
//void process_segment(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& output_file);
void process_segment_1240(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& output_file);

#endif
