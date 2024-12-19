#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <string>

#include "../../src/threads/processor_thread_utils.h"
#include "../../src/firmware_config.h"
#include "../../src/algorithms/threshold_detectors.h"

using TimePoint = std::chrono::system_clock::time_point;

FirmwareConfig firmwareConfig;

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

    // Constants
    constexpr int HEADER_SIZE = 12;
    constexpr int CONTENT_SIZE = 8;
    constexpr int BYTES_PER_SAMP = 2;
    constexpr int SAMPLE_OFFSET = 32768; // Assuming SAMPLE_OFFSET is 32768 as used in the conversion

    // Create 12 header bytes algorithmically (e.g., sequential values starting from 0)
    std::vector<uint8_t> headerBytes(HEADER_SIZE);
    uint8_t headerValue = 0;
    std::generate(headerBytes.begin(), headerBytes.end(), [&headerValue]() { return headerValue++; });

    // Create 8 content bytes
    std::vector<uint8_t> contentBytes = {0x80, 0x04, 0x80, 0x0d, 0x80, 0x0e, 0x80, 0x12}; // Example content

    // Combine header and content bytes
    std::vector<uint8_t> dataBytes;
    dataBytes.insert(dataBytes.end(), headerBytes.begin(), headerBytes.end());
    dataBytes.insert(dataBytes.end(), contentBytes.begin(), contentBytes.end());

    // Validate dataBytes size
    ASSERT_EQ(dataBytes.size(), HEADER_SIZE + CONTENT_SIZE);

    // Expected results
    Eigen::MatrixXf expectedResult(4, 1); // 4 rows, 1 column for this example
    expectedResult << 4.0f, 13.0f, 14.0f, 18.0f; // Expected converted values for the content bytes

    // Initialize the matrix with sufficient rows and columns
    Eigen::MatrixXf dataSegment(4, 1); // 4 rows, 1 column for this example
    dataSegment.setZero();             // Initialize all values to zero

    // Set up the counter
    int counter = 0;

    // Call the function under test
    firmwareConfig.convertAndInsertData(dataSegment, std::span<uint8_t>(dataBytes), counter, HEADER_SIZE, CONTENT_SIZE, BYTES_PER_SAMP, SAMPLE_OFFSET);

    // Assert that the converted data matches the expectation
    EXPECT_TRUE(dataSegment.isApprox(expectedResult));
}

TEST(ConvertDataTest, EmptyData) {
    // Constants
    constexpr int HEADER_SIZE = 12;
    constexpr int CONTENT_SIZE = 0; // No content bytes
    constexpr int BYTES_PER_SAMP = 2;
    constexpr int SAMPLE_OFFSET = 32768;

    // Create empty header and content bytes
    std::vector<uint8_t> headerBytes(HEADER_SIZE);
    std::vector<uint8_t> contentBytes(CONTENT_SIZE);

    // Combine header and content bytes
    std::vector<uint8_t> dataBytes;
    dataBytes.insert(dataBytes.end(), headerBytes.begin(), headerBytes.end());
    dataBytes.insert(dataBytes.end(), contentBytes.begin(), contentBytes.end());

    // Initialize the matrix
    Eigen::MatrixXf dataSegment(1, 1); // Minimal matrix size
    dataSegment.setZero();             // Initialize all values to zero

    // Expected result
    Eigen::MatrixXf expectedResult(1, 1);
    expectedResult.setZero(); // Expect an empty result

    // Set up the counter
    int counter = 0;

    // Call the function under test
    firmwareConfig.convertAndInsertData(dataSegment, std::span<uint8_t>(dataBytes), counter, HEADER_SIZE, CONTENT_SIZE, BYTES_PER_SAMP, SAMPLE_OFFSET);

    // Assert that the converted data is empty
    EXPECT_TRUE(dataSegment.isApprox(expectedResult));
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
