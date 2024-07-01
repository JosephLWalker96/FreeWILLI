#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <queue>
#include <vector>
#include <random>
#include <iomanip> //std::setw
#include "custom_types.h"
#include "utils.h"

using std::cerr;
using std::endl;
using std::cout;
using std::string;
using std::vector;
using TimePoint = std::chrono::system_clock::time_point;

void PrintTimes(const vector<TimePoint>& timestamps) {
    /**
    * @brief Prints the timestamps provided in the input vector.
    *
    * This function prints the timestamps provided in the input vector
    * in the format "YYYY-MM-DD HH:MM:SS:Microseconds".
    *
    * @param timestamps A vector of TimePoint objects representing the timestamps to be printed.
    *                   TimePoint is a type alias for a time point based on std::chrono::system_clock.
    */
    
    for (auto& timestamp : timestamps) {
        std::time_t timeRepresentation = std::chrono::system_clock::to_time_t(timestamp);
        std::tm timeData = *std::localtime(&timeRepresentation); 
        auto microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count() % 1000000; 
        
        cout << "Timestamp: ";
        cout << timeData.tm_year + 1900 << '-'
            << timeData.tm_mon + 1 << '-'
            << timeData.tm_mday << ' '
            << timeData.tm_hour << ':'
            << timeData.tm_min << ':'
            << timeData.tm_sec << ':'
            << microSeconds << endl;
    }
}

void RestartListener(Session& sess){
     /**
     * @brief (Re)starts the udp listener. It closes the existing socket connection and creates a new one.
     * Additionally, it clears the buffer (dataBuffer) and the segment to be processed (dataSegment) 
     * as well as the vector containing the timestamps (dataTimes).
     *
     * @param sess A reference to the Session object representing the listener session.
     */
    
    cout << "restarting listener: " << endl;

    if (close(sess.datagramSocket) == -1) {
        std::cerr << "Failed to close socket" << std::endl;
        throw std::runtime_error("Failed to close socket");
    }


    sess.datagramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sess.datagramSocket == -1) {
        cerr << "Error creating socket" << endl;
        throw std::runtime_error("Error creating socket");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(sess.UDP_IP.c_str());
    serverAddr.sin_port = htons(sess.UDP_PORT);

    if (sess.UDP_IP == "192.168.100.220"){
        cout << "Sending wake up data to IP address to data logger " << endl;
        const char* m1 = "Open";
        unsigned char m2[96] = {0};
        unsigned char message[100];
        std::memcpy(message, m1,4);
        std::memcpy(message + 4, m2, 96);
        if (sendto(sess.datagramSocket, message, sizeof(message), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
            cerr << "Error sending data" << endl;
            throw std::runtime_error("Error sending data");
        }
    }
    else if (bind(sess.datagramSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error binding socket" << endl;
        throw std::runtime_error("Error binding socket");
    }
    ClearQueue(sess.dataBuffer);
    sess.dataSegment.clear();
    sess.dataTimes.clear();
}

int ProcessFile(Experiment& exp, const string fileName) {
    /**
    * @brief Processes a configuration file and initializes global variables accordingly.
    *
    * @param fileName The name (or path) of the configuration file to process.
    */
    
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open config file: " << fileName  << endl;
        return 1;
    }
    inputFile >> exp.HEAD_SIZE >> exp.MICRO_INCR >> exp.NUM_CHAN >> exp.SAMPS_PER_CHANNEL >> exp.BYTES_PER_SAMP;
    exp.DATA_SIZE = exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN * exp.BYTES_PER_SAMP; 
    
    exp.PACKET_SIZE = exp.HEAD_SIZE + exp.DATA_SIZE;
    exp.REQUIRED_BYTES = exp.DATA_SIZE + exp.HEAD_SIZE;
    exp.DATA_BYTES_PER_CHANNEL = exp.SAMPS_PER_CHANNEL * exp.BYTES_PER_SAMP;
    return 0;
}

void InitiateOutputFile(string& outputFile, std::tm& timeStruct, int64_t microSec, string& feature){

    outputFile = "deployment_files/"  + std::to_string(timeStruct.tm_year + 1900) + '-' + std::to_string(timeStruct.tm_mon + 1) + '-' + 
                     std::to_string(timeStruct.tm_mday) + '-' + std::to_string(timeStruct.tm_hour) + '-' + std::to_string(timeStruct.tm_min) + '-' +
                     std::to_string(timeStruct.tm_sec) + '-' + std::to_string(microSec) + '_' + feature;
    
    cout << "created and writting to file: " << outputFile << endl;
    
    // Open the file in write mode and clear its contents if it exists, create a new file otherwise
    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::trunc);
    if (file.is_open()) {
        file << "Timestamp (microseconds)" << std::setw(20) << "Peak Amplitude" << endl;
        file.close();
    } 
    else {
        cerr << "Error: Unable to open file for writing: " << outputFile << endl;
        throw std::runtime_error("Error: Unable to open file for writing: ");
    }
}

