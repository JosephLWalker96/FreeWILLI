#include <iostream>
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

void restartListener(Session& sess){
    /*
    Functionality:
    This function is responsible for (re)starting the listener.
    The socket connection is re(set). The  buffer (dataBuffer) and segment to be processed (dataSegment) are cleared.

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

void ProcessFile(const string& fileName) {
    cout << "Entered ProcessFile: " << fileName <<  endl;
    cout << "HERE " << std::filesystem::current_path() << endl;
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable ERRR to open file '" << fileName << "'." << endl;
        return;
    }
    inputFile >> HEAD_SIZE >> MICRO_INCR >> NUM_CHAN >> SAMPS_PER_CHANNEL >> BYTES_PER_SAMP;
    DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP;       //packet data size (bytes) = 1240
    
    cout << "NUM_CHAN: " << NUM_CHAN << endl;
    PACKET_SIZE = HEAD_SIZE + DATA_SIZE;                             //packet size (bytes) = 1252
    REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP;     //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
}

void ClearQueue(std::queue<std::vector<uint8_t>>& q){
    std::queue<std::vector<uint8_t>> empty;
    std::swap(q, empty);
}

bool withProbability(double probability){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double randomValue = dis(gen);
    return randomValue < probability;
}
