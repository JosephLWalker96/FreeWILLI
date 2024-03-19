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

#include "my_globals.h"

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
/*
void process_segment(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& output_file) {
  //Eigen::VectorXd data_abs = data.cast<double>().array().square().sqrt(); // Absolute value and square root
  Eigen::Map<Eigen::MatrixXd> data_matrix(data.data(), 1, DATA_SEGMENT_LENGTH);
  Eigen::MatrixXd data_abs = data_matrix.abs();

  //Eigen::VectorXd eigen_vector = Eigen::Map<Eigen::VectorXd>(data.data(), data.size());

  //Eigen::VectorXd data_abs = eigen_vector.array().abs2(); // .cast<double>().array().square().sqrt(); // Absolute value and square root

  // Average data using a filter
  //Eigen::VectorXd pulse_filter = Eigen::VectorXd::Ones(256) / 256;
  Eigen::MatrixXd pulse_filter(1, 3);
  pulse_filter.setOnes();

  Eigen::MatrixXd filtered_signal = data_abs.convolve(pulse_filter);

  // Remove low amplitude values
  filtered_signal = filtered_signal.unaryExpr([](double x) { return x < 80 ? 0 : x; });

  // Create a mask to segment click regions
  Eigen::VectorXd filt = Eigen::VectorXd::Ones(256);
  Eigen::VectorXd output = filtered_signal.convolve(filt);
  output = output.unaryExpr([](double x) { return x > 0 ? 1 : 0; });

  // Find index of click regions and split
  Eigen::VectorXd diff = output.tail(output.size() - 1) - output.head(output.size() - 1);
filtered_signal.elem(arma::find(filtered_signal < 80)).fill(0);  Eigen::VectorXi output_indices = diff.indices().unaryExpr([](int x) { return x + 1; });

  std::vector<Eigen::VectorXd> non_zero_mask_regions = split_vector(filtered_signal, output_indices);
  std::vector<Eigen::VectorXd> non_zero_regions = split_vector(data.cast<double>(), output_indices);

  if (non_zero_regions.size() > 1) {
    std::vector<std::pair<TimePoint, double>> clicks;
    output.conservativeResize(output.size() + 1);
    output(0) = 0;

    for (int index = 0; index < non_zero_mask_regions.size(); ++index) {
      if (non_zero_mask_regions[index].any()) {
        double seconds = (output[index] + non_zero_regions[index].maxCoeff()) * 1e-5;
        TimePoint click_time = times[0] + std::chrono::microseconds(static_cast<int>(seconds * 1e6));
        double peak_amp = non_zero_regions[index].maxCoeff();
        clicks.push_back({click_time, peak_amp});
      }
    }
    cout << sizeof(clicks) << endl;
    // Write clicks to output file
    std::ofstream file(output_file, std::ios_base::app);
    for (const auto& click : clicks) {
      file << click.first << ", " << click.second << "\n";
    }
    
  }
}
*/

void ProcessSegment(arma::Row<int16_t>& data, std::vector<TimePoint>& times, const std::string& output_file) {
    // Convert data to absolute values (square and sqrt)
    //arma::vec data_abs = arma::pow(data, 2.0);
    //data_abs = arma::sqrt(data_abs);
    arma::Row<double> data_abs = arma::conv_to<arma::Row<double>>::from(data);

    //arma::Row<double> data_abs = arma::abs(data);
    data_abs = arma::abs(data_abs);
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment() " << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << data_abs[j] << " ";
        }
        std::cout << std::endl;
    #endif
   
    // define the filter for click regions
    arma::Row<double> filter(1, 256, arma::fill::ones); // Filter of size 256 with all ones
    filter /= 256.0;
    
    // Convolve abs_data with the filter
    arma::Row<double> convolved_data = arma::conv(data_abs, filter, "same");

    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment(), convolved data: (before)" << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << convolved_data[j] << " ";
        }
        std::cout << std::endl;
    #endif
    
    // Thresholding low amplitude values
    convolved_data.elem(arma::find(convolved_data < 80)).fill(0);
    
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment(), convolved data (after) " << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << convolved_data[j] << " ";
        }
        std::cout << std::endl;
    #endif

    
    // Create a mask for click regions (ones filter)
    arma::Row<double> filt(1,256,arma::fill::ones);
    arma::Row<double> output_f = arma::conv(convolved_data, filt, "same");
    output_f.elem(arma::find(output_f > 0.0)).fill(1.0);
    
    
    // Find click region indices based on differences
    arma::Row<double> diff = arma::diff(output_f);
    //arma::uvec output = arma::find(diff != 0.0);
    
    //arma::vec output = arma::conv_to<arma::vec>::from(arma::find(diff != 0.0));
    arma::uvec output = arma::find(diff != 0.0);
    
    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment(), split points !!!!!!!!!!!!!!!" << std::endl;
        std::cout << output.n_elem << std::endl;
        /*
        for (size_t j = 0; j < output.size(); j++){
            std::cout << output[j] << " ";
        }
        std::cout << std::endl;
        */
    #endif
    /*
    // Split data and filtered signal based on click regions
    arma::mat non_zero_mask_regions(output.n_elem, filtered_signal.max());
    arma::mat non_zero_regions(data.n_elem, data.max());  // Assuming data holds max values for regions

    non_zero_mask_regions.fill(0);
    non_zero_regions.fill(0);

    for (int i = 0; i < output.n_elem; ++i) {
        int start_idx = i > 0 ? output[i - 1] + 1 : 0;
        int end_idx = output[i];
        non_zero_mask_regions.row(i) = filtered_signal.subvec(start_idx, end_idx);
        non_zero_regions.row(i) = data.subvec(start_idx, end_idx);
    }
    */


    /*
    // Extract click information
    arma::vec clicks;
    if (non_zero_regions.n_elem > 1) {
        output = arma::join_horiz(arma::zeros(1), output);
        for (int i = 0; i < non_zero_mask_regions.n_elem; ++i) {
            if (arma::any(non_zero_mask_regions.row(i) > 0)) {
            //double seconds = (output[i] + arma::index_max(non_zero_regions[i])) * 1e-5;
            //double seconds = (output[i] + (non_zero_regions[i]).arma::index_max()) * 1e-5;
            double seconds = (output[i] + arma::as_scalar(arma::index_max(non_zero_regions[i]))) * 1e-5;
            arma::umat click_time = times[0] + arma::datetime<arma::u64>(seconds * 1e6, arma::micro); // Assuming times is arma::vec
            double peak_amp = arma::max(non_zero_regions[i]);
            clicks = arma::join_vert(clicks, arma::vec({click_time[0], peak_amp}));
            }
        }
    }

  // (Optional) Write clicks to a file (replace with your preferred method)
  // std::ofstream outfile(output_file);
  // outfile << clicks << std::endl;
  // outfile.close();

  return clicks;
  */
}

