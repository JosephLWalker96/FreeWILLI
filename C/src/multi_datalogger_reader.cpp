/*
This is a C++ version of Python/multi_datalogger_reader.py

Compile code manually:
g++ multi_datalogger_reader.cpp process_data.cpp utils.cpp -o listen -larmadillo
g++ -std=c++20 multi_datalogger_reader.cpp process_data.cpp utils.cpp -o listen -larmadillo -I/usr/include/sigpack-1.2.7/sigpack
g++ -std=c++20 multi_datalogger_reader.cpp process_data.cpp utils.cpp TDOA_estimation.cpp -o listen -larmadillo -I/usr/include/sigpack-1.2.7/sigpack -lliquid
g++ -Ofast -std=c++20 -march=native -flto multi_datalogger_reader.cpp process_data.cpp utils.cpp TDOA_estimation.cpp -o listen -DARMA_DONT_USE_WRAPPER -DNDEBUG -lopenblas -llapack -I/usr/include/sigpack-1.2.7/sigpack -lliquid
g++ -Ofast -std=c++20 -march=native -DEIGEN_FFT_BACKEND=FFTW -flto -msse2 multi_datalogger_reader.cpp process_data.cpp utils.cpp TDOA_estimation.cpp -o listen -DARMA_DONT_USE_WRAPPER -DNDEBUG -lopenblas -llapack -I/usr/include/sigpack-1.2.7/sigpack -lliquid -lfftw3
g++ -Ofast -std=c++20 -march=native -DEIGEN_FFT_BACKEND=FFTW -flto -msse2 multi_datalogger_reader.cpp process_data.cpp utils.cpp TDOA_estimation.cpp -o listen -lopenblas -llapack -I/usr/include/sigpack-1.2.7/sigpack -lliquid -lfftw3
g++ -Ofast -std=c++20 -march=native -flto multi_datalogger_reader.cpp process_data.cpp utils.cpp TDOA_estimation.cpp -o listen -lopenblas -llapack -I/usr/include/sigpack-1.2.7/sigpack -lliquid -lfftw3

Execute (datalogger simulator):
./listen 192.168.7.2 1045 1240

Execute (datalogger):
./listen.exe 192.168.100.220 50000 1240

TO DO:
    0) How to deal with out of bounds DOA vals
    1) Exception handling.. cerr + throw
    2) Zero pad before circular convolution

OPTIMIZATIONS: 
    1) filter in frequency domain. Do not return to time domain
    2) Watch and Implement "How to align data for efficient FIR filter"



RESOURCES:
    debugging (core dump): https://www.youtube.com/watch?v=3T3ZDquDDVg&t=190s
*/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <ostream>
#include <string>
#include <fstream>
#include <mutex>
#include <thread>
#include <queue>
#include <vector>
#include <chrono>
#include <atomic>
#include <stdexcept>
#include <random>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iomanip> //put_time
#include <ctime>
#include <cstdint>
#include <armadillo>

#include <sigpack.h>
#include <fftw/fftw.h>
//#include <Eigen/Dense>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Core>

#include <complex>
#include <liquid/liquid.h>
//#include <liquid.h>

#include "process_data.h"
#include "TDOA_estimation.h"
#include "utils.h"
#include "my_globals.h"
#include "filters.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::cerr;
using std::vector;
using std::ifstream;
using namespace std::chrono_literals;

// Global Constants
unsigned int HEAD_SIZE;                                 //packet head size (bytes)
unsigned int NUM_CHAN;                                  //number of channels per packet
unsigned int SAMPS_PER_CHANNEL;                         //samples per packet per channel, for 2 channels, this value is 5*62  = 310
unsigned int BYTES_PER_SAMP;                            //bytes per sample
unsigned int DATA_SIZE;                                 //packet data size (bytes)
unsigned int PACKET_SIZE;                               //packet size (bytes)
unsigned int REQUIRED_BYTES;
unsigned int DATA_BYTES_PER_CHANNEL;                    //number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels
unsigned int NUM_PACKS_DETECT;
unsigned int DATA_SEGMENT_LENGTH;
unsigned int MICRO_INCR;                                // time between packets
const unsigned int SAMPLE_RATE = 1e5;
const double TIME_WINDOW = 0.01;                 // fraction of a second to consider  
const string OUTPUT_FILE = "clicks_data.txt";
std::string outputFile = "Ccode_clicks.txt"; // Change to your desired file name

