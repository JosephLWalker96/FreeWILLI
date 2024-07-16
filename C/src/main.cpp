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

#include "custom_types.h"
#include "process_data.h"
#include "TDOA_estimation.h"
#include "utils.h"
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

        // Read filter weights from file 
        vector<double> filterWeights = ReadFIRFilterFile(exp.filterWeights);
        
        // Convert filter coefficients to float
        vector<float> filterWeightsFloat(filterWeights.begin(), filterWeights.end());        
        
        // Declare time checking variables
        bool previousTimeSet = false;
        auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
        
        // pre-allocate memory for vectors
        sess.dataSegment.reserve(exp.DATA_SEGMENT_LENGTH);
        sess.dataTimes.reserve(exp.NUM_PACKS_DETECT);

        int paddedLength = filterWeightsFloat.size() + channelSize - 1;
        int fftOutputSize = (paddedLength / 2) + 1;

        static Eigen::VectorXf ch1(paddedLength);
        static Eigen::VectorXf ch2(paddedLength);
        static Eigen::VectorXf ch3(paddedLength);
        static Eigen::VectorXf ch4(paddedLength);
        
        Eigen::MatrixXcf savedFFTs(paddedLength, exp.NUM_CHAN); // save the FFT transformed channels
        Eigen::MatrixXcf savedFFTs_FFTW(fftOutputSize, exp.NUM_CHAN); // save the FFT transformed channels
        
        /* Zero-pad filter weights to the length of the signal                     */
        std::vector<float> paddedFilterWeights(paddedLength, 0.0);
        for (int i = 0; i < filterWeightsFloat.size(); ++i) {
            paddedFilterWeights[i] = filterWeightsFloat[i];
        }

        // FFT of filter
        Eigen::VectorXcf filterFreq(fftOutputSize);
        fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(paddedLength, paddedFilterWeights.data(), reinterpret_cast<fftwf_complex*>(filterFreq.data()), FFTW_ESTIMATE);
        fftwf_execute(fftFilter);
        
        // Container for pulling bytes from buffer (dataBuffer)
        vector<uint8_t> dataBytes;

        // FFT without sigpack
        exp.fftCh1 = fftwf_plan_dft_r2c_1d(paddedLength, ch1.data(), reinterpret_cast<fftwf_complex*>(savedFFTs_FFTW.col(0).data()), FFTW_ESTIMATE);
        exp.fftCh2 = fftwf_plan_dft_r2c_1d(paddedLength, ch2.data(), reinterpret_cast<fftwf_complex*>(savedFFTs_FFTW.col(1).data()), FFTW_ESTIMATE);
        exp.fftCh3 = fftwf_plan_dft_r2c_1d(paddedLength, ch3.data(), reinterpret_cast<fftwf_complex*>(savedFFTs_FFTW.col(2).data()), FFTW_ESTIMATE);
        exp.fftCh4 = fftwf_plan_dft_r2c_1d(paddedLength, ch4.data(), reinterpret_cast<fftwf_complex*>(savedFFTs_FFTW.col(3).data()), FFTW_ESTIMATE);
        
        // set the frequency of file writes
        const std::chrono::milliseconds FLUSH_INTERVAL(1000);
        const size_t BUFFER_SIZE_THRESHOLD = 1000; // Adjust as needed
        auto lastFlushTime = std::chrono::steady_clock::now();
        
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

                // Convert byte data to floats
                auto startCDTime = std::chrono::steady_clock::now();
                ConvertData(sess.dataSegment, dataBytes, exp.DATA_SIZE, exp.HEAD_SIZE); // bytes data is decoded and appended to sess.dataSegment
                auto endCDTime = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationCD = endCDTime - startCDTime;
                //cout << "Convert data: " << durationCD.count() << endl;
                
                auto startTimestamps = std::chrono::steady_clock::now();
                GenerateTimestamps(sess.dataTimes, dataBytes, exp.MICRO_INCR, previousTimeSet, previousTime , exp.detectionOutputFile, exp.tdoaOutputFile, exp.doaOutputFile);
                auto endTimestamps = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationGenerate = endTimestamps - startTimestamps;
                //cout << "durationGenerate: " << durationGenerate.count() << endl;

                // Check if the amount of bytes in packet is what is expected
                if (dataBytes.size() != exp.PACKET_SIZE) {
                    std::stringstream msg; // compose message to dispatch
                    msg << "Error: incorrect number of bytes in packet: " <<  "PACKET_SIZE: " << exp.PACKET_SIZE << " dataBytes size: " << dataBytes.size() << endl;
                    throw std::runtime_error(msg.str());
                }

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
            
            auto beforeFFTWF = std::chrono::steady_clock::now();
            fftwf_execute(exp.fftCh1);
            fftwf_execute(exp.fftCh2);
            fftwf_execute(exp.fftCh3);
            fftwf_execute(exp.fftCh4);
            auto afterFFTWF = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFFTWF = afterFFTWF - beforeFFTWF;
            cout << "FFT time: " << durationFFTWF.count() << endl;
           
            
            auto beforeFFTW = std::chrono::steady_clock::now();
            savedFFTs_FFTW.col(0) = savedFFTs_FFTW.col(0).array() * filterFreq.array();
            savedFFTs_FFTW.col(1) = savedFFTs_FFTW.col(1).array() * filterFreq.array();
            savedFFTs_FFTW.col(2) = savedFFTs_FFTW.col(2).array() * filterFreq.array();
            savedFFTs_FFTW.col(3) = savedFFTs_FFTW.col(3).array() * filterFreq.array();
            auto afterFFTW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFFTW = afterFFTW - beforeFFTW;
            cout << "FFT filter time: " << durationFFTW.count() << endl;
            
            auto beforeGCCW = std::chrono::steady_clock::now();
            Eigen::VectorXf resultMatrix = GCC_PHAT_FFTW_E(savedFFTs_FFTW, exp.inverseFFT, exp.interp, paddedLength, exp.NUM_CHAN, exp.SAMPLE_RATE);
            auto afterGCCW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationGCCW = afterGCCW - beforeGCCW;
            //cout << "Eigen C GCC: " << durationGCCW.count() << endl;
            
            Eigen::VectorXf DOAs = DOA_EstimateVerticalArray(resultMatrix, exp.speedOfSound, exp.chanSpacing);
            cout << "DOA_FFTs: " << DOAs.transpose() << endl;
           
            // Write to buffers
            sess.peakAmplitudeBuffer.push_back(detResult.peakAmplitude);
            sess.peakTimesBuffer.push_back(detResult.peakTimes);
            sess.resultMatrixBuffer.push_back(resultMatrix);
            sess.DOAsBuffer.push_back(DOAs);

            auto currentTime = std::chrono::steady_clock::now();
            if (sess.peakAmplitudeBuffer.size() >= BUFFER_SIZE_THRESHOLD) {//|| currentTime - lastFlushTime >= FLUSH_INTERVAL) {
                cout << "Flushing buffers of length: " << sess.peakAmplitudeBuffer.size() << endl;

                auto beforeW = std::chrono::steady_clock::now();
                WritePulseAmplitudes(sess.peakAmplitudeBuffer, sess.peakTimesBuffer, exp.detectionOutputFile);
                auto afterW = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationW = afterW - beforeW;
                cout << "Write: " << durationW.count() << endl;

                auto beforeW1 = std::chrono::steady_clock::now();
                WriteArray(sess.resultMatrixBuffer, sess.peakTimesBuffer, exp.tdoaOutputFile);
                auto afterW1 = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationW1 = afterW1 - beforeW1;
                cout << "Write1: " << durationW1.count() << endl;

                auto beforeW2 = std::chrono::steady_clock::now();
                WriteArray(sess.DOAsBuffer, sess.peakTimesBuffer, exp.doaOutputFile);
                auto afterW2 = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationW2 = afterW2 - beforeW2;
                cout << "Write:2 " << durationW2.count() << endl;
                
                lastFlushTime = currentTime;
                sess.peakAmplitudeBuffer.clear();
                sess.peakTimesBuffer.clear();
                sess.resultMatrixBuffer.clear();
                sess.DOAsBuffer.clear();
            
            }
        }
    } 
    catch (const GCC_Value_Error& e) {
        
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
    }
    catch (const std::ios_base::failure& e) {
        std::stringstream msg; // compose message to dispatch
        msg << e.what() << endl;
        cerr << msg.str();
        std::exit(1);
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
    const string path = "config_files/" + std::to_string(firmwareVersion) + "_config.txt";
    if (ProcessFile(exp, path)) {
        cout  << "Error: Unable to open config file: " << path  << endl;
        std::exit(1);
    }
    exp.ProcessFncPtr = ProcessSegmentInterleaved;
    
    exp.NUM_PACKS_DETECT = (int)(exp.TIME_WINDOW * 100000 / exp.SAMPS_PER_CHANNEL);
    exp.DATA_SEGMENT_LENGTH = exp.NUM_PACKS_DETECT * exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN; 

    cout << "HEAD_SIZE: "              << exp.HEAD_SIZE               << endl; 
    cout << "SAMPS_PER_CHAN: "         << exp.SAMPS_PER_CHANNEL       << endl;
    cout << "BYTES_PER_SAMP: "         << exp.BYTES_PER_SAMP          << endl;
    cout << "Bytes per packet:       " << exp.REQUIRED_BYTES          << endl;
    cout << "Time between packets:   " << exp.MICRO_INCR              << endl;
    cout << "Number of channels:     " << exp.NUM_CHAN                << endl;
    cout << "Data bytes per channel: " << exp.DATA_BYTES_PER_CHANNEL  << endl;
    cout << "Detecting over a time window of " << exp.TIME_WINDOW << " seconds, using " << exp.NUM_PACKS_DETECT <<  " packets" << endl;


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
        
        // Destroy FFTWF objects
        fftwf_destroy_plan(exp.fftCh1);
        fftwf_destroy_plan(exp.fftCh2);
        fftwf_destroy_plan(exp.fftCh3);
        fftwf_destroy_plan(exp.fftCh4);
        exp.fftCh1 = nullptr;
        exp.fftCh2 = nullptr;
        exp.fftCh3 = nullptr;
        exp.fftCh4 = nullptr;
        
        fftwf_destroy_plan(exp.inverseFFT);
        exp.inverseFFT = nullptr;

        // Destroy FIR filter objects
        //firfilt_rrrf_destroy(exp.firFilterCh1);
        //firfilt_rrrf_destroy(exp.firFilterCh2);
        //firfilt_rrrf_destroy(exp.firFilterCh3);
        //firfilt_rrrf_destroy(exp.firFilterCh4);
        //exp.firFilterCh1 = nullptr;
        //exp.firFilterCh2 = nullptr;
        //exp.firFilterCh3 = nullptr;
        //exp.firFilterCh4 = nullptr;

        sess.errorOccurred = false;
    }
}