void ProcessSegment1240(std::vector<int16_t>& data, std::vector<TimePoint>& times, const std::string& output_file) {
    // Reshape the data into its original components (assuming 4 rows and 124 columns)
    if (data.size() % (4 * 124) != 0) {
    // Handle case where data size is not divisible by 488 (4 rows * 124 columns)
    // ... (implement appropriate error handling or logic)
        std::cout << "ERROR in process_segment_1240 function: not divisible" << std::endl;
    }

    const int num_rows = 1; //data.size() / (4 * 124);
    const int num_cols = 124;
    //arma::mat data_reshaped = arma::mat(data).reshape(num_rows, num_cols);  // Reshape directly from std::vector
    //arma::mat data_reshaped = arma::vec::from(data).reshape(num_rows, num_cols);
    //arma::imat data_reshaped(data.data(), num_rows, num_cols, false, true);
    arma::Mat<int16_t> data_reshaped(data.data(), num_rows * 4, num_cols, false, true);

    // Extract the first channel (assuming 4 channels and row-major order)
    //arma::vec ch1 = data_reshaped.col(0);
    arma::Row<int16_t> ch1 = data_reshaped.row(0);
    /*
    for (int i = 0; i < 10; i++){
        std::cout << ch1[i] << std::endl;
    }
    */

    #ifdef DEBUG_PRINT_UNPACKED
        std::cout << "Inside ProcessSegment1240() " << std::endl;
        for (size_t j = 0; j < 12; j++){
            std::cout << ch1[j] << " ";
        }
        std::cout << std::endl;
    #endif
        
    // Process the extracted channel data
    ProcessSegment(ch1, times, output_file);  // Use memptr to access raw data
}

/*
void process_segment_1550(const std::vector<double>& data, const std::vector<TimePoint>& times, const std::string& output_file) {
  // Extract first channel data (assuming 4 channels and starting from index 0)
  std::vector<double> ch1(data.size() / 4);
  std::copy(data.begin(), data.begin() + data.size() / 4, ch1.begin());

  // Process the extracted channel data
  process_segment(ch1, times, output_file);
}



void process_segment_1240(std::vector<double>& data, std::vector<TimePoint>& times, const std::string& output_file) {
  // Reshape the data into its original components (assuming 4 rows and 124 columns)
  if (data.size() % (4 * 124) != 0) {
    // Handle case where data size is not divisible by 488 (4 rows * 124 columns)
    // ... (implement appropriate error handling or logic)
  }

  const int num_rows = data.size() / (4 * 124);
  const int num_cols = 124;
  Eigen::MatrixXd data_reshaped(num_rows, num_cols);

  int index = 0;
  for (int row = 0; row < num_rows; ++row) {
    for (int col = 0; col < num_cols; ++col) {
      data_reshaped(row, col) = data[index];
      ++index;
    }
  }

  // Extract the first channel (assuming 4 channels and row-major order)
  Eigen::VectorXd ch1 = data_reshaped.col(0);

  // Process the extracted channel data
  process_segment(ch1.toStdVector(), times, output_file);
}


void write_clicks(const std::vector<std::pair<TimePoint, double>>& clicks, const std::string& output_file) {
  // Open the file in append mode (assuming you want to add to existing content)
  std::ofstream file(output_file, std::ios_base::app);

  // Check if file opened successfully
  if (!file.is_open()) {
    // Handle the error (e.g., print an error message)
    return;
  }

  // Write each click data to the file
  for (const auto& click : clicks) {
    // Format and write the click time and peak amplitude
    file << click.first << ", " << click.second << std::endl;
  }

  file.close();
}
*/
