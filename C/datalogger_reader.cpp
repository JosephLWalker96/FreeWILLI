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
//using std::chrono;
//using std::chrono::sys_clock;

int HEAD_SIZE;                      //packet head size (bytes)
double MICRO_INCR;            // time between packets
int NUM_CHAN;                      //number of channels per packet
int SAMPS_PER_CHANNEL;            //samples per packet per channel, for 2 channels, this value is 5*62  = 310
int BYTES_PER_SAMP;                                             //bytes per sample

int DATA_SIZE;       //packet data size (bytes) = 1240
int PACKET_SIZE;                             //packet size (bytes) = 1252
int REQUIRED_BYTES;
int DATA_BYTES_PER_CHANNEL;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
int NUM_PACKS_DETECT;
const float TIME_WINDOW = 0.5;                                                    // fraction of a second to consider  

int packet_counter = 0;
std::queue<vector<uint8_t>> data_buffer;
std::mutex buffer_mutex;  // For thread-safe buffer access

void processFile(const string& fileName) {
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

void udp_listener(int sockfd) {
    
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    while (true) {
        vector<uint8_t> dataB(PACKET_SIZE);
        //vector<uint16_t> dataB(PACKET_SIZE);
        int bytes_received = recvfrom(sockfd, dataB.data(), PACKET_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
        
        if (bytes_received == -1) {
            cerr << "Error in recvfrom" << endl;
            continue;
        }
        
        //dataB.resize(bytes_received); // Adjust size based on actual bytes received
        lock_guard<mutex> lock(buffer_mutex);
        data_buffer.push(dataB);
        if (packet_counter % 500 == 0) {
            cout << "Num packets received is " << packet_counter << endl;
        }
    }
}

void data_processor() {
    while (true) {
        vector<int16_t> data_segment;
        vector<std::chrono::system_clock::time_point> times;
        while (data_segment.size() < NUM_PACKS_DETECT * SAMPS_PER_CHANNEL) {
            lock_guard<mutex> lock(buffer_mutex);
            if (!data_buffer.empty()) {
                vector<uint8_t> dataB = data_buffer.front();
                //vector<uint16_t> dataI = (uint16_t)data_buffer.front();
                data_buffer.pop();
                
                //auto duration = timestamp;
                std::tm time_data;
                time_data.tm_year = (int)dataB[0] + 2023; // Offset for year since 2000
                time_data.tm_mon = dataB[1] - 1;   // Months are 0-indexed
                time_data.tm_mday = dataB[2];
                time_data.tm_hour = dataB[3];
                time_data.tm_min = dataB[4];
                time_data.tm_sec = dataB[5];

                uint32_t usec = (static_cast<uint32_t>(dataB[6]) << 24) |
                         (static_cast<uint32_t>(dataB[7]) << 16) |
                         (static_cast<uint32_t>(dataB[8]) << 8) |
                         static_cast<uint32_t>(dataB[9]);
                
                // Construct date and time
                /*std::chrono::time_point<std::chrono::system_clock> date_time =
                        std::mktime(&time_data);
                date_time += std::chrono::microseconds(usec);
                */

                // Convert byte data to unsigned 16bit ints
                vector<int16_t> data;
                for (size_t i = 0; i < DATA_SIZE; i += 2) {
                    int16_t value = static_cast<int16_t>(dataB[12+i]) +
                                   (static_cast<int16_t>(dataB[i + 13]) << 8);
                    data.push_back(value - 32768);
                }

                for (size_t j = 0; j < 12; j++){
                    cout << data[j] << " ";
                }
                cout << endl;
    
                // Append data and timestamp
                //times.push_back(timestamp);
                //timestamps.push_back(std::mktime(&time_data));
                //data_segment.insert(data_segment.end(), dataJ.begin(), dataJ.end());

                        ////////////data_segment.insert(data_segment.end(), dataJ.begin(), dataJ.end());
                        // ...
            }
        }

        // process_segment(data_segment, times, args.output_file);  // Replace with your processing code
    }
}


int main(int argc, char *argv[]){
    //char* UDP_IP[12];
    string UDP_IP = argv[1];                                         // IP address of data logger or simulator
    int UDP_PORT = stoi(argv[2]);                                     // Port to listen for UDP packets
    int fw_version = stoi(argv[3]);
    printf("Listening to IP address %s and port %d \n", UDP_IP.c_str(),UDP_PORT);

    //#import variables according to firmware version specified
    cout << "Assuming firmware version: " << fw_version << endl;
    if (fw_version == 1550){
        //from Firmware_config.firmware_1550 import *
        //process_segment = process_segment_1550
        cout << "functionality for fw version 1550 is undefined" << endl;
        return -1;
    }
    else if (fw_version == 1240){
        processFile("1240_config.txt");
    }
    else{
        printf("ERROR: Unknown firmware version");
        return -1;
    }

    
    NUM_PACKS_DETECT = TIME_WINDOW * 100000 / SAMPS_PER_CHANNEL;  // NEED TO ROUND THIS  the number of data packets that are needed to perform energy detection 
    

    cout << "HEAD_SIZE: " << HEAD_SIZE << endl; 
    cout << "SAMPS_PER_CHAN: " << SAMPS_PER_CHANNEL << endl;
    cout << "BYTES_PER_SAMP: " << BYTES_PER_SAMP << endl;
    cout << "Bytes per packet:       " <<  REQUIRED_BYTES << endl;
    cout << "Time between packets:   " <<  MICRO_INCR << endl;
    cout << "Number of channels:     " <<  NUM_CHAN << endl;
    cout << "Data bytes per channel: " <<  DATA_BYTES_PER_CHANNEL << endl;
    cout << "Detecting over a time window of " << TIME_WINDOW << " seconds, using " << NUM_PACKS_DETECT <<  " packets" << endl;

        
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(UDP_IP.c_str());
    server_addr.sin_port = htons(UDP_PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Error binding socket" << endl;
        return 1;
    }

    thread udp_thread(udp_listener, sockfd);
    thread processor_thread(data_processor);

    udp_thread.join();
    processor_thread.join();

    //cout << "GLOBAL COUNTER: " << COUNTER << endl;

    close(sockfd);
        return 0;   
    }

