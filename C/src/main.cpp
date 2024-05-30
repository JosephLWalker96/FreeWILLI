/*
This is a C++ version of Python/multi_datalogger_reader.py


Execute (datalogger simulator):
./listen_* 192.168.7.2 1045 1240

Execute (datalogger):
./listen_* 192.168.100.220 50000 1240


RESOURCES:
    debugging (core dump): https://www.youtube.com/watch?v=3T3ZDquDDVg&t=190s
    download armadillo manually: https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
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
//#include <armadillo> not needed if <sigpack.h> is used

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
#include "custom_types.h"
#include "filters.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::cerr;
using std::vector;
using std::ifstream;
using namespace std::chrono_literals;


// Global variables (used for testing and logging to console)
int packetCounter = 0;
int detectionCounter = 0;
bool test = true;

void UdpListener(Session& sess, unsigned int PACKET_SIZE) {
    /**
     * @brief Listens for UDP packets and processes them, storing data in a buffer.
     * 
     * This function continuously listens for incoming UDP packets on a specified socket.
     * It receives data and stores the received data into a buffer (sess.dataBuffer). 
     * Statistics about the received packets are printed to the console.
     * 
     * @param exp Reference to an Experiment object, which contains configuration details like PACKET_SIZE.
     * @param sess Reference to a Session object, which contains session-specific details and state, such as the datagram socket and buffers.
     * 
     * @throws std::runtime_error if there is an error in receiving data from the socket.
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
            
            bytesReceived = recvfrom(sess.datagramSocket, dataBytes.data(), receiveSize, 0, (struct sockaddr*)&addr, &addrLength);
            
            if (bytesReceived == -1) {
                throw std::runtime_error( "Error in recvfrom: bytesReceived is -1");
            }
            
            dataBytes.resize(bytesReceived); // Adjust size based on actual bytes received
            
            sess.dataBufferLock.lock(); // give this thread exclusive rights to modify the shared dataBytes variable
            sess.dataBuffer.push(dataBytes);
            qSize = sess.dataBuffer.size();
            sess.dataBufferLock.unlock();
            
            packetCounter += 1;
            if (packetCounter % printInterval == 0) {
                endPacketTime = std::chrono::steady_clock::now();
                durationPacketTime = endPacketTime - startPacketTime;
                define = durationPacketTime.count() / printInterval; 
                cout << "Num packets received is " <<  packetCounter << " " << define  << " " << qSize << " " << packetCounter - qSize << " " << detectionCounter << endl;
                startPacketTime = std::chrono::steady_clock::now();
            }
        }
    } catch (const std::exception& e ){
        // Handle the exception
        cerr << "Error occured in UDP Listener Thread: " << endl;
        cerr << e.what() << endl;
        sess.errorOccurred = true;
    }
}

void DataProcessor(Session& sess, Experiment& exp) {
    /**
     * @brief Processes data segments from a shared buffer, performs filtering and analysis.
     *
     * This function continuously processes data segments retrieved from a shared buffer (`dataBuffer`).
     * It extracts timestamps and sample values, applies necessary adjustments and filters, and stores
     * the processed data into a segment (`dataSegment`). The processed data is then further analyzed
     * to detect pulses, apply filters, and estimate time differences and directions of arrival.
     *
     * @param exp Reference to an Experiment object containing configuration details like data segment length,
     *            number of channels, filter weights, and other processing parameters.
     * @param sess Reference to a Session object containing session-specific details and state, such as the 
     *             data buffer, segment buffer, and various locks for synchronization.
     * 
     * @throws std::runtime_error if there is an error in processing data, such as incorrect packet sizes
     *                            or unexpected time increments.
     */

    try {
        int channelSize = exp.DATA_SEGMENT_LENGTH / exp.NUM_CHAN; // the number of samples per channel within a dataSegment
        
        // declare FFT object
        sp::FFTW fftw(channelSize, FFTW_ESTIMATE); // no 0 padding is currently being used
        
        // Read filter weights from file 
        arma::Col<double> h = ReadFIRFilterFile(exp.filterWeights);
        

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
        
        // pre-allocate memory for vectors
        sess.dataSegment.reserve(exp.DATA_SEGMENT_LENGTH);
        sess.dataTimes.reserve(exp.NUM_PACKS_DETECT);
        
        // Initialize armadillo containers to be used for number crunching
        arma::Col<double> ch1(channelSize);
        arma::Col<double> ch2(channelSize);
        arma::Col<double> ch3(channelSize);
        arma::Col<double> ch4(channelSize);
        arma::Mat<arma::cx_double> savedFFTs(channelSize, exp.NUM_CHAN); // save the FFT transformed channels
       
        // Container for pulling bytes from buffer (dataBuffer)
        vector<uint8_t> dataBytes;

        while (!sess.errorOccurred) {
            
            sess.dataTimes.clear();
            sess.dataSegment.clear();

            while (sess.dataSegment.size() < exp.DATA_SEGMENT_LENGTH) {
                
                auto startLoop = std::chrono::steady_clock::now();
                
                sess.dataBufferLock.lock();   // give this thread exclusive rights to modify the shared dataBytes variable
                int qSize = sess.dataBuffer.size();
                if (qSize < 1){
                    sess.dataBufferLock.unlock();
                    cout << "Sleeping: " << endl;
                    std::this_thread::sleep_for(200ms);
                    continue;
                }
                else{
                    dataBytes = sess.dataBuffer.front();
                    sess.dataBuffer.pop();
                    sess.dataBufferLock.unlock();
                }

                if (dataBytes.size() != exp.PACKET_SIZE){
                    cout << "PACKET_SIZE: " << exp.PACKET_SIZE << " dataBytes size: " << dataBytes.size() << endl;
                    throw std::runtime_error("Error: incorrect number of bytes in packet");
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

                if (previousTimeSet && (elapsed_time_ms != exp.MICRO_INCR)){
                    cerr << "Error: Time not incremented by " <<  exp.MICRO_INCR << " " << elapsed_time_ms << endl;
                    throw std::runtime_error("Error: Time not incremented by MICRO_INCR");
                }

                // Convert byte data to doubles
                auto startCDTime = std::chrono::steady_clock::now();
                ConvertData(sess.dataSegment, dataBytes, exp.DATA_SIZE, exp.HEAD_SIZE);
                auto endCDTime = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationCD = endCDTime - startCDTime;
                
                sess.dataTimes.push_back(specificTime);
                
                previousTime = specificTime;
                previousTimeSet = true;
                auto endLoop = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationLoop = endLoop - startLoop;
                //cout << "Loop duration: " << durationLoop.count() << endl;
            }

            /*
             *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
             *   now apply energy detector. 
             */
            
            exp.ProcessFncPtr(sess.dataSegment, ch1, ch2, ch3, ch4, exp.NUM_CHAN);
            
            DetectionResult values = ThresholdDetect(ch1, sess.dataTimes, exp.energyDetThresh, exp.SAMPLE_RATE);
            
            if (values.maxPeakIndex < 0){ // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue;
            }
            detectionCounter++;
           
            WritePulseAmplitudes(values.peakAmplitude, values.peakTimes, exp.outputFile);
            
            /*
             *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
             */

            //auto beforeFilter = std::chrono::steady_clock::now();
            FilterWithFIR(ch1,ch2,ch3,ch4, fir_filt);
            //FilterWithLiquidFIR(ch1,ch2,ch3,ch4, fir_filt);
            //FilterWithIIR(ch1,ch2,ch3,ch4, iir_filt);
            //auto afterFilter = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationFilter = afterFilter - beforeFilter;
            //cout << "C FIR Filter: " << durationFilter.count() << endl;
            
            auto beforeGCC = std::chrono::steady_clock::now();
            
            // Perform FFT using SigPack's FFTW object
            savedFFTs.col(0) = fftw.fft(ch1);
            savedFFTs.col(1) = fftw.fft(ch2);
            savedFFTs.col(2) = fftw.fft(ch3);
            savedFFTs.col(3) = fftw.fft(ch4);
            
            arma::Col<double> resultMatrix = GCC_PHAT(savedFFTs, exp.interp, fftw, channelSize, exp.NUM_CHAN, exp.SAMPLE_RATE);
            //Eigen::MatrixXd resultMatrix = GCC_PHAT_Eigen(dataE, exp.interp); // need to create dataE matrix 
            auto afterGCC = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationGCC = afterGCC - beforeGCC;
            //cout << "C GCC: " << durationGCC.count() << endl;
            //cout << resultMatrix.t() << endl;

            //arma::Col<double> DOAs = DOA_EstimateVerticalArray(resultMatrix, exp.speedOfSound, exp.chanSpacing);
            //auto endAll = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationFilter = endAll - beforeGCC;
            //cout << "Duration Filter: " << durationFilter.count() << endl;

        }
    } 
    catch (const GCC_Value_Error& e){
        // Handle the exception
        // WRITE DATA TO FILE
        cerr << e.what() << endl;
        sess.errorOccurred = true;
    }

    catch (const std::exception& e ){
        // Handle the exception
        cerr << "Error occured in data processor thread: " << endl;
        cerr << e.what() << endl;
        sess.errorOccurred = true;
      }
}


