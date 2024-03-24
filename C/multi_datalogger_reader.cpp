/*

This is a C++ version of Python/multi_datalogger_reader.py

Compile code manually:
g++ multi_datalogger_reader.cpp process_data.cpp -o exe -larmadillo

Execute (datalogger simulator):
./exe 192.168.7.2 1045 1240

Execute (datalogger):
./exe 192.168.100.220 50000 1240

*/


#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>
#include <fstream>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <chrono>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iomanip> //put_time
#include <ctime>
#include <cstdint>

#include "process_data.h"
#include "my_globals.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::cerr;
using std::vector;
using std::ifstream;
using std::lock_guard;
using std::mutex;
using std::stoi;
using std::thread;

int HEAD_SIZE;                                 //packet head size (bytes)
double MICRO_INCR;                             // time between packets
int NUM_CHAN;                                  //number of channels per packet
int SAMPS_PER_CHANNEL;                         //samples per packet per channel, for 2 channels, this value is 5*62  = 310
int BYTES_PER_SAMP;                            //bytes per sample

int DATA_SIZE;                                 //packet data size (bytes)
int PACKET_SIZE;                               //packet size (bytes)
int REQUIRED_BYTES;
int DATA_BYTES_PER_CHANNEL;                    //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
int NUM_PACKS_DETECT;
const float TIME_WINDOW = 0.5;                 // fraction of a second to consider  
const string OUTPUT_FILE = "clicks_data.txt";
int packetCounter = 0;
int DATA_SEGMENT_LENGTH;

// Macros for preprocessor directives
//#define SPEED_TEST
//#define DEBUG_PRINT_UNPACKED

std::queue<vector<uint8_t>> dataBuffer;
std::mutex bufferMutex;                       // For thread-safe buffer access

void ProcessFile(const string& fileName) {
    ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open file '" << fileName << "'." << endl;
        return;
    }
    inputFile >> HEAD_SIZE >> MICRO_INCR >> NUM_CHAN >> SAMPS_PER_CHANNEL >> BYTES_PER_SAMP;
    DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP;       //packet data size (bytes) = 1240
    PACKET_SIZE = HEAD_SIZE + DATA_SIZE;                             //packet size (bytes) = 1252
    REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
}

void UdpListener(int datagramSocket) {
    
    struct sockaddr_in addr;
    socklen_t addrLength = sizeof(addr);
    int bytesReceived;
    vector<uint8_t> dataB(PACKET_SIZE);
    #ifdef SPEED_TEST
        // Record the start time
        auto startTime = std::chrono::high_resolution_clock::now();
        decltype(startTime) endTime;
        long double duration;
        bytesReceived = recvfrom(datagramSocket, dataB.data(), PACKET_SIZE, 0, (struct sockaddr*)&addr, &addrLength);
    #endif
    while (true) {
        //vector<uint8_t> dataB(PACKET_SIZE);
        #ifndef SPEED_TEST
            int bytesReceived = recvfrom(datagramSocket, dataB.data(), PACKET_SIZE, 0, (struct sockaddr*)&addr, &addrLength);
        #endif
        if (bytesReceived == -1) {
            cerr << "Error in recvfrom" << endl;
            continue;
        }
        
        //dataB.resize(bytesReceived); // Adjust size based on actual bytes received
        lock_guard<mutex> lock(bufferMutex);
        dataBuffer.push(dataB);
        packetCounter += 1;
        if (packetCounter % 500 == 0) {
            cout << "Num packets received is " << packetCounter << endl;
            
            #ifdef SPEED_TEST
                endTime = std::chrono::high_resolution_clock::now();
                // Calculate the duration in seconds
                duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1e6;
                cout << "Processed 500 packets in " << duration << " !!!!!!!!!!!!!!!!!!" << endl;
                startTime = std::chrono::high_resolution_clock::now();
            #endif
        }
    }
}

