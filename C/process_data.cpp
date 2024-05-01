#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
//#include <eigen3/Eigen/Dense>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include <iomanip> // for output formatting

#include "my_globals.h"
#include "process_data.h"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;

void PrintTimes(const vector<TimePoint>& timestamps){
    for (auto& timestamp : timestamps){
        std::time_t timeRepresentation = std::chrono::system_clock::to_time_t(timestamp);
        std::tm timeData = *std::localtime(&timeRepresentation); 
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count() % 1000000; 
        
        cout << "Timestamp: ";
        cout << timeData.tm_year + 1900 << '-'
            << timeData.tm_mon + 1 << '-'
            << timeData.tm_mday << ' '
            << timeData.tm_hour << ':'
            << timeData.tm_min << ':'
            << timeData.tm_sec << ':'
            << microsecs << endl;
    }
}



DetectionResult ThresholdDetect(arma::Col<double>& data, vector<TimePoint>& times, int threshold){
    DetectionResult result;

    int peakIndex = arma::index_max(data);
    double peakAmplitude = arma::max(data);

    if (peakAmplitude > threshold){
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude.push_back(peakAmplitude);
        long long microseconds = (long long)peakIndex / SAMPLE_RATE * 1e6;
        std::chrono::time_point<std::chrono::system_clock> maxPeakTime = times[0] + std::chrono::microseconds(microseconds);
        result.peakTimes.push_back(maxPeakTime);
    }

    return result;
}

void ProcessSegment(arma::Col<double>& data, vector<TimePoint>& times, const string& outputFile) {
    arma::Col<double> dataAbsolute = arma::abs(arma::conv_to<arma::Col<double>>::from(data));
    
    #ifdef PRINT_PROCESS_SEGMENT
        cout << "Inside ProcessSegment() " << endl;
        cout << "dataAbsolute.size() " << dataAbsolute.size() << endl;
        cout << "first few elements in dataAbsolute " << endl;
        for (size_t j = 0; j < 12; j++){
            cout << dataAbsolute[j] << " ";
        }
        cout << endl;
    #endif
   
    // define the filter for click regions
    arma::Col<double> smoothFilter(1.0, 256, arma::fill::ones); // Filter of size 256 with all ones
    smoothFilter /= 256.0;
    
    #ifdef PRINT_PROCESS_SEGMENT
        cout << "first few elements in smoothFilter " << endl;
        for (size_t j = 0; j < 12; j++){
            cout << smoothFilter[j] << " ";
        }
        cout << endl;
    #endif

    // Convolve abs_data with the filter
    arma::Col<double> dataSmoothed = arma::conv(dataAbsolute, smoothFilter, "same");

    #ifdef PRINT_PROCESS_SEGMENT
        cout << "first few elements in dataSmoothed: " << endl;
        cout << "dataSmoothed.size() " << dataSmoothed.size() << endl;
        for (size_t j = 0; j < 12; j++){
            cout << dataSmoothed[j] << " ";
        }
        cout << endl;
        cout << "last few elements in dataSmoothed: "<< endl;
        for (size_t j = dataSmoothed.size() - 12; j < dataSmoothed.size(); j++){
            cout << dataSmoothed[j] << " ";
        }
        cout << endl;
    #endif
    
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
        //WriteClicks(clickPeakAmps,timestamps, "Ccode_clicks.txt");
    }
}

void ProcessSegmentStacked(vector<double>& data, vector<TimePoint>& times, const string& outputFile) {
    
    #ifdef PRINT_PROCESS_SEGMENT_1240
        cout << "Inside ProcessSegment1240() " << endl;
        cout << "data.size(): " << data.size() << endl;
    #endif
    
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


void ProcessSegmentInterleaved(vector<double>& data, vector<TimePoint>& times, const string& outputFile, arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4) {
    
    size_t channelSize = data.size() / NUM_CHAN;
    ch1.set_size(channelSize);                    // Reserve space in the arma::Col (optional but can improve performance)
    ch2.set_size(channelSize);                    // Reserve space in the arma::Col (optional but can improve performance)
    ch3.set_size(channelSize);                    // Reserve space in the arma::Col (optional but can improve performance)
    ch4.set_size(channelSize);                    // Reserve space in the arma::Col (optional but can improve performance)

    // Iterate through the data vector and save every 4th element into the arma::Col
    for (size_t i = 0, j = 0; i < data.size(); i += NUM_CHAN, ++j) {
        ch1(j) = data[i];  // Saving every 4th element into ch1
        ch2(j) = data[i+1]; // Saving every 4th element into ch1
        ch3(j) = data[i+2]; // Saving every 4th element into ch1
        ch4(j) = data[i+3]; // Saving every 4th element into ch1
    }
    #ifdef PRINT_PROCESS_SEGMENT_1550
        cout << "Inside ProcessSegment1550() " << endl;
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
        
    //ProcessSegment(ch1, times, outputFile);

}

void WritePulseAmplitudes(const vector<double>& clickPeakAmps,
                 const vector<TimePoint>& timestamps,
                 const string& filename) {
  std::ofstream outfile(filename, std::ios::app);
  if (outfile.is_open()) {
    // Check if vectors have the same size
    if (clickPeakAmps.size() != timestamps.size()) {
      cerr << "Error: Click amplitude and timestamp vectors have different sizes." << endl;
      return;
    }

    // Write data rows
    for (size_t i = 0; i < clickPeakAmps.size(); ++i) {
      auto time_point = timestamps[i];
      auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
      outfile << time_since_epoch.count() << std::setw(20) << clickPeakAmps[i] << endl;
    }

    outfile.close();
  } else {
    cerr << "Error: Could not open file " << filename << endl;
  }
}