int packetCounter = 0;
bool test = true;

void UdpListener(Session& sess) {
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
        int receiveSize = PACKET_SIZE + 1;      // + 1 to detect if an erroneous amount of data is being sent
        vector<uint8_t> dataBytes(receiveSize);
        std::chrono::duration<double> durationPacketTime;
        auto startPacketTime = std::chrono::steady_clock::now();
        auto endPacketTime = startPacketTime;
        int qSize;
        double define;

        while (!sess.errorOccurred) {
            
            sess.udpSocketLock.lock();
            bytesReceived = recvfrom(sess.datagramSocket, dataBytes.data(), receiveSize, 0, (struct sockaddr*)&addr, &addrLength);
            sess.udpSocketLock.unlock();
            
            if (bytesReceived == -1) {
                throw std::runtime_error( "Error in recvfrom: bytesReceived is -1");
            }
            
            dataBytes.resize(bytesReceived); // Adjust size based on actual bytes received
            packetCounter += 1;
            if (packetCounter % printInterval == 0) {
                endPacketTime = std::chrono::steady_clock::now();
                durationPacketTime = endPacketTime - startPacketTime;
                sess.dataBufferLock.lock();
                qSize = sess.dataBuffer.size();
                sess.dataBufferLock.unlock();
                define = durationPacketTime.count() / printInterval; 
                cout << "Num packets received is " <<  packetCounter << " " << define  << " " << qSize << " " << packetCounter - qSize << endl;
                startPacketTime = std::chrono::steady_clock::now();
            }
            sess.dataBufferLock.lock();
            sess.dataBuffer.push(dataBytes);
            sess.dataBufferLock.unlock();
        }
    } catch (const std::exception& e ){
        // Handle the exception
        cerr << "Error occured in UDP Listener Thread: " << endl;
        cerr << e.what() << endl;
        sess.errorOccurred = true;
    }
}

