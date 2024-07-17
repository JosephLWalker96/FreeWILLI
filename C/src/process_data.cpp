#include <vector>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
//#include <eigen3/Eigen/Dense>
#include <iostream>
#include <iomanip> // for output formatting
#include <ctime>

#include "custom_types.h"
#include "process_data.h"
#include "utils.h"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;



void ConvertData(vector<float>& dataSegment, vector<uint8_t>& dataBytes, unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE) {
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
    
     for (size_t i = 0; i < DATA_SIZE; i += 2) {
        float value = static_cast<float>(static_cast<uint16_t>(dataBytes[HEAD_SIZE+i]) << 8) +
                       static_cast<float>(dataBytes[i + HEAD_SIZE + 1]);
        value -= 32768.0;
        dataSegment.push_back(value);
    }
}


void GenerateTimestamps(std::vector<TimePoint>& dataTimes, vector<uint8_t>& dataBytes,unsigned int MICRO_INCR, bool& previousTimeSet, std::chrono::time_point<std::chrono::system_clock>& previousTime, string& detectionOutputFile, string& tdoaOutputFile, string& doaOutputFile) {

    std::tm timeStruct{};                                     // Initialize a std::tm structure to hold the date and time components
    timeStruct.tm_year = (int)dataBytes[0] + 2000 - 1900;     // Offset for year since 2000.. tm_year is years since 1900 
    timeStruct.tm_mon = (int)dataBytes[1] - 1;                // Months are 0-indexed
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
    if (timeResult == std::time_t(-1)) {
        throw std::runtime_error("Error: failure in mktime \n");
    }

    auto currentTime = std::chrono::system_clock::from_time_t(timeResult);  // convert std::time_t to std::chrono::system_clock::time_point
    currentTime += std::chrono::microseconds(microSec);
    dataTimes.push_back(currentTime);

    // Calculate the elapsed time in microseconds since the previous time point
    auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count();

                

    // Check if the previous time was set and if the elapsed time is not equal to the expected increment
    if (previousTimeSet && (elapsedTime != MICRO_INCR)){
        std::stringstream msg; // compose message to dispatch
        msg <<  "Error: Time not incremented by " <<  MICRO_INCR << " " << elapsedTime << endl;
        throw std::runtime_error(msg.str());
    }


    previousTime = currentTime;
    previousTimeSet = true;
    
    if ((detectionOutputFile).empty()){
        string feature = "detection";
        InitiateOutputFile(detectionOutputFile, timeStruct, microSec, feature);
        feature = "tdoa";
        InitiateOutputFile(tdoaOutputFile, timeStruct, microSec, feature);
        feature = "doa";
        InitiateOutputFile(doaOutputFile, timeStruct, microSec, feature);
    }
}



DetectionResult ThresholdDetect(const Eigen::VectorXf& data, const std::vector<TimePoint>& times, const double& threshold, const unsigned int& SAMPLE_RATE) {
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

    if (peakAmplitude >= threshold) {
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude = peakAmplitude;
        unsigned int microseconds = peakIndex * (1e6 / SAMPLE_RATE);
        auto maxPeakTime = times[0] + std::chrono::microseconds(microseconds);
        result.peakTimes = maxPeakTime;
    }
    return result;
}

