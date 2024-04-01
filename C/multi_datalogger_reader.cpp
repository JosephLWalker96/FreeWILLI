/*

This is a C++ version of Python/multi_datalogger_reader.py

Compile code manually:
g++ multi_datalogger_reader.cpp process_data.cpp -o listen.exe -larmadillo

Execute (datalogger simulator):
./listen.exe 192.168.7.2 1045 1240

Execute (datalogger):
./listen.exe 192.168.100.220 50000 1240


TO DO:
  1) Check for data and time glitches
  2) Restart threads if glitches occur
  3) Decode bytes from buffer once
  3) Refactor code

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
    
    #ifdef SPEED_TEST // Record the start time
        auto startTime = std::chrono::high_resolution_clock::now();
        decltype(startTime) endTime;
        long double duration;
        bytesReceived = recvfrom(datagramSocket, dataB.data(), PACKET_SIZE, 0, (struct sockaddr*)&addr, &addrLength);
    #endif
    
    while (true) {
        
        #ifndef SPEED_TEST
            int bytesReceived = recvfrom(datagramSocket, dataB.data(), PACKET_SIZE, 0, (struct sockaddr*)&addr, &addrLength);
        #endif
        
        if (bytesReceived == -1) {
            cerr << "Error in recvfrom" << endl;
            continue;
        }
        //dataB.resize(bytesReceived); // Adjust size based on actual bytes received
        std::lock_guard<std::mutex> lock(bufferMutex);
        dataBuffer.push(dataB);
        packetCounter += 1;
        if (packetCounter % 500 == 0) {
            cout << "Num packets received is " << packetCounter << endl;
            
            #ifdef SPEED_TEST // Calculate the duration in seconds
                endTime = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1e6;
                cout << "Processed 500 packets in " << duration << " seconds" << endl;
                startTime = std::chrono::high_resolution_clock::now();
            #endif

        }
    }
}

void DataProcessor(void (*ProcessingFunction)(vector<int16_t>&, vector<TimePoint>&, const string&)) {
    while (true) {
        vector<int16_t> dataSegment;
        vector<std::chrono::system_clock::time_point> timestamps;
        while (dataSegment.size() < DATA_SEGMENT_LENGTH) {
            if (!dataBuffer.empty()) {
                std::lock_guard<std::mutex> lock(bufferMutex);
                vector<uint8_t> dataB = dataBuffer.front();
                dataBuffer.pop();
                
                std::tm timeStruct;
                timeStruct.tm_year = (int)dataB[0] + 2000 - 1900;     // Offset for year since 2000.. tm_year is years since 1900 
                timeStruct.tm_mon = dataB[1] - 1;                     // Months are 0-indexed
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
                
                // Convert byte data to signed 16bit ints
                vector<int16_t> data;
                for (size_t i = 0; i < DATA_SIZE; i += 2) {
                    int16_t value = static_cast<int16_t>(dataB[12+i]) +
                                   (static_cast<int16_t>(dataB[i + 13]) << 8);
                    data.push_back(value - 32768);
                }
                timestamps.push_back(specificTime);
                dataSegment.insert(dataSegment.end(), data.begin(), data.end());
            }
        }
        
        #ifdef PRINT_DATA_PROCESSOR // print first few values in dataSegment
            cout << "Inside DataProcessor() " << endl;
            cout << "dataSegment Size: " << dataSegment.size() << " should be same as " << NUM_PACKS_DETECT * (DATA_SIZE / 2) << endl;
            for (size_t j = 0; j < 50; j++){
                cout << dataSegment[j] << " ";
            }
            cout << endl;
        #endif
        ProcessingFunction(dataSegment, timestamps, OUTPUT_FILE);
    }
}


int main(int argc, char *argv[]){
    
    string UDP_IP = argv[1];                                         // IP address of data logger or simulator
    int UDP_PORT = std::stoi(argv[2]);                                     // Port to listen for UDP packets
    int firmwareVersion = std::stoi(argv[3]);
    printf("Listening to IP address %s and port %d \n", UDP_IP.c_str(),UDP_PORT);

    //import variables according to firmware version specified
    cout << "Assuming firmware version: " << firmwareVersion << endl;
    void(*ProcessFncPtr)(vector<int16_t>&, vector<TimePoint>&, const string&) = nullptr;
    if (firmwareVersion == 1550){
        ProcessFile("1550_config.txt");
        ProcessFncPtr = ProcessSegment1550;
    }
    else if (firmwareVersion == 1240){
        ProcessFile("1240_config.txt");
        ProcessFncPtr = ProcessSegment1240;
    }
    else{
        cerr << "ERROR: Unknown firmware version" << endl;
        return 1;
    }
    
    NUM_PACKS_DETECT = (int)(TIME_WINDOW * 100000 / SAMPS_PER_CHANNEL);  // NEED TO ROUND THIS  the number of data packets that are needed to perform energy detection 
    DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN; 

    cout << "HEAD_SIZE: " << HEAD_SIZE << endl; 
    cout << "SAMPS_PER_CHAN: " << SAMPS_PER_CHANNEL << endl;
    cout << "BYTES_PER_SAMP: " << BYTES_PER_SAMP << endl;
    cout << "Bytes per packet:       " <<  REQUIRED_BYTES << endl;
    cout << "Time between packets:   " <<  MICRO_INCR << endl;
    cout << "Number of channels:     " <<  NUM_CHAN << endl;
    cout << "Data bytes per channel: " <<  DATA_BYTES_PER_CHANNEL << endl;
    cout << "Detecting over a time window of " << TIME_WINDOW << " seconds, using " << NUM_PACKS_DETECT <<  " packets" << endl;

    std::string output_file = "Ccode_clicks.txt"; // Change to your desired file name
    
    // Open the file in write mode and clear its contents if it exists, create a new file otherwise
    std::ofstream file(output_file, std::ofstream::out | std::ofstream::trunc);

    if (file.is_open()) {
        // Write header row (optional)
        file << "Timestamp (microseconds)" << std::setw(20) << "Peak Amplitude" << endl;
        file.close();
        cout << "File created and cleared: " << output_file << endl;
    } else {
        cerr << "Error: Unable to open file for writing: " << output_file << endl;
        return 1; // Indicate error
    }

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

    std::thread udpThread(UdpListener, datagramSocket);
    std::thread processorThread(DataProcessor, ProcessFncPtr);

    udpThread.join();
    processorThread.join();

    return 0;
}