void DataProcessor(void (*ProcessingFunction)(vector<double>&, arma::Col<double>&, arma::Col<double>&, arma::Col<double>&, arma::Col<double>&), Session& sess) {
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
        int channelSize = DATA_SEGMENT_LENGTH / NUM_CHAN; // the number of samples per channel within a dataSegment
        
        // declare FFT object
        sp::FFTW fftw(channelSize, FFTW_ESTIMATE); // no 0 padding is currently being used
        
        // Read filter weights from file 
        arma::Col<double> h = ReadFIRFilterFile("filters/My_filter.txt");
        

        // Define FIR filter using the liquid library 
        //firfilt_rrrf q = firfilt_rrrf_create(h,h.n_elem);// create filter object
        
        
        // Define IIR filter using SigPack library
        /*
        sp::IIR_filt<double, double, double> iir_filt;
        arma::Col<double> b_iir = {0.49580191, -1.9260157, 2.8613285, -1.9260157, 0.49580191};
        arma::Col<double> a_iir = {1.0, -2.53934052, 2.67821627, -1.31270499, 0.26392123};
        iir_filt.set_coeffs(b_iir,a_iir);
        */

        // Define FIR filter using SigPack library
        sp::FIR_filt<double, double, double> fir_filt;
        fir_filt.set_coeffs(h);
        
        // Declare time checking variables
        bool previousTimeSet = false;
        std::chrono::time_point<std::chrono::system_clock> previousTime = std::chrono::time_point<std::chrono::system_clock>::min(); // CHECK VALUE

        // Initialize armadillo containers to be used for number crunching
        arma::Col<double> ch1(channelSize);
        arma::Col<double> ch2(channelSize);
        arma::Col<double> ch3(channelSize);
        arma::Col<double> ch4(channelSize);
        arma::Mat<double> dataMatrix(ch1.n_elem, 4);
       
        // Container for pulling bytes from buffer (dataBuffer)
        vector<uint8_t> dataBytes;

        while (!sess.errorOccurred) {
            
            sess.dataTimesLock.lock();
            sess.dataTimes.clear();
            sess.dataTimesLock.unlock();
            
            sess.dataSegmentLock.lock();
            sess.dataSegment.clear();
            sess.dataSegmentLock.unlock();

            //arma::Col<double> pad(15, arma::fill::zeros);

            while (sess.dataSegment.size() < DATA_SEGMENT_LENGTH) {
                
                sess.dataBufferLock.lock();
                int qSize = sess.dataBuffer.size();
                sess.dataBufferLock.unlock();
                
                if (qSize < 1){
                    cout << "Sleeping: " << endl;
                    std::this_thread::sleep_for(200ms);
                    continue;
                }
                
                sess.dataBufferLock.lock();
                dataBytes = sess.dataBuffer.front();
                sess.dataBuffer.pop();
                sess.dataBufferLock.unlock();

                if (dataBytes.size() != PACKET_SIZE){
                    throw std::runtime_error("Error: recieved incorrect number of packets");
                }
                
                std::tm timeStruct{};
                timeStruct.tm_year = (int)dataBytes[0] + 2000 - 1900;     // Offset for year since 2000.. tm_year is years since 1900 
                timeStruct.tm_mon = (int)dataBytes[1] - 1;                     // Months are 0-indexed
                timeStruct.tm_mday = (int)dataBytes[2];
                timeStruct.tm_hour = (int)dataBytes[3];
                timeStruct.tm_min = (int)dataBytes[4];
                timeStruct.tm_sec = (int)dataBytes[5];


                int64_t microSec = (static_cast<int64_t>(dataBytes[6]) << 24) +
                         (static_cast<int64_t>(dataBytes[7]) << 16) +
                         (static_cast<int64_t>(dataBytes[8]) << 8) +
                         static_cast<int64_t>(dataBytes[9]);
                                
                
                // Use std::mktime to convert std::tm to std::time_t
                std::time_t timeResult = std::mktime(&timeStruct);

                if (timeResult == std::time_t(-1)) {
                    throw std::runtime_error("Error: failure in mktime");
                }

                std::chrono::time_point<std::chrono::system_clock> specificTime = std::chrono::system_clock::from_time_t(timeResult);
                specificTime += std::chrono::microseconds(microSec);
                
                auto duration = specificTime - previousTime;
                auto elapsed_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(specificTime - previousTime).count();

                if (previousTimeSet && (elapsed_time_ms != MICRO_INCR)){
                    cerr << "Error: Time not incremented by " <<  MICRO_INCR << " " << elapsed_time_ms << endl;
                    throw std::runtime_error("Error: Time not incremented by MICRO_INCR");
                }
                
                // Convert byte data to doubles
                sess.dataSegmentLock.lock();
                ConvertData(sess.dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE);
                sess.dataSegmentLock.unlock();
                
                //cout << "size: " << sess.dataSegment.size() << endl;
                /*cout << "chan 1 examples:  ";
                for (int yy = 0; yy < 10; yy++){
                    cout << ch1(yy) << " "; 
                }
                cout << endl;
                */
                sess.dataTimesLock.lock();
                sess.dataTimes.push_back(specificTime);
                sess.dataTimesLock.unlock();
                
                previousTime = specificTime;
                previousTimeSet = true;
            }

            /*
             *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
             *   now apply energy detector. 
             */
            sess.dataSegmentLock.lock();
            ProcessingFunction(sess.dataSegment, ch1, ch2, ch3, ch4);
            sess.dataSegmentLock.unlock();
            
            double threshold = 80.0;
            
            DetectionResult values = ThresholdDetect(ch1, sess.dataTimes, threshold);
            
            if (values.maxPeakIndex < 0){ // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue;
            }
           
            WritePulseAmplitudes(values.peakAmplitude, values.peakTimes, outputFile);
            
            /*
             *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
             */

            auto beforeFilter = std::chrono::steady_clock::now();
            FilterWithFIR(ch1,ch2,ch3,ch4, fir_filt);
            //FilterWithLiquidFIR(ch1,ch2,ch3,ch4, fir_filt);
            //FilterWithIIR(ch1,ch2,ch3,ch4, iir_filt);
            auto afterFilter = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFilter = afterFilter - beforeFilter;
            //cout << "C FIR Filter: " << durationFilter.count() << endl;
            


            /*cout << "chan 1 filt  examples:  ";
            for (int y = 0; y < 10; y++){
                cout << ch1(y) << " "; 
            }
            cout << endl;
            */
            
            dataMatrix.insert_cols(0,ch1);
            dataMatrix.insert_cols(1,ch2);
            dataMatrix.insert_cols(2,ch3);
            dataMatrix.insert_cols(3,ch4);
            
            int interp = 1;
            auto beforeGCC = std::chrono::steady_clock::now();
            arma::Col<double> resultMatrix = GCC_PHAT(dataMatrix, interp, fftw, channelSize);
            //Eigen::MatrixXd resultMatrix = GCC_PHAT_Eigen(dataE, interp); // need to create dataE matrix 
            auto afterGCC = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationGCC = afterGCC - beforeGCC;
            //cout << "C GCC: " << durationGCC.count() << endl;
            

            arma::Col<int> chanSpacing = {1,2,3,1,2,1};
            arma::Col<double> DOAs = DOA_EstimateVerticalArray(resultMatrix, 1500.0, chanSpacing);

            //cout << "DOAs: " << DOAs.t() << endl;

            /*
            for (int ii = 0; ii < 4; ++ii) {
                for (int jj = 0; jj < 4; ++jj) {
                    cout << resultMatrix(ii, jj) << " ";
                }
                cout << endl;
            }
            */

        }
    } catch (const std::exception& e ){
        // Handle the exception
        cerr << "Error occured in data processor thread: " << endl;
        cerr << e.what() << endl;
        sess.errorOccurred = true;
      }
}


