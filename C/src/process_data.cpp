#include "pch.h"
#include "utils.h"
#include "process_data.h"
#include "session.h"
#include "tracker.h"

using TimePoint = std::chrono::system_clock::time_point;

// forward declarations
class Tracker;

// Function to perform frequency domain FIR filtering
void FrequencyDomainFIRFiltering(
    const Eigen::MatrixXf &channelData, // Zero-padded time-domain data
    const Eigen::VectorXcf &filterFreq, // Frequency domain filter (FIR taps in freq domain)
    fftwf_plan &forwardFFT,             // FFT plan
    Eigen::MatrixXcf &savedFFTs)        // Output of FFT transformed time-domain data
{
    int numChannels = channelData.cols();
    // std::cout << "numChannels: " << numChannels << std::endl;

    // Perform FFT on the input time-domain data
    fftwf_execute(forwardFFT);

    // Apply the frequency domain filter to each channel

    for (int i = 0; i < numChannels; i++)
    {
        savedFFTs.col(i) = savedFFTs.col(i).array() * filterFreq.array();
    }
}

void ConvertAndAppend(std::vector<float> &dataSegment, std::span<uint8_t> dataBytes, const int &DATA_SIZE, const int &HEAD_SIZE)
{
    /**
     * @brief Converts raw data bytes to double values and stores them in the provided data segment.
     *
     * This function converts raw data bytes to double values using a specific format and stores them
     * in the provided data segment vector. The conversion involves combining two bytes into a single
     * double value and then adjusting it to account for an offset. If the converted value contains NaN (Not a Number),
     * an exception is thrown.
     *
     * @param dataSegment A reference to a vector of doubles where the converted data values will be stored.
     * @param dataBytes A reference to a vector of unsigned 8-bit integers representing the raw data bytes to be converted.
     * @param DATA_SIZE The size of the data segment to be converted.
     * @param HEAD_SIZE The size of the header portion (timestamps) to skip at the beginning).
     */

    for (size_t i = 0; i < DATA_SIZE; i += 2)
    {
        float value = static_cast<float>(static_cast<uint16_t>(dataBytes[HEAD_SIZE + i]) << 8) +
                      static_cast<float>(dataBytes[i + HEAD_SIZE + 1]);
        value -= 32768.0;
        dataSegment.push_back(value);
    }
}

TimePoint GenerateTimestamp(std::vector<uint8_t> &dataBytes, const int NUM_CHAN)
{

    std::tm timeStruct{};                                 // Initialize a std::tm structure to hold the date and time components
    timeStruct.tm_year = (int)dataBytes[0] + 2000 - 1900; // Offset for year since 2000.. tm_year is years since 1900
    timeStruct.tm_mon = (int)dataBytes[1] - 1;            // Months are 0-indexed
    timeStruct.tm_mday = (int)dataBytes[2];
    timeStruct.tm_hour = (int)dataBytes[3];
    timeStruct.tm_min = (int)dataBytes[4];
    timeStruct.tm_sec = (int)dataBytes[5];

    // Calculate microseconds from the given bytes by shifting and combining them
    int64_t microSec = (static_cast<int64_t>(dataBytes[6]) << 24) +
                       (static_cast<int64_t>(dataBytes[7]) << 16) +
                       (static_cast<int64_t>(dataBytes[8]) << 8) +
                       static_cast<int64_t>(dataBytes[9]);

    // Use std::mktime to convert std::tm to std::time_t
    std::time_t timeResult = std::mktime(&timeStruct);

    // Check if mktime failed
    if (timeResult == std::time_t(-1))
    {
        throw std::runtime_error("Error: failure in mktime \n");
    }

    auto currentTime = std::chrono::system_clock::from_time_t(timeResult); // convert std::time_t to std::chrono::system_clock::time_point
    currentTime += std::chrono::microseconds(microSec);

    return currentTime;
}
bool CheckForDataErrors(Session &sess, std::vector<uint8_t> &dataBytes, const int MICRO_INCR, bool &previousTimeSet,
                        TimePoint &previousTime, const int PACKET_SIZE)
{

    // Calculate the elapsed time in microseconds since the previous time point
    auto currentTime = sess.dataTimes.back();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count();

    // Check if the previous time was set and if the elapsed time is not equal to the expected increment
    if (previousTimeSet && (elapsedTime != MICRO_INCR))
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Time not incremented by " << MICRO_INCR << " " << elapsedTime << std::endl;

        std::cerr << msg.str() << std::endl;
        WriteDataToCerr(sess.dataTimes, sess.dataBytesSaved);
        sess.dataTimes.clear();
        sess.dataSegment.clear();
        sess.dataBytesSaved.clear();
        previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
        previousTimeSet = false;
        return true;
        // throw std::runtime_error(msg.str());
    }
    else if (dataBytes.size() != PACKET_SIZE)
    {                          // Check if the amount of bytes in packet is what is expected
        std::stringstream msg; // compose message to dispatch
        msg << "Error: incorrect number of bytes in packet: " << "PACKET_SIZE: " << PACKET_SIZE << " dataBytes size: " << dataBytes.size() << std::endl;
        throw std::runtime_error(msg.str());
    }

    previousTime = currentTime;
    previousTimeSet = true;
    return false;
}

