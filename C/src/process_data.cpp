#include <vector>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <eigen3/Eigen/Dense>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include <iomanip> // for output formatting

#include "custom_types.h"
#include "process_data.h"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;



void ConvertData(vector<double>& dataSegment, vector<uint8_t>& dataBytes, unsigned int& DATA_SIZE, unsigned int& HEAD_SIZE) {
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
        double value = static_cast<double>(static_cast<uint16_t>(dataBytes[HEAD_SIZE+i]) << 8) +
                       static_cast<double>(dataBytes[i + HEAD_SIZE + 1]);
        value -= 32768.0;
        dataSegment.push_back(value);
    }
}

DetectionResult ThresholdDetect(arma::Col<double>& data, vector<TimePoint>& times, const double& threshold, const unsigned int& SAMPLE_RATE){
    /**
    * @brief Detects peaks in the input data above a specified threshold.
    *
    * This function analyzes the input data to detect peaks that exceed the specified threshold.
    * If a peak is found, its index, amplitude, and corresponding time are recorded in a DetectionResult structure.
    *
    * @param data A reference to an Armadillo column vector containing the input data.
    * @param times A vector of TimePoint objects representing the timestamps corresponding to the input data.
    * @param threshold The threshold value above which peaks are detected.
    *
    * @return A DetectionResult structure containing information about the detected peaks.
    */
    
    DetectionResult result{};

    int peakIndex = arma::index_max(data);
    double peakAmplitude = arma::max(data);

    if (peakAmplitude >= threshold) {
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude.push_back(peakAmplitude);
        unsigned int microseconds = peakIndex * (1e6 / SAMPLE_RATE);
        auto maxPeakTime = times[0] + std::chrono::microseconds(microseconds);
        result.peakTimes.push_back(maxPeakTime);
    }
    return result;
}

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


void ProcessSegmentInterleaved(vector<double>& data, arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, unsigned int& NUM_CHAN) {
    /**
    * @brief Processes interleaved data into separate channel. Each channel's data is saved into a corresponding Armadillo column vector.
    * 
    * @param data A reference to a vector of doubles containing interleaved data from multiple channels.
    * @param ch1 - ch4 A reference to an Armadillo column vector to store channels 1-4 data.
    */        
    
    // Iterate through the data vector and save every NUM_CHANth element into the arma::Col
    for (size_t i = 0, j = 0; i < data.size(); i += NUM_CHAN, ++j) {
        ch1(j) = data[i];
        ch2(j) = data[i+1];
        ch3(j) = data[i+2];
        ch4(j) = data[i+3];
    }
}