int main(int argc, char *argv[]){
    arma::arma_version ver;
    cout << "ARMA version: "<< ver.as_string() << endl;

    string UDP_IP = argv[1];                                         // IP address of data logger or simulator
    if (UDP_IP == "self"){
        UDP_IP = "127.0.0.1";
    }
    int UDP_PORT = std::stoi(argv[2]);                                     // Port to listen for UDP packets
    int firmwareVersion = std::stoi(argv[3]);
    
    printf("Listening to IP address %s and port %d \n", UDP_IP.c_str(),UDP_PORT);

    //import variables according to firmware version specified
    cout << "Assuming firmware version: " << firmwareVersion << endl;
    void(*ProcessFncPtr)(vector<double>&, arma::Col<double>&, arma::Col<double>&, arma::Col<double>&, arma::Col<double>&) = nullptr;
    if (firmwareVersion == 1550){
        ProcessFile("ConfigFiles/1550_config.txt");
        ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else if (firmwareVersion == 1240){
        ProcessFile("ConfigFiles/1240_config.txt");
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

    
    // Open the file in write mode and clear its contents if it exists, create a new file otherwise
    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::trunc);
    if (file.is_open()) {
        file << "Timestamp (microseconds)" << std::setw(20) << "Peak Amplitude" << endl;
        file.close();
        cout << "File created and cleared: " << outputFile << endl;
    } 
    else {
        cerr << "Error: Unable to open file for writing: " << outputFile << endl;
        return 1;
    }

    // Declare a listening 'Session'
    Session sess;
    sess.UDP_PORT = UDP_PORT;
    sess.UDP_IP = UDP_IP;

    while (true){
        
        RestartListener(sess);
        
        std::thread udpThread(UdpListener, std::ref(sess));
        std::thread processorThread(DataProcessor, ProcessFncPtr, std::ref(sess));

        udpThread.join();
        processorThread.join();
       
        if (sess.errorOccurred){
            cout << "Restarting threads..." << endl;
        }
        else {
            cout << "Unknown problem occurred" << endl;
        }
        /* 
        // Close the existing socket before rebinding (if already created)
        sess.udpSocketLock.lock();
        if (close(sess.datagramSocket) == -1) {
            cerr << "Failed to close socket" << endl;
            //throw std::runtime_error("Failed to close socket");
        }
        sess.udpSocketLock.unlock();
        */
        sess.errorOccurred = false;
    }
    return 0;
}
