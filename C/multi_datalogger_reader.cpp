/*

This is a C++ version of Python/multi_datalogger_reader.py

Compile code manually:
g++ multi_datalogger_reader.cpp process_data.cpp utils.cpp -o listen -larmadillo

Execute (datalogger simulator):
./listen 192.168.7.2 1045 1240

Execute (datalogger):
./listen.exe 192.168.100.220 50000 1240


TO DO:
  1) Check for data and time glitches
  2) Restart threads if glitches occur
  3) Decode bytes from buffer once
  4) Refactor code
  5) handle thread exceptions


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
#include <armadillo>

#include "process_data.h"
#include "utils.h"
#include "my_globals.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::cerr;
using std::vector;
using std::ifstream;

int HEAD_SIZE;                                 //packet head size (bytes)
int NUM_CHAN;                                  //number of channels per packet
int SAMPS_PER_CHANNEL;                         //samples per packet per channel, for 2 channels, this value is 5*62  = 310
int BYTES_PER_SAMP;                            //bytes per sample
int DATA_SIZE;                                 //packet data size (bytes)
int PACKET_SIZE;                               //packet size (bytes)
int REQUIRED_BYTES;
int DATA_BYTES_PER_CHANNEL;                    //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
int NUM_PACKS_DETECT;
int packetCounter = 0;
int DATA_SEGMENT_LENGTH;
int SAMPLE_RATE = 1e5;
int MICRO_INCR;                                // time between packets
const float TIME_WINDOW = 0.5;                 // fraction of a second to consider  
const string OUTPUT_FILE = "clicks_data.txt";

std::queue<vector<uint8_t>> dataBuffer;

std::mutex dataBufferLock;                       // For thread-safe buffer access
std::mutex dataSegmentLock;                       // For thread-safe buffer access
std::mutex dataTimesLock;                       // For thread-safe buffer access
std::mutex udpSocketLock;                       // For thread-safe buffer access

void UdpListener(int datagramSocket) {
    /*
    Functionality:
    This function serves as a UDP listener that continuously listens for incoming UDP packets.
    It receives UDP packets of a specified size, checks their length, and if they meet the expected size, 
    increments a packet counter. The received data is then placed into a shared buffer for further processing.

    Global Variables:
    - dataBuffer: shared buffer to store received data packets.
    - packetCounter: counter to keep track of the number of received packets.
    - udpSocket: the UDP socket used for listening to incoming packets.    
    */

    try {
        struct sockaddr_in addr;
        socklen_t addrLength = sizeof(addr);
        int bytesReceived;
        int printInterval = 500;
        vector<uint8_t> dataBytes(PACKET_SIZE + 1); // + 1 to detect if an erroneous amount of data is being sent
        auto startPacketTime = std::chrono::high_resolution_clock::now();
        
        while (true) {
            
            udpSocketLock.lock();
            int bytesReceived = recvfrom(datagramSocket, dataBytes.data(), PACKET_SIZE, 0, (struct sockaddr*)&addr, &addrLength);
            udpSocketLock.unlock();
            
            if (bytesReceived == -1) {
                cerr << "Error in recvfrom" << endl;
                continue;
            }

            dataBytes.resize(bytesReceived); // Adjust size based on actual bytes received
            packetCounter += 1;
            if (packetCounter % printInterval == 0) {
                auto endPacketTime = std::chrono::high_resolution_clock::now();
                auto durationPacketTime = std::chrono::duration_cast<std::chrono::seconds>(endPacketTime - startPacketTime).count();
                dataBufferLock.lock();
                size_t qSize = dataBuffer.size();
                dataBufferLock.unlock();
                float define = (float)durationPacketTime / packetCounter; 
                cout << "Num packets received is " <<  packetCounter << " " << define  << " " << qSize << " " << packetCounter - qSize << endl;
                //startPacketTime = std::chrono::high_resolution_clock::now();

            }
            dataBufferLock.lock();
            dataBuffer.push(dataBytes);
            dataBufferLock.unlock();
        }
    } catch (const std::exception& e ){
        // Handle the exception
        cerr << "Error occured in UDPListener Thread" << endl;

    }
}

