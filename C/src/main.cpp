/*

DESCRIPTION:
@file main.cpp
 @brief A program for receiving and processing UDP packets in real-time.
 
 This program sets up a UDP listener to receive packets from a specified IP address and port.
 The received packets are stored in a shared buffer and then processed to extract and analyze data.
 The program uses two threads to handle the data receiving and processing tasks concurrently.
 
 Key components:
 - **UDP Listener**: Listens for incoming UDP packets, stores the received data in a buffer, 
   and prints statistics about the received packets.
 - **Data Processor**: Retrieves data from the buffer, processes the data by applying filters 
   and performing analysis to detect and estimate specific signal characteristics.
 - **Session and Experiment Classes**: Manage session-specific and experiment-specific details,
   including configuration settings, data buffers, and synchronization mechanisms.
 - **Multi-threading**: Uses separate threads for listening to UDP packets and processing data,
   ensuring efficient and real-time handling of incoming data.
 
 The program is designed to handle configuration settings dynamically based on specified 
 firmware versions, adjust processing parameters accordingly, and log processed data to an output file.
 In case of errors, the program attempts to restart the listener and processing threads to maintain 
 continuous operation.
 
 @note This program requires the Armadillo and SigPack libraries for matrix operations and signal processing.
       It also uses FFTW for fast Fourier transforms and includes error handling to manage runtime exceptions.



EXAMPLE RUN:

Execute (datalogger simulator):
./HarpListen 192.168.7.2 1045 1240 2500

Execute (datalogger):
./HarpListen 192.168.100.220 50000 1240 2500


RESOURCES:
    debugging (core dump): https://www.youtube.com/watch?v=3T3ZDquDDVg&t=190s
    download armadillo manually: https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
*/

#include <cstddef>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <ostream>
#include <string>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <stdexcept>
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
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Core>

#include <liquid/liquid.h>

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
using namespace std::chrono_literals;


