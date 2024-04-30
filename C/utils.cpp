
#include "my_globals.h"
#include <iostream>
#include <string>
#include <fstream>

using std::cerr;
using std::endl;
using std::cout;
using std::string;

void ProcessFile(const string& fileName) {
    std::ifstream inputFile(fileName);
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