int main(int argc, char *argv[]){
    arma::arma_version ver;
    cout << "ARMA version: "<< ver.as_string() << endl;
    
    Experiment exp;
    
    string UDP_IP = argv[1];                                         // IP address of data logger or simulator
    if (UDP_IP == "self"){
        UDP_IP = "127.0.0.1";
    }
    int UDP_PORT = std::stoi(argv[2]);                                     // Port to listen for UDP packets
    int firmwareVersion = std::stoi(argv[3]);
    
    printf("Listening to IP address %s and port %d \n", UDP_IP.c_str(),UDP_PORT);

    //import variables according to firmware version specified
    cout << "Assuming firmware version: " << firmwareVersion << endl;
    if (firmwareVersion == 1550){
        if (ProcessFile(exp, "../ConfigFiles/1550_config.txt")){
            return EXIT_FAILURE;
        }
        exp.ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else if (firmwareVersion == 1240){
        if (ProcessFile(exp, "../ConfigFiles/1240_config.txt")){
            return EXIT_FAILURE;
        }
        exp.ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else{
        cerr << "ERROR: Unknown firmware version" << endl;
        return EXIT_FAILURE;
    }
    
    exp.NUM_PACKS_DETECT = (int)(exp.TIME_WINDOW * 100000 / exp.SAMPS_PER_CHANNEL);  // NEED TO ROUND THIS  the number of data packets that are needed to perform energy detection 
    exp.DATA_SEGMENT_LENGTH = exp.NUM_PACKS_DETECT * exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN; 

    cout << "HEAD_SIZE: "              << exp.HEAD_SIZE               << endl; 
    cout << "SAMPS_PER_CHAN: "         << exp.SAMPS_PER_CHANNEL       << endl;
    cout << "BYTES_PER_SAMP: "         << exp.BYTES_PER_SAMP          << endl;
    cout << "Bytes per packet:       " << exp.REQUIRED_BYTES         << endl;
    cout << "Time between packets:   " << exp.MICRO_INCR             << endl;
    cout << "Number of channels:     " << exp.NUM_CHAN               << endl;
    cout << "Data bytes per channel: " << exp.DATA_BYTES_PER_CHANNEL << endl;
    cout << "Detecting over a time window of " << exp.TIME_WINDOW << " seconds, using " << exp.NUM_PACKS_DETECT <<  " packets" << endl;

    
    // Open the file in write mode and clear its contents if it exists, create a new file otherwise
    std::ofstream file(exp.outputFile, std::ofstream::out | std::ofstream::trunc);
    if (file.is_open()) {
        file << "Timestamp (microseconds)" << std::setw(20) << "Peak Amplitude" << endl;
        file.close();
        cout << "File created and cleared: " << exp.outputFile << endl;
    } 
    else {
        cerr << "Error: Unable to open file for writing: " << exp.outputFile << endl;
        return 1;
    }

    // Declare a listening 'Session'
    Session sess;
    sess.UDP_PORT = UDP_PORT;
    sess.UDP_IP = UDP_IP;

    while (true){
        
        RestartListener(sess);
        
        std::thread udpThread(UdpListener, std::ref(sess), exp.PACKET_SIZE);
        std::thread processorThread(DataProcessor, std::ref(sess), std::ref(exp));

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