void DataProcessor(void (*ProcessingFunction)(vector<int16_t>&, vector<TimePoint>&, const string&, 
arma::Row<int16_t>&, arma::Row<int16_t>&, arma::Row<int16_t>&, arma::Row<int16_t>&)) {
    /*
    Functionality:
    This function serves as a data processor that continuously processes data segments retrieved from a shared buffer (dataBuffer).
    It extracts the necessary information from the received data, such as timestamps and sample values, performs adjustments,
    and stores the processed data into a segment (dataSegment). This segment is then processed.

    Global Variables:
    - dataBuffer: Shared buffer containing received data packets.
    - dataSegment: Segment of data to be processed.
    - dataTimes: Array containing timestamps associated with data segments. 
    */

    try {
        bool previousTimeSet = false;
        std::chrono::time_point<std::chrono::system_clock> previousTime = std::chrono::time_point<std::chrono::system_clock>::min(); // CHECK VALUE

        while (true) {
            
            dataTimesLock.lock();
            vector<std::chrono::system_clock::time_point> dataTimes;
            dataTimesLock.unlock();
            dataTimesLock.lock();
            vector<int16_t> dataSegment;
            dataTimesLock.unlock();
            
            while (dataSegment.size() < DATA_SEGMENT_LENGTH) {
                
                dataBufferLock.lock();
                int qSize = dataBuffer.size();
                dataBufferLock.unlock();

                if (qSize < 1){
                    // sleep function here
                    continue;
                }
                
                dataBufferLock.lock();
                vector<uint8_t> dataBytes = dataBuffer.front();
                dataBuffer.pop();
                dataBufferLock.unlock();

                if (dataBytes.size() != PACKET_SIZE){
                    cerr << "Error: recieved incorrect number of packets" << endl;
                    previousTimeSet = false; 
                    //restartListener()
                    continue;
                }
                
                std::tm timeStruct;
                timeStruct.tm_year = (int)dataBytes[0] + 2000 - 1900;     // Offset for year since 2000.. tm_year is years since 1900 
                timeStruct.tm_mon = dataBytes[1] - 1;                     // Months are 0-indexed
                timeStruct.tm_mday = dataBytes[2];
                timeStruct.tm_hour = dataBytes[3];
                timeStruct.tm_min = dataBytes[4];
                timeStruct.tm_sec = dataBytes[5];

                uint32_t microSec = (static_cast<uint32_t>(dataBytes[6]) << 24) |
                         (static_cast<uint32_t>(dataBytes[7]) << 16) |
                         (static_cast<uint32_t>(dataBytes[8]) << 8) |
                         static_cast<uint32_t>(dataBytes[9]);
                
                std::chrono::time_point<std::chrono::system_clock> specificTime = std::chrono::system_clock::from_time_t(mktime(&timeStruct));
                specificTime += std::chrono::microseconds(microSec);
                
                if (previousTimeSet && (std::chrono::duration_cast<std::chrono::microseconds>(specificTime - previousTime).count()) != MICRO_INCR){
                    cerr << "Error: Time not incremented by " <<  MICRO_INCR << endl; 
                    previousTimeSet = false;
                    //restartListener;
                    continue;
                }
                // Convert byte data to signed 16bit ints
                vector<int16_t> data;
                for (size_t i = 0; i < DATA_SIZE; i += 2) {
                    int16_t value = static_cast<int16_t>(dataBytes[12+i]) +
                                   (static_cast<int16_t>(dataBytes[i + 13]) << 8);
                    data.push_back(value - 32768);
                }

                dataTimesLock.lock();
                dataTimes.push_back(specificTime);
                dataTimesLock.unlock();
                
                dataSegmentLock.lock();
                dataSegment.insert(dataSegment.end(), data.begin(), data.end());
                dataSegmentLock.unlock();

                previousTime = specificTime;
            }
            
            #ifdef PRINT_DATA_PROCESSOR // print first few values in dataSegment
                cout << "Inside DataProcessor() " << endl;
                cout << "dataSegment Size: " << dataSegment.size() << " should be same as " << NUM_PACKS_DETECT * (DATA_SIZE / 2) << endl;
                for (size_t j = 0; j < 50; j++){
                    cout << dataSegment[j] << " ";
                }
                cout << endl;
            #endif
           
            arma::Row<int16_t> ch1, ch2, ch3, ch4;
            dataSegmentLock.lock();
            ProcessingFunction(dataSegment, dataTimes, OUTPUT_FILE, ch1, ch2, ch3, ch4);
            dataSegmentLock.unlock();
            int threshold = 80;
            DetectionResult values = ThresholdDetect(ch1, dataTimes, threshold);
            if (values.peakTimes.size() == 0){
                cout << "No pulse detected!!!!!" << endl;
                continue;
            }
            //WritePulseAmplitudes(clickTimes, clickAmplitudes, args.output_file);
        }
    } catch (const std::exception& e ){
        // Handle the exception
        cerr << "Error occured in data processor  Thread" << endl;
      }
}


int main(int argc, char *argv[]){
    
    string UDP_IP = argv[1];                                         // IP address of data logger or simulator
    int UDP_PORT = std::stoi(argv[2]);                                     // Port to listen for UDP packets
    int firmwareVersion = std::stoi(argv[3]);
    printf("Listening to IP address %s and port %d \n", UDP_IP.c_str(),UDP_PORT);

    //import variables according to firmware version specified
    cout << "Assuming firmware version: " << firmwareVersion << endl;
    void(*ProcessFncPtr)(vector<int16_t>&, vector<TimePoint>&, const string&, arma::Row<int16_t>&, arma::Row<int16_t>&, arma::Row<int16_t>&, arma::Row<int16_t>&) = nullptr;
    if (firmwareVersion == 1550){
        ProcessFile("1550_config.txt");
        ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else if (firmwareVersion == 1240){
        ProcessFile("1240_config.txt");
        ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else{
        cerr << "ERROR: Unknown firmware version" << endl;
        return 1;
    }
    
    NUM_PACKS_DETECT = (int)(TIME_WINDOW * 100000 / SAMPS_PER_CHANNEL);  // NEED TO ROUND THIS  the number of data packets that are needed to perform energy detection 
    DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN; 

    cout << "HEAD_SIZE: "              << HEAD_SIZE               << endl; 
    cout << "SAMPS_PER_CHAN: "         << SAMPS_PER_CHANNEL       << endl;
    cout << "BYTES_PER_SAMP: "         << BYTES_PER_SAMP          << endl;
    cout << "Bytes per packet:       " << REQUIRED_BYTES         << endl;
    cout << "Time between packets:   " << MICRO_INCR             << endl;
    cout << "Number of channels:     " << NUM_CHAN               << endl;
    cout << "Data bytes per channel: " << DATA_BYTES_PER_CHANNEL << endl;
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
    

    //while (true)
    std::thread udpThread(UdpListener, datagramSocket);
    std::thread processorThread(DataProcessor, ProcessFncPtr);

    udpThread.join();
    processorThread.join();
    

    return 0;
}
