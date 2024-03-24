#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <eigen3/Eigen/Dense>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include <iomanip> // for output formatting

#include "my_globals.h"
#include "process_data.h"

using TimePoint = std::chrono::system_clock::time_point;

void PrintTimes(const std::vector<std::chrono::system_clock::time_point>& timestamps){
    for (auto& timestamp : timestamps){
        std::time_t time_t_representation = std::chrono::system_clock::to_time_t(timestamp);
        std::tm time_data = *std::localtime(&time_t_representation); 
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count() % 1000000; // Use std::put_time for human-readable formatting
        // Print the timestamp
        std::cout << "Timestamp: ";
        std::cout << time_data.tm_year + 1900 << '-'
            << time_data.tm_mon + 1 << '-'
            << time_data.tm_mday << ' '
            << time_data.tm_hour << ':'
            << time_data.tm_min << ':'
            << time_data.tm_sec << ':'
            << microsecs << std::endl;
    }
}

void ProcessSegment(arma::Row<int16_t>& data, std::vector<TimePoint>& times, const std::string& output_file) {
    arma::Row<double> data_abs = arma::abs(arma::conv_to<arma::Row<double>>::from(data));
    std::cout << "data_Abs " << data_abs.size() << std::endl;
    
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "dataAbs " << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << data_abs[j] << " ";
        }
        std::cout << std::endl;
    #endif
   
    // define the filter for click regions
    arma::Row<double> filter(1.0, 256, arma::fill::ones); // Filter of size 256 with all ones
    filter /= 256.0;
    
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "filter " << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << filter[j] << " ";
        }
        std::cout << std::endl;
    #endif
    // Convolve abs_data with the filter
    arma::Row<double> convolved_data = arma::conv(data_abs, filter, "same");

    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment(), convolved data: (before)" << std::endl;
        std::cout << convolved_data.size() << std::endl;
        for (size_t j = 0; j < 50; j++){
            std::cout << convolved_data[j] << " ";
        }
        std::cout << std::endl;
        std::cout << "end of conv: "<< std::endl;
        for (size_t j = convolved_data.size()-4; j < convolved_data.size(); j++){
            std::cout << convolved_data[j] << " ";
        }
        std::cout << std::endl;
    #endif
    
    // Thresholding low amplitude values
    convolved_data.elem(arma::find(convolved_data < 80.0)).fill(0.0);
    
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment(), convolved data (after) " << std::endl;
        for (size_t j = 0; j < 50; j++){
            std::cout << convolved_data[j] << " ";
        }
        std::cout << std::endl;
    #endif

    
    // Create a mask for click regions (ones filter)
    arma::Row<double> filt(1.0,256,arma::fill::ones);
    arma::Row<double> output_f = arma::conv(convolved_data, filt, "same");
    std::cout << "##################################  WHERE " << arma::find(output_f > 0.0) << std::endl;
    output_f.elem(arma::find(output_f > 0.0)).fill(1.0);
    

    // Find click region indices based on differences
    arma::Row<double> diff = arma::diff(output_f);
    
    arma::uvec output = arma::find(diff != 0.0);
    std::cout << "OUTPUT: " << output << std::endl;

    #ifdef DEBUG_PRINT_UNPACKED
        //std::cout << "Inside ProcessSegment(), split points !!!!!!!!!!!!!!!" << std::endl;
        //std::cout << output.n_elem << std::endl;
        /*
        for (size_t j = 0; j < output.size(); j++){
            std::cout << output[j] << " ";
        }
        std::cout << std::endl;
        */
    #endif
    
    // Split data and filtered signal based on click regions
    std::cout << "OUTPUT.N_ELEM " << output.n_elem << std::endl;
    std::cout << "CONVOLVED_DATA.MAX() " << convolved_data.max() << std::endl;

    if (output.n_elem > 0){
        std::vector<std::chrono::system_clock::time_point> timestamps;
        std::cout << "Above horiz" << std::endl;
        //output = arma::join_cols(arma::uvec(1, 0), output);
        output.insert_rows(0, 1);
        output(0) = 0;
        std::cout << output << std::endl;
        std::cout << "Below horiz" << std::endl;
        std::vector<int16_t> click_amps;
        std::cout << "Above for-loop" << std::endl;
        for (size_t i = 0; i < output.size() - 1; ++i) {
            int start_point = output[i];
            int end_point = output[i + 1];

            // Check if there's any non-zero value in the region
            std::cout << "Above if-statement" << std::endl;
            if (arma::any(convolved_data.subvec(start_point, end_point - 1) != 0.0)) {
                //arma::vec region_data = data.subvec(start_point, end_point - 1);
                std::cout << "Above region-data" << std::endl;
                arma::Row<int16_t> region_data = data.subvec(start_point, end_point - 1);

                // Calculate the index (avoid potential iterator subtraction issues)
                int peak_index = arma::index_max(region_data);
                int16_t peak_amplitude = arma::max(region_data);
                
                // Calculate click time based on sampling rate and peak index
                long long seconds = ((start_point + peak_index) * 10); // divide by 1e5 then times 1e6
                
                std::chrono::time_point<std::chrono::system_clock> click_time = times[0] + std::chrono::microseconds(seconds);
                click_amps.push_back(peak_amplitude);
                timestamps.push_back(click_time);
            }
        }
        WriteClicks(click_amps,timestamps, "Ccode_clicks.txt");
    }
}