// Global variables (used for manual testing and logging to console)
int packetCounter = 0; // this should only be used inside the UDPListener function, as it is not protected by a mutex
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
        const int printInterval = 500;
        const int receiveSize = PACKET_SIZE + 1;              // + 1 to detect if more data than expected is being received
        size_t queueSize;
        auto startPacketTime = std::chrono::steady_clock::now();
        auto endPacketTime = startPacketTime;
        std::chrono::duration<double> durationPacketTime;     // stores the average amount of time (seconds) between successive UDP packets, averaged over 'printInterval' packets
        //auto durationPacketTime = std::chrono::duration_cast<std::chrono::microseconds>(endPacketTime - startPacketTime);  
        vector<uint8_t> dataBytes(receiveSize);
        
        while (!sess.errorOccurred) {
            
            bytesReceived = recvfrom(sess.datagramSocket, dataBytes.data(), receiveSize, 0, (struct sockaddr*)&addr, &addrLength);
            
            if (bytesReceived == -1)
                throw std::runtime_error( "Error in recvfrom: bytesReceived is -1");
            
            dataBytes.resize(bytesReceived);         // Adjust size based on actual bytes received
            
            sess.dataBufferLock.lock();              // give this thread exclusive rights to modify the shared dataBytes variable
            sess.dataBuffer.push(dataBytes);
            queueSize = sess.dataBuffer.size();
            sess.dataBufferLock.unlock();            // relinquish exclusive rights  
            
            packetCounter += 1;
            if (packetCounter % printInterval == 0) {
                endPacketTime = std::chrono::steady_clock::now();
                durationPacketTime = endPacketTime - startPacketTime;
                
                std::stringstream msg; // compose message to dispatch
                msg << "Num packets received is " <<  packetCounter << " " << durationPacketTime.count() / printInterval  
                    << " " << queueSize << " " << packetCounter - queueSize << " " << detectionCounter << endl;
                
                cout << msg.str(); // using one instance of "<<" makes the operation atomic
                
                startPacketTime = std::chrono::steady_clock::now();
            }

            if (queueSize > 1000) { // check if buffer has grown to an unacceptable size 
                cout << "Buffer overflowing!! \n";
                throw std::runtime_error("Buffer overflowing \n");
            }
                
        }
    } catch (const std::exception& e ) {
        cerr << "Error occured in UDP Listener Thread: \n";
        
        std::stringstream msg; // compose message to dispatch
        msg << e.what() << endl;
        cerr << msg.str();

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
        arma::Col<double> filterWeights = ReadFIRFilterFile(exp.filterWeights);
        

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
        sp::FIR_filt<double, double, double> firFilter;
        firFilter.set_coeffs(filterWeights);
        
        // Declare time checking variables
        bool previousTimeSet = false;
        auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
        //auto previousTime = std::chrono::system_clock::time_point::min();
        //std::chrono::microseconds previousTime;
        
        // pre-allocate memory for vectors
        sess.dataSegment.reserve(exp.DATA_SEGMENT_LENGTH);
        sess.dataTimes.reserve(exp.NUM_PACKS_DETECT);
        
        // Initialize armadillo containers to be used for storing channel data
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
            sess.dataBytesSaved.clear();

            while (sess.dataSegment.size() < exp.DATA_SEGMENT_LENGTH) {
                
                auto startLoop = std::chrono::steady_clock::now();
                
                sess.dataBufferLock.lock();   // give this thread exclusive rights to modify the shared dataBytes variable
                size_t queueSize = sess.dataBuffer.size();
                if (queueSize < 1) {
                    sess.dataBufferLock.unlock();
                    //cout << "Sleeping: " << endl;
                    std::this_thread::sleep_for(200ms);
                    continue;
                }
                else {
                    dataBytes = sess.dataBuffer.front();
                    sess.dataBuffer.pop();
                    sess.dataBufferLock.unlock();
                }

                sess.dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error
                
                
                std::tm timeStruct{};                                     // Initialize a std::tm structure to hold the date and time components
                timeStruct.tm_year = (int)dataBytes[0] + 2000 - 1900;     // Offset for year since 2000.. tm_year is years since 1900 
                timeStruct.tm_mon = (int)dataBytes[1] - 1;                // Months are 0-indexed
                timeStruct.tm_mday = (int)dataBytes[2];
                timeStruct.tm_hour = (int)dataBytes[3];
                timeStruct.tm_min = (int)dataBytes[4];
                timeStruct.tm_sec = (int)dataBytes[5];

                // Calculate microseconds from the given bytes by shifting and combining them
                int64_t microSec = (static_cast<int64_t>(dataBytes[6]) << 24) +
                         (static_cast<int64_t>(dataBytes[7]) << 16) +
                         (static_cast<int64_t>(dataBytes[8]) << 8) +
                         static_cast<int64_t>(dataBytes[9]);
                                
                
                // Use std::mktime to convert std::tm to std::time_t
                std::time_t timeResult = std::mktime(&timeStruct);

                // Check if mktime failed
                if (timeResult == std::time_t(-1)) {
                    throw std::runtime_error("Error: failure in mktime \n");
                }

                auto currentTime = std::chrono::system_clock::from_time_t(timeResult);  // convert std::time_t to std::chrono::system_clock::time_point
                currentTime += std::chrono::microseconds(microSec);
                sess.dataTimes.push_back(currentTime);

                // Calculate the elapsed time in microseconds since the previous time point
                auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count();


                /* 
                 * conduct checks to data here:
                 *
                 */

                // Check if the amount of bytes in packet is what is expected
                if (dataBytes.size() != exp.PACKET_SIZE) {
                    std::stringstream msg; // compose message to dispatch
                    msg << "Error: incorrect number of bytes in packet: " <<  "PACKET_SIZE: " << exp.PACKET_SIZE << " dataBytes size: " << dataBytes.size() << endl;
                    throw std::runtime_error(msg.str());
                }
                

                // Check if the previous time was set and if the elapsed time is not equal to the expected increment
                if (previousTimeSet && (elapsedTime != exp.MICRO_INCR)){
                    std::stringstream msg; // compose message to dispatch
                    msg <<  "Error: Time not incremented by " <<  exp.MICRO_INCR << " " << elapsedTime << endl;
                    throw std::runtime_error(msg.str());
                }

                // Convert byte data to doubles
                auto startCDTime = std::chrono::steady_clock::now();
                ConvertData(sess.dataSegment, dataBytes, exp.DATA_SIZE, exp.HEAD_SIZE); // bytes data is decoded and appended to sess.dataSegment
                auto endCDTime = std::chrono::steady_clock::now();
                auto durationCD = endCDTime - startCDTime;
                
                
                previousTime = currentTime;
                previousTimeSet = true;
                
                if ((exp.detectionOutputFile).empty()){
                    string feature = "detection";
                    InitiateOutputFile(exp.detectionOutputFile, timeStruct, microSec, feature);
                    feature = "tdoa";
                    InitiateOutputFile(exp.tdoaOutputFile, timeStruct, microSec, feature);
                    feature = "doa";
                    InitiateOutputFile(exp.doaOutputFile, timeStruct, microSec, feature);
                }

                auto endLoop = std::chrono::steady_clock::now();
                auto durationLoop = endLoop - startLoop;
                //cout << "Loop duration: " << durationLoop.count() << endl;
            }

            /*
             *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
             *   now apply energy detector. 
             */
            
            exp.ProcessFncPtr(sess.dataSegment, ch1, ch2, ch3, ch4, exp.NUM_CHAN);
            
            DetectionResult detResult = ThresholdDetect(ch1, sess.dataTimes, exp.energyDetThresh, exp.SAMPLE_RATE);

            if (detResult.maxPeakIndex < 0){  // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue;                  // get next dataSegment; return to loop
            }
            
            /*
             *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
             */
            
            detectionCounter++;
            
            //PrintTimes(sess.dataTimes);

            //auto beforeFilter = std::chrono::steady_clock::now();
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

            arma::Col<double> DOAs = DOA_EstimateVerticalArray(resultMatrix, exp.speedOfSound, exp.chanSpacing);
            
            WritePulseAmplitudes(detResult.peakAmplitude, detResult.peakTimes, exp.detectionOutputFile);
            WriteArray(resultMatrix, detResult.peakTimes, exp.tdoaOutputFile);
            WriteArray(DOAs, detResult.peakTimes, exp.doaOutputFile);

            //cout << DOAs.t() << endl;
            //auto endAll = std::chrono::steady_clock::now();
            //std::chrono::duration<double> durationFilter = endAll - beforeGCC;
            //cout << "Duration Filter: " << durationFilter.count() << endl;

        }
    } 
    catch (const GCC_Value_Error& e) {
        
        std::stringstream msg; // compose message to dispatch
        msg << e.what() << endl;
        cerr << msg.str();
        
        /*        
        try {
           WriteDataToCerr(sess.dataTimes, sess.dataBytesSaved);
        }
        catch (...) {
            cerr << "failed to write data to cerr \n";
        }
        */
        sess.errorOccurred = true;
    }
    catch (const std::ios_base::failure& e) {
        std::stringstream msg; // compose message to dispatch
        msg << e.what() << endl;
        cerr << msg.str();
        std::exit(EXIT_FAILURE);
    }
    catch (const std::exception& e ) {
        cerr << "Error occured in data processor thread: \n";

        
        std::stringstream msg; // compose message to dispatch
        msg << e.what() << endl;
        cerr << msg.str();


        try {
           WriteDataToCerr(sess.dataTimes, sess.dataBytesSaved);
        }
        catch (...) {
            cerr << "failed to write data to cerr \n";
        }
        sess.errorOccurred = true;
        cerr << "End of catch statement\n";
    }
}