arma::Col<double> ReadFIRFilterFile(const string& fileName) {
     /**
     * @brief Reads a file containing the FIR filter taps and returns the values as an Armadillo column vector.
     *
     * @param fileName The name (or path) of the file to read.
     *                 The file should contain comma-separated numeric values on each line.
     * @return arma::Col<double> An Armadillo column vector containing the numeric values 
     *                           read from the file.
     * @throws std::runtime_error If the file cannot be opened.
     */


    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Unable to open filter file '" << fileName << "'." << endl;
        throw std::ios_base::failure(msg.str());
    }
    string line;
    vector<double> filterValues;
    while (std::getline(inputFile, line)){
        vector<double> values;
        std::stringstream stringStream(line);
        string token;
        
        while(std::getline(stringStream,token, ',')){
            try {
                double value = std::stod(token);
                filterValues.push_back(value);
                //cout << value << " ";
            } catch(const std::invalid_argument& e) {
                cerr << "Invalid numeric value: " << token << endl;
            }
        }
    }
    arma::Col<double> filter(filterValues);
    return filter;
}

void ClearQueue(std::queue<std::vector<uint8_t>>& fullQueue) {
    /**
    * @brief This function effectively clears the given queue by swapping it with an
    * empty queue, thus removing all its elements.
    *
    * @param q The queue to be cleared. This queue holds vectors of uint8_t.
    */
    
    std::queue<std::vector<uint8_t>> empty;
    std::swap(fullQueue, empty);
}

bool WithProbability(double probability){
    /**
    * @brief Generates a boolean value based on the given probability.
    * This fucntion is used for testing.
    * 
    * @param probability The probability (between 0.0 and 1.0) of returning true.
    * @return bool Returns true with the specified probability, otherwise returns false.
    */

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double randomValue = dis(gen);
    return randomValue < probability;
}
void WritePulseAmplitudes(const vector<double>& clickPeakAmps, const vector<TimePoint>& timestamps, const string& filename) {
    /**
    * @brief Writes pulse amplitudes and corresponding timestamps to a file.
    *
    * @param clickPeakAmps A reference to a vector of doubles containing pulse amplitudes.
    * @param timestamps A reference to a vector of TimePoint objects representing the timestamps corresponding to the pulse amplitudes.
    * @param filename A string specifying the output file path or name.
    */
    

    //cout << "clickPeakAmps.size(): " << clickPeakAmps.size() << endl;
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
    } 
    else {
        cerr << "Error: Could not open file " << filename << endl;
    }
}


void WriteArray(const arma::Col<double>& array, const vector<TimePoint>& timestamps, const string& filename) {
    /**
    * @brief Writes pulse amplitudes and corresponding timestamps to a file.
    *
    * @param clickPeakAmps A reference to a vector of doubles containing pulse amplitudes.
    * @param timestamps A reference to a vector of TimePoint objects representing the timestamps corresponding to the pulse amplitudes.
    * @param filename A string specifying the output file path or name.
    */
    

    //cout << "clickPeakAmps.size(): " << clickPeakAmps.size() << endl;
    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open()) {
        // Check if vectors have the same size
        /*
        if (clickPeakAmps.size() != timestamps.size()) {
            cerr << "Error: Click amplitude and timestamp vectors have different sizes." << endl;
            return;
        }
        */
        // Write data rows
        auto time_point = timestamps[0];
        auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
        outfile << time_since_epoch.count() << std::setw(20);
        for (size_t i = 0; i < array.n_elem; ++i) {
            outfile << array[i] << " ";
        }
        outfile << endl;

        outfile.close();
    } 
    else {
        cerr << "Error: Could not open file " << filename << endl;
    }
}

void WriteDataToCerr(vector<TimePoint>& dataTimes,vector<double>& dataSegment, vector<vector<uint8_t>>& dataBytesSaved){
    cerr << "Errored Timestamps: " << endl;
    for (const auto timestamp : dataTimes){
        auto convertedTime = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count(); 
        cerr << convertedTime << endl;
    }
    cerr << endl;
    cerr << "Errored decoded data: " << endl;
    for (const auto data : dataSegment)
        cerr << data << " ";
    cerr << endl;
    cerr << "Errored bytes of last packet: " << endl;
    for (const auto byteArray : dataBytesSaved){
        for (const auto data : byteArray){
            cerr << std::setw(2) << std::setfill('0') << static_cast<int>(data);
        }
        cerr << endl;
    }
    cerr << endl;


}