void ProcessSegment1240(std::vector<int16_t>& data, std::vector<TimePoint>& times, const std::string& output_file) {
    size_t num_segments = data.size() / (4*124); // Calculate the number of segments

    // Initialize Armadillo row to store extracted elements
    arma::Row<int16_t> ch1(num_segments * 124);

    // Iterate over the data in steps of 100
    for (size_t i = 0; i < num_segments; ++i) {
        // Calculate the starting index of the current segment
        size_t segment_start = i * (4*124);

        // Extract 10 consecutive elements from the current segment
        for (size_t j = 0; j < 124; ++j) {
            ch1(i * 124 + j) = data[segment_start + j];
        }
    }
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment1240() " << std::endl;
        std::cout << "Ch1 size" << ch1.size() << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << ch1[j] << " ";
        }
        std::cout << std::endl;
        for (size_t j = ch1.size() - 4; j < ch1.size(); j++){
            std::cout << ch1[j] << " ";
        }
        std::cout << std::endl;
    #endif

    ProcessSegment(ch1, times, output_file);  // Use memptr to access raw data
}


void ProcessSegment1550(std::vector<int16_t>& data, std::vector<TimePoint>& times, const std::string& output_file) {
    arma::Row<int16_t> ch1;

    // Reserve space in the arma::Row (optional but can improve performance)
    ch1.set_size(data.size() / 4);

    // Iterate through the data vector and save every 4th element into the arma::Row
    for (size_t i = 0, j = 0; i < data.size(); i += 4, ++j) {
        ch1(j) = data[i]; // Saving every 4th element into ch1
    }
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment1550() " << std::endl;
        std::cout << "Ch1 size" << ch1.size() << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << ch1[j] << " ";
        }
        std::cout << std::endl;
        for (size_t j = ch1.size() - 4; j < ch1.size(); j++){
            std::cout << ch1[j] << " ";
        }
        std::cout << std::endl;
    #endif
        
    // Process the extracted channel data
    ProcessSegment(ch1, times, output_file);  // Use memptr to access raw data
}

void WriteClicks(const std::vector<int16_t>& click_amps,
                 const std::vector<std::chrono::system_clock::time_point>& timestamps,
                 const std::string& filename) {
  std::ofstream outfile(filename, std::ios::app);
  if (outfile.is_open()) {
    // Check if vectors have the same size
    if (click_amps.size() != timestamps.size()) {
      std::cerr << "Error: Click amplitude and timestamp vectors have different sizes." << std::endl;
      return;
    }

    // Write data rows
    for (size_t i = 0; i < click_amps.size(); ++i) {
      auto time_point = timestamps[i];
      auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
      outfile << time_since_epoch.count() << std::setw(20) << click_amps[i] << std::endl;
    }

    outfile.close();
  } else {
    std::cerr << "Error: Could not open file " << filename << std::endl;
  }
}
