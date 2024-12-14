#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <string>

#include "../../src/threads/processor_thread_utils.h"
#include "../../src/runtime_config.h"
#include "../../src/firmware_config.h"
#include "../../src/algorithms/threshold_detectors.h"

using TimePoint = std::chrono::system_clock::time_point;

// Helper function to create sample data and timestamps
std::pair<Eigen::VectorXf, std::vector<TimePoint>> CreateTestData(int size, double peakValue, int peakIndex)
{
  Eigen::VectorXf data(size);
  data.setZero();
  data(peakIndex) = peakValue;

  std::vector<TimePoint> timestamps(size);
  // Set timestamps based on your specific TimePoint implementation (adjust if needed)
  for (int i = 0; i < size; ++i)
  {
    timestamps[i] = std::chrono::system_clock::now();
  }
  return std::make_pair(data, timestamps);
}
/*
TEST(ConvertDataTest, ValidData)
{
  std::vector<uint8_t> dataBytes = {0x80, 0x04, 0x80, 0x0d, 0x80, 0x0e}; // Sample data bytes (modify as needed)
  unsigned int DATA_SIZE = dataBytes.size();
  unsigned int HEAD_SIZE = 0;
  std::vector<float> expectedResult = {4.0, 13.0, 14.0}; // Expected converted value (modify as needed)

  // Create an empty vector to store the converted data
  std::vector<float> dataSegment;

  // Call the function under test
  convertAndAppend(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE);

  // Assert that the converted data matches the expectation
  EXPECT_EQ(dataSegment, expectedResult);
}

/*
TEST(ConvertDataTest, NaNValue) {
  // Prepare test data with NaN value
  std::vector<uint8_t> dataBytes = {0xFF, 0xFF, 0x03, 0x04}; // Sample data with NaN (modify as needed)
  unsigned int DATA_SIZE = dataBytes.size();
  unsigned int HEAD_SIZE = 0;

  // Create an empty vector to store the converted data
  std::vector<double> dataSegment;

  // Expect a runtime_error to be thrown for NaN
  EXPECT_THROW({ ConvertData(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE); }, std::runtime_error);
}

TEST(ConvertDataTest, InfValue) {
  // Prepare test data with Inf value
  std::vector<uint8_t> dataBytes = {0xFF, 0x7F, 0x03, 0x04}; // Sample data with positive Inf (modify as needed)
  unsigned int DATA_SIZE = dataBytes.size();
  unsigned int HEAD_SIZE = 0;

  // Create an empty vector to store the converted data
  std::vector<double> dataSegment;

  // Expect a runtime_error to be thrown for Inf
  EXPECT_THROW({ ConvertData(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE); }, std::runtime_error);
}

TEST(ConvertDataTest, EmptyData) {
  // Prepare empty data
  std::vector<uint8_t> dataBytes;
  unsigned int DATA_SIZE = dataBytes.size();
  unsigned int HEAD_SIZE = 0;
  std::vector<double> expectedResult; // Empty expected result

  // Create an empty vector to store the converted data
  std::vector<double> dataSegment;

  // Call the function under test
  ConvertData(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE);

  // Assert that the converted data is empty
  EXPECT_EQ(dataSegment, expectedResult);
}


*/

TEST(ThresholdDetectTest, NoPeak)
{

  // Create data with no peak above threshold
  double threshold = 0.5;
  int dataSize = 100;
  auto testData = CreateTestData(dataSize, 0.2, 50); // Peak value below threshold
  Eigen::VectorXf &data = testData.first;
  std::vector<TimePoint> &times = testData.second;

  // Call ThresholdDetect
  DetectionResult result = detectTimeDomainThreshold(data, times, threshold, 100000);

  // Verify no peak detected
  EXPECT_EQ(result.minPeakIndex, -1);
  EXPECT_EQ(result.maxPeakIndex, -1);
  EXPECT_EQ(result.peakAmplitude, 0);
}

TEST(ThresholdDetectTest, SinglePeak)
{
  // Create data with a single peak above threshold
  double threshold = 0.5;
  int dataSize = 100;
  double peakValue = 0.8;
  int peakIndex = 30;
  auto testData = CreateTestData(dataSize, peakValue, peakIndex);
  Eigen::VectorXf &data = testData.first;
  std::vector<TimePoint> &times = testData.second;

  // Call ThresholdDetect
  DetectionResult result = detectTimeDomainThreshold(data, times, threshold, 100000);

  // Verify peak information is correct
  EXPECT_EQ(result.minPeakIndex, peakIndex);
  EXPECT_EQ(result.maxPeakIndex, peakIndex);
  EXPECT_FLOAT_EQ(result.peakAmplitude, peakValue);

  // Verify peak time is within expected range (adjust tolerance based on your TimePoint implementation)
  // auto expectedPeakTime = times[peakIndex] + std::chrono::microseconds((long)(peakIndex * 1e6) / SAMPLE_RATE);
  // EXPECT_LT(std::chrono::duration_cast<std::chrono::microseconds>(result.peakTimes[0] - expectedPeakTime).count(), 100); // Tolerance of 100 microseconds
}

TEST(ThresholdDetectTest, Infinity)
{
  // Create data with a single peak above threshold
  double threshold = 0.5;
  int dataSize = 100;
  double peakValue = 0.8;
  int peakIndex = 30;
  auto testData = CreateTestData(dataSize, peakValue, peakIndex);
  Eigen::VectorXf &data = testData.first;
  std::vector<TimePoint> &times = testData.second;

  data(peakIndex) = std::numeric_limits<float>::infinity();

  // Call ThresholdDetect
  DetectionResult result = detectTimeDomainThreshold(data, times, threshold, 100000);

  // Verify peak information is correct
  EXPECT_EQ(result.minPeakIndex, peakIndex);
  EXPECT_EQ(result.maxPeakIndex, peakIndex);
  EXPECT_DOUBLE_EQ(result.peakAmplitude, std::numeric_limits<float>::infinity());

  // Verify peak time is within expected range (adjust tolerance based on your TimePoint implementation)
  // auto expectedPeakTime = times[peakIndex] + std::chrono::microseconds((long)(peakIndex * 1e6) / SAMPLE_RATE);
  // EXPECT_LT(std::chrono::duration_cast<std::chrono::microseconds>(result.peakTimes[0] - expectedPeakTime).count(), 100); // Tolerance of 100 microseconds
}
