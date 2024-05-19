#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <queue>
#include <vector>
#include <random>

#include "my_globals.h"
#include "utils.h"
#include <filesystem>
using std::cerr;
using std::endl;
using std::cout;
using std::string;
using std::vector;

void RestartListener(Session& sess){
     /**
     * @brief (Re)starts the udp listener. It closes the existing socket connection and creates a new one.
     * Additionally, it clears the buffer (dataBuffer) and the segment to be processed (dataSegment) 
     * as well as the vector containing the timestamps (dataTimes).
     *
     * @param sess A reference to the Session object representing the listener session.
     */
    
    cout << "restarting listener: " << endl;

    sess.udpSocketLock.lock();
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

    if (bind(sess.datagramSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error binding socket" << endl;
        throw std::runtime_error("Error binding socket");
    }
    sess.udpSocketLock.unlock();

    sess.dataBufferLock.lock();
    ClearQueue(sess.dataBuffer);
    sess.dataBufferLock.unlock();

    sess.dataSegmentLock.lock();
    sess.dataSegment.clear();
    sess.dataSegmentLock.unlock();

    sess.dataTimesLock.lock();
    sess.dataTimes.clear();
    sess.dataTimesLock.unlock();
}

void ProcessFile(Experiment& exp, const string& fileName) {
    /**
    * @brief Processes a configuration file and initializes global variables accordingly.
    *
    * @param fileName The name (or path) of the configuration file to process.
    */
    
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open config file: " << fileName  << endl;
        return;
    }
    inputFile >> exp.HEAD_SIZE >> exp.MICRO_INCR >> exp.NUM_CHAN >> exp.SAMPS_PER_CHANNEL >> exp.BYTES_PER_SAMP;
    exp.DATA_SIZE = exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN * exp.BYTES_PER_SAMP; 
    
    exp.PACKET_SIZE = exp.HEAD_SIZE + exp.DATA_SIZE;
    exp.REQUIRED_BYTES = exp.DATA_SIZE + exp.HEAD_SIZE;
    exp.DATA_BYTES_PER_CHANNEL = exp.SAMPS_PER_CHANNEL * exp.BYTES_PER_SAMP;
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
        cerr << "Error: Unable to open filter file '" << fileName << "'." << endl;
        throw std::runtime_error("Error: Unable to open filter file");
    }
    string line;
    vector<double> filterValues;
    while (std::getline(inputFile, line)){
        vector<double> values;
        std::stringstream stringStream(line);
        string token;
        
        //cout << "Filter values: ";
        while(std::getline(stringStream,token, ',')){
            try {
                double value = std::stod(token);
                filterValues.push_back(value);
                //cout << value << " ";
            } catch(const std::invalid_argument& e) {
                cerr << "Invalid numeric value: " << token << endl;
            }
        }
        //cout << endl;
    }
    arma::Col<double> filter(filterValues);
    return filter;
}

void ClearQueue(std::queue<std::vector<uint8_t>>& q){
    /**
    * @brief This function effectively clears the given queue by swapping it with an
    * empty queue, thus removing all its elements.
    *
    * @param q The queue to be cleared. This queue holds vectors of uint8_t.
    */
    
    std::queue<std::vector<uint8_t>> empty;
    std::swap(q, empty);
}

bool WithProbability(double& probability){
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