DetectionResult ThresholdDetect(const Eigen::VectorXf &data, const std::span<TimePoint> times, const float &threshold, const int &SAMPLE_RATE)
{
    /**
     * @brief Detects peaks in the input data above a specified threshold.
     *
     * This function analyzes the input data to detect peaks that exceed the specified threshold.
     * If a peak is found, its index, amplitude, and corresponding time are recorded in a DetectionResult structure.
     *
     * @param data A reference to an Eigen vector containing the input data.
     * @param times A vector of TimePoint objects representing the timestamps corresponding to the input data.
     * @param threshold The threshold value above which peaks are detected.
     *
     * @return A DetectionResult structure containing information about the detected peaks.
     */

    DetectionResult result{};
    int peakIndex = 0;

    float peakAmplitude = data.maxCoeff(&peakIndex);

    if (peakAmplitude >= threshold)
    {
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude = peakAmplitude;
        unsigned int microseconds = peakIndex * (1e6 / SAMPLE_RATE);
        auto maxPeakTime = times[0] + std::chrono::microseconds(microseconds);
        result.peakTimes = maxPeakTime;
    }
    return result;
}

DetectionResult ThresholdDetectFD(const Eigen::VectorXcf &data, const std::span<TimePoint> times, const float &threshold, const int &SAMPLE_RATE)
{
    /**
     * @brief Detects peaks in the input data above a specified threshold.
     *
     * This function analyzes the input data to detect peaks that exceed the specified threshold.
     * If a peak is found, its index, amplitude, and corresponding time are recorded in a DetectionResult structure.
     *
     * @param data A reference to an Eigen vector containing the input data.
     * @param times A vector of TimePoint objects representing the timestamps corresponding to the input data.
     * @param threshold The threshold value above which peaks are detected.
     *
     * @return A DetectionResult structure containing information about the detected peaks.
     */

    DetectionResult result{};

    int peakIndex = 0;
    float peakAmplitude = data.array().abs().sum() / data.size(); // data.array().abs2().rowwise().sum(); // data.maxCoeff(&peakIndex);

    if (peakAmplitude >= threshold)
    {
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude = peakAmplitude;
        // unsigned int microseconds = peakIndex * (1e6 / SAMPLE_RATE);
        // auto maxPeakTime = times[0] + std::chrono::microseconds(microseconds);
        // result.peakTimes = maxPeakTime;
    }
    return result;
}

void ProcessSegmentInterleaved(std::span<float> data, Eigen::MatrixXf &channels, const int NUM_CHAN)
{
    /**
     * @brief Processes interleaved data into separate channels. Each channel's data is saved into a corresponding Eigen matrix.
     *
     * @param data A reference to a container of floats containing interleaved data from multiple channels.
     * @param channels A reference to an Eigen matrix to hold the separate channel data. Each row corresponds to a channel.
     * @param NUM_CHAN The number of channels.
     */

    // Calculate the number of samples per channel
    size_t numSamples = data.size() / NUM_CHAN;
#ifdef __ARM_NEON
#else
#endif
    // Ensure the channels matrix has the correct dimensions
    // channels.resize(numSamples, NUM_CHAN);

    // Iterate through the data container and save every NUM_CHANth element into the corresponding row of the channels matrix
    for (unsigned int ch = 0; ch < NUM_CHAN; ++ch)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            channels(i, ch) = data[i * NUM_CHAN + ch];
        }
    }
}