int main(int argc, char *argv[]) {
    arma::arma_version ver;
    cout << "ARMA version: "<< ver.as_string() << endl;
    
    // Declare a listening 'Session'
    Session sess;
    Experiment exp;
    
    sess.UDP_IP = argv[1];                                         // IP address of data logger or simulator
    if (sess.UDP_IP == "self") {
        sess.UDP_IP = "127.0.0.1";
    }
    cout << "IP " << sess.UDP_IP << endl;
    sess.UDP_PORT = std::stoi(argv[2]);

    int firmwareVersion = std::stoi(argv[3]);
    exp.energyDetThresh = std::stod(argv[4]);


    cout << "Listening to IP address " << sess.UDP_IP.c_str() << " and port " << sess.UDP_PORT << endl;

    //import variables according to firmware version specified
    cout << "Firmware version: " << firmwareVersion << endl;
    if (firmwareVersion == 1550) {
        const string path = "config_files/1550_config.txt";
        if (ProcessFile(exp, path)) {
            cout  << "Error: Unable to open config file: " << path  << endl;
            return EXIT_FAILURE;
        }
        exp.ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else if (firmwareVersion == 1240) {
        const string path = "config_files/1240_config.txt";
        if (ProcessFile(exp, path)) {
            cout  << "Error: Unable to open config file: " << path  << endl;
            return EXIT_FAILURE;
        }
        exp.ProcessFncPtr = ProcessSegmentInterleaved;
    }
    else {
        cerr << "ERROR: Unknown firmware version" << endl;
        return EXIT_FAILURE;
    }
    
    exp.NUM_PACKS_DETECT = (int)(exp.TIME_WINDOW * 100000 / exp.SAMPS_PER_CHANNEL);
    exp.DATA_SEGMENT_LENGTH = exp.NUM_PACKS_DETECT * exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN; 

    cout << "HEAD_SIZE: "              << exp.HEAD_SIZE               << endl; 
    cout << "SAMPS_PER_CHAN: "         << exp.SAMPS_PER_CHANNEL       << endl;
    cout << "BYTES_PER_SAMP: "         << exp.BYTES_PER_SAMP          << endl;
    cout << "Bytes per packet:       " << exp.REQUIRED_BYTES         << endl;
    cout << "Time between packets:   " << exp.MICRO_INCR             << endl;
    cout << "Number of channels:     " << exp.NUM_CHAN               << endl;
    cout << "Data bytes per channel: " << exp.DATA_BYTES_PER_CHANNEL << endl;
    cout << "Detecting over a time window of " << exp.TIME_WINDOW << " seconds, using " << exp.NUM_PACKS_DETECT <<  " packets" << endl;

    // Define a vector of 3 elements
    Eigen::Vector3d v;
    // Initialize the vector with values
    v << 1, 2, 3;
    // Print the vector
    std::cout << "Eigen vector: " << v.transpose() << std::endl;
    

    while (true) {
        
        RestartListener(sess);
        
        std::thread listenerThread(UdpListener, std::ref(sess), exp.PACKET_SIZE);
        std::thread processorThread(DataProcessor, std::ref(sess), std::ref(exp));

        listenerThread.join();
        processorThread.join();
       
        if (sess.errorOccurred) {
            cout << "Restarting threads..." << endl;
        }
        else {
            cout << "Unknown problem occurred" << endl;
        }
        sess.errorOccurred = false;
    }
}