void DataProcessor() {
    if (MICRO_INCR == 1240){
        void(*processFncPtr)(std::vector<int16_t>&, std::vector<TimePoint>&, const std::string&) = ProcessSegment1240;
    }

    while (true) {
        vector<int16_t> dataSegment;
        //arma::vec dataSegment;
        vector<std::chrono::system_clock::time_point> timestamps;
        while (dataSegment.size() < DATA_SEGMENT_LENGTH) {
            //lock_guard<mutex> lock(bufferMutex);
            if (!dataBuffer.empty()) {
                lock_guard<mutex> lock(bufferMutex);
                vector<uint8_t> dataB = dataBuffer.front();
                //vector<uint16_t> dataI = (uint16_t)dataBuffer.front();
                dataBuffer.pop();
                
                //auto duration = timestamp;
                std::tm timeStruct;
                timeStruct.tm_year = (int)dataB[0] + 2000 - 1900; // Offset for year since 2000.. tm_year is years since 1900 
                timeStruct.tm_mon = dataB[1] - 1;   // Months are 0-indexed
                timeStruct.tm_mday = dataB[2];
                timeStruct.tm_hour = dataB[3];
                timeStruct.tm_min = dataB[4];
                timeStruct.tm_sec = dataB[5];

                uint32_t microSec = (static_cast<uint32_t>(dataB[6]) << 24) |
                         (static_cast<uint32_t>(dataB[7]) << 16) |
                         (static_cast<uint32_t>(dataB[8]) << 8) |
                         static_cast<uint32_t>(dataB[9]);
                
                std::chrono::time_point<std::chrono::system_clock> specificTime = std::chrono::system_clock::from_time_t(mktime(&timeStruct));
                specificTime += std::chrono::microseconds(microSec);
                
                // Convert byte data to unsigned 16bit ints
                vector<int16_t> data;
                for (size_t i = 0; i < DATA_SIZE; i += 2) {
                    int16_t value = static_cast<int16_t>(dataB[12+i]) +
                                   (static_cast<int16_t>(dataB[i + 13]) << 8);
                    data.push_back(value - 32768);
                }
                
                // Append data and timestamp
                timestamps.push_back(specificTime);
                //timestamps.push_back(std::mktime(&timeStruct));
                dataSegment.insert(dataSegment.end(), data.begin(), data.end());
            }
        }
        // print first 12 values recieved
        #ifdef DEBUG_PRINT_UNPACKED
            cout << "Inside DataProcessor() " << endl;
            for (size_t j = 0; j < 50; j++){
                cout << dataSegment[j] << " ";
            }
            cout << endl;
        #endif
        cout << "dataSegment Size: " << dataSegment.size() << endl;
        ProcessSegment1240(dataSegment, timestamps, OUTPUT_FILE);  // Replace with your processing code
    }
}


int main(int argc, char *argv[]){
    
    string UDP_IP = argv[1];                                         // IP address of data logger or simulator
    int UDP_PORT = stoi(argv[2]);                                     // Port to listen for UDP packets
    int firmwareVersion = stoi(argv[3]);
    printf("Listening to IP address %s and port %d \n", UDP_IP.c_str(),UDP_PORT);

    //import variables according to firmware version specified
    cout << "Assuming firmware version: " << firmwareVersion << endl;
    if (firmwareVersion == 1550){
        cerr << "functionality for firmware version 1550 is undefined" << endl;
        return 1;
    }
    else if (firmwareVersion == 1240){
        ProcessFile("1240_config.txt");
    }
    else{
        cerr << "ERROR: Unknown firmware version" << endl;
        return 1;
    }

    
    NUM_PACKS_DETECT = TIME_WINDOW * 100000 / SAMPS_PER_CHANNEL;  // NEED TO ROUND THIS  the number of data packets that are needed to perform energy detection 
    DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL; 

    cout << "HEAD_SIZE: " << HEAD_SIZE << endl; 
    cout << "SAMPS_PER_CHAN: " << SAMPS_PER_CHANNEL << endl;
    cout << "BYTES_PER_SAMP: " << BYTES_PER_SAMP << endl;
    cout << "Bytes per packet:       " <<  REQUIRED_BYTES << endl;
    cout << "Time between packets:   " <<  MICRO_INCR << endl;
    cout << "Number of channels:     " <<  NUM_CHAN << endl;
    cout << "Data bytes per channel: " <<  DATA_BYTES_PER_CHANNEL << endl;
    cout << "Detecting over a time window of " << TIME_WINDOW << " seconds, using " << NUM_PACKS_DETECT <<  " packets" << endl;

        
    int datagramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (datagramSocket == -1) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(UDP_IP.c_str());
    serverAddr.sin_port = htons(UDP_PORT);

    if (bind(datagramSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error binding socket" << endl;
        return 1;
    }

    thread udpThread(UdpListener, datagramSocket);
    thread processorThread(DataProcessor);

    udpThread.join();
    processorThread.join();

    //close(datagramSocket);
    return 0;
}
