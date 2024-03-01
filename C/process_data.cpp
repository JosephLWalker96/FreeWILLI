#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <chrono>
#include <Eigen/Dense>

using TimePoint = std::chrono::system_clock::time_point;

void process_segment(const std::vector<double>& data, const std::vector<TimePoint>& times, const std::string& output_file) {
  Eigen::VectorXd data_abs = data.cast<double>().array().square().sqrt(); // Absolute value and square root

  // Average data using a filter
  Eigen::VectorXd pulse_filter = Eigen::VectorXd::Ones(256) / 256;
  Eigen::VectorXd filtered_signal = data_abs.convolve(pulse_filter);

  // Remove low amplitude values
  filtered_signal = filtered_signal.unaryExpr([](double x) { return x < 80 ? 0 : x; });

  // Create a mask to segment click regions
  Eigen::VectorXd filt = Eigen::VectorXd::Ones(256);
  Eigen::VectorXd output = filtered_signal.convolve(filt);
  output = output.unaryExpr([](double x) { return x > 0 ? 1 : 0; });

  // Find index of click regions and split
  Eigen::VectorXd diff = output.tail(output.size() - 1) - output.head(output.size() - 1);
  Eigen::VectorXi output_indices = diff.indices().unaryExpr([](int x) { return x + 1; });

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

    // Write clicks to output file
    std::ofstream file(output_file, std::ios_base::app);
    for (const auto& click : clicks) {
      file << click.first << ", " << click.second << "\n";
    }
  }
}



void process_segment_1550(const std::vector<double>& data, const std::vector<TimePoint>& times, const std::string& output_file) {
  // Extract first channel data (assuming 4 channels and starting from index 0)
  std::vector<double> ch1(data.size() / 4);
  std::copy(data.begin(), data.begin() + data.size() / 4, ch1.begin());

  // Process the extracted channel data
  process_segment(ch1, times, output_file);
}

void process_segment_1240(const std::vector<double>& data, const std::vector<TimePoint>& times, const std::string& output_file) {
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