/*
void ProcessSegment(arma::Col<double>& data, vector<TimePoint>& times, const string& outputFile) {
    arma::Col<double> dataAbsolute = arma::abs(data);
    
    // define the filter for click regions
    arma::Col<double> smoothFilter(1.0, 256, arma::fill::ones); // Filter of size 256 with all ones
    smoothFilter /= 256.0;
    
    // Convolve abs_data with the filter
    arma::Col<double> dataSmoothed = arma::conv(dataAbsolute, smoothFilter, "same");
    
    // Thresholding low amplitude values
    dataSmoothed.elem(arma::find(dataSmoothed < 80.0)).fill(0.0);
    
    // Create a mask for click regions (ones filter)
    arma::Col<double> segmentationFilter(1.0,256,arma::fill::ones);
    arma::Col<double> dataMasked = arma::conv(dataSmoothed, segmentationFilter, "same");
    dataMasked.elem(arma::find(dataMasked > 0.0)).fill(1.0);
    

    // Find click region indices based on differences
    arma::Col<double> diff = arma::diff(dataMasked);
    arma::uvec segmentBoundary = arma::find(diff != 0.0);

    if (segmentBoundary.n_elem > 0){
        vector<TimePoint> timestamps;
        segmentBoundary.insert_rows(0, 1);
        segmentBoundary(0) = 0;
        vector<double> clickPeakAmps;
        for (size_t i = 0; i < segmentBoundary.size() - 1; ++i) {
            int start_point = segmentBoundary[i];
            int end_point = segmentBoundary[i + 1];

            // Check if there's any non-zero value in the region
            if (arma::any(dataSmoothed.subvec(start_point, end_point - 1) != 0.0)) {
                //arma::vec region_data = data.subvec(start_point, end_point - 1);
                arma::Col<double> clickSegment = data.subvec(start_point, end_point - 1);

                // Calculate the index (avoid potential iterator subtraction issues)
                int peakIndex = arma::index_max(clickSegment);
                double peakAmplitude = arma::max(clickSegment);
                
                // Calculate click time based on sampling rate and peak index
                long long seconds = ((start_point + peakIndex) * 10); // divide (...) by 1e5 then times 1e6
                
                std::chrono::time_point<std::chrono::system_clock> peakAmplitudeTime = times[0] + std::chrono::microseconds(seconds);
                clickPeakAmps.push_back(peakAmplitude);
                timestamps.push_back(peakAmplitudeTime);
            }
        }
    }
}
*/

/*
void ProcessSegmentStacked(vector<double>& data, vector<TimePoint>& times, const string& outputFile, unsigned int& NUM_CHAN, unsigned int& SAMPS_PER_CHANNEL, unsigned int& NUM_PACKS_DETECT) {
    //
    // THIS FUNCTION IS DEPRICATED
    //

    arma::Col<int16_t> ch1;
    int channelSize = data.size() / NUM_CHAN;
    ch1.set_size(channelSize);                    // Reserve space in the arma::Col (optional but can improve performance) 
    
    // Iterate over the data in steps of 100
    for (size_t i = 0; i < NUM_PACKS_DETECT; ++i) {
        // Calculate the starting index of the current segment
        size_t segmentStart = i * (NUM_CHAN * SAMPS_PER_CHANNEL);

        // Extract SAMPS_PER_CHANNEL consecutive elements from the current segment
        for (size_t j = 0; j < SAMPS_PER_CHANNEL; ++j) {
            ch1(i * SAMPS_PER_CHANNEL + j) = data[segmentStart + j];
        }
    }
    #ifdef PRINT_PROCESS_SEGMENT_1240
        cout << "Ch1 size " << ch1.size() << endl;
        cout << "First few elements in ch1 " << ch1.size() << endl;
        for (size_t j = 0; j < 12; j++){
            cout << ch1[j] << " ";
        }
        cout << endl;
        cout << "Last few elements in ch1 " << ch1.size() << endl;
        for (size_t j = ch1.size() - 12; j < ch1.size(); j++){
            cout << ch1[j] << " ";
        }
        cout << endl;
    #endif

    //ProcessSegment(ch1, times, outputFile);  // Use memptr to access raw data
}

*/
void ProcessSegmentInterleaved(std::vector<float>& data, Eigen::MatrixXf& channels, unsigned int NUM_CHAN) {
    /**
    * @brief Processes interleaved data into separate channels. Each channel's data is saved into a corresponding Eigen matrix.
    * 
    * @param data A reference to a vector of floats containing interleaved data from multiple channels.
    * @param channels A reference to an Eigen matrix to hold the separate channel data. Each row corresponds to a channel.
    * @param NUM_CHAN The number of channels.
    */
    
    // Calculate the number of samples per channel
    size_t numSamples = data.size() / NUM_CHAN;
    
    // Ensure the channels matrix has the correct dimensions
    //channels.resize(numSamples, NUM_CHAN);

    // Iterate through the data vector and save every NUM_CHANth element into the corresponding row of the channels matrix
    for (unsigned int ch = 0; ch < NUM_CHAN; ++ch) {
        for (size_t i = 0; i < numSamples; ++i) {
            channels(i, ch) = data[i * NUM_CHAN + ch];
        }
    }
}
