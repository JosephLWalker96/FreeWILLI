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
#include "pch.h"
using std::cout;
using std::cin;
using std::endl;
using std::cerr;
using namespace std::chrono_literals;

// Global variables (used for manual testing and logging to console)
int packetCounter = 0; // this should only be used inside the UDPListener function, as it is not protected by a mutex
int detectionCounter = 0;
bool test = true;

#ifdef PICO
SemaphoreHandle_t deviceInitSemaphore; // For signaling Wi-Fi init completion
FreeRTOSMutex listenerMutex;
FreeRTOSMutex prosessorMutex;
#define MAIN_TASK_PRIORITY     ( tskIDLE_PRIORITY + 3)
#define LISTEN_TASK_PRIORITY   ( tskIDLE_PRIORITY + 3)
#define PROCESS_TASK_PRIORITY    ( tskIDLE_PRIORITY + 3)
#define BLINK_TASK_PRIORITY    ( tskIDLE_PRIORITY + 1 )

#define LISTENER_DONE_BIT (1 << 0)  // Bit 0
#define PROCESSOR_DONE_BIT (1 << 1)  // Bit 1
EventGroupHandle_t xEventGroup = xEventGroupCreate();

#endif

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
#ifdef PICO
    printf("Started Initialization at UDP Listener\n");
//    size_t currentFreeHeap = xPortGetFreeHeapSize();
//    size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();
//    printf("Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
//           currentFreeHeap, minFreeHeap);
//    printf("Current task stack high water mark: %u words\n", uxTaskGetStackHighWaterMark(NULL));
    struct sockaddr_in addr;
    socklen_t addrLength = sizeof(addr);
    ssize_t bytesReceived;
    const int printInterval = 500;
    const int receiveSize = PACKET_SIZE + 1;              // + 1 to detect if more data than expected is being received
    size_t queueSize;
    auto startPacketTime = std::chrono::steady_clock::now();
    auto endPacketTime = startPacketTime;
    std::chrono::duration<double> durationPacketTime;     // stores the average amount of time (seconds) between successive UDP packets, averaged over 'printInterval' packets
    std::vector<uint8_t> dataBytes(receiveSize);
    printf("Finished Initialization in UDP Listener\n");
    // Monitor heap usage
//    currentFreeHeap = xPortGetFreeHeapSize();
//    minFreeHeap = xPortGetMinimumEverFreeHeapSize();
//    printf("Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
//           currentFreeHeap, minFreeHeap);
//    printf("Current task stack high water mark: %u words\n", uxTaskGetStackHighWaterMark(NULL));

    printf("Start waiting for input...\n");
    while (!sess.errorOccurred) {
//        printf("Waiting for input...\n");
        bytesReceived = recvfrom(
            sess.datagramSocket,
            dataBytes.data(),
            receiveSize,
            0,
            (struct sockaddr*)&addr,
            &addrLength
        );
//        printf("%d ", bytesReceived);

//        if (bytesReceived == -1) {
//            std::cerr << "Error in recvfrom: bytesReceived is -1" << std::endl;
//            sess.errorOccurred = true;
//            vTaskDelay(pdMS_TO_TICKS(10));
//            break;
//        } else if (bytesReceived == 0) {
//            vTaskDelay(pdMS_TO_TICKS(10));
//        }else {
//            printf("byte received!");
//        }
        if (bytesReceived > 0){
//            printf("byte received!\n");

            dataBytes.resize(bytesReceived);         // Adjust size based on actual bytes received

            sess.dataBufferLock.lock();              // give this thread exclusive rights to modify the shared dataBytes variable
            sess.dataBuffer.push(dataBytes);
            queueSize = sess.dataBuffer.size();
//            cout << "queue size: "+std::to_string(queueSize) <<endl;
            sess.dataBufferLock.unlock();            // relinquish exclusive rights

//            if (queueSize == 10)
//                break;

            packetCounter += 1;
            if (packetCounter % printInterval == 0) {
                endPacketTime = std::chrono::steady_clock::now();
                durationPacketTime = endPacketTime - startPacketTime;
                std::stringstream msg; // compose message to dispatch
                msg << "Num packets received is " <<  packetCounter << " " << durationPacketTime.count() / printInterval
                    << " " << queueSize << " " << packetCounter - queueSize << " " << detectionCounter << std::endl;
                std::cout << msg.str(); // using one instance of "<<" makes the operation atomic
                startPacketTime = std::chrono::steady_clock::now();
            }

            if (queueSize > 100) { // check if buffer has grown to an unacceptable size
                std::cerr << "Buffer overflowing" << std::endl;
                sess.dataBufferLock.lock();
                ClearQueue(sess.dataBuffer);
                sess.dataBufferLock.unlock();
                sess.errorOccurred = true;
                break;
            }
        }
        // Monitor heap usage
//        currentFreeHeap = xPortGetFreeHeapSize();
//        minFreeHeap = xPortGetMinimumEverFreeHeapSize();
//        printf("Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
//               currentFreeHeap, minFreeHeap);
//        printf("Current task stack high water mark: %u words\n", uxTaskGetStackHighWaterMark(NULL));
//        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (sess.errorOccurred) {
        std::cerr << "Error occurred in UDP Listener Thread" << std::endl;
    }

    xEventGroupSetBits(xEventGroup, LISTENER_DONE_BIT);
#else
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
        std::vector<uint8_t> dataBytes(receiveSize);

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
#endif
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
#ifdef PICO
   // the number of samples per channel within a dataSegment
    int channelSize = exp.DATA_SEGMENT_LENGTH / exp.NUM_CHAN;

    // Read filter weights from file
//    std::vector<double> filterWeights = ReadFIRFilterFile(exp.filterWeights);
    std::vector<float> filterWeightsFloat = {
            8.5304705e-18, -1.2040846e-03, -2.7904883e-03, -4.2366693e-03,
            -3.9514871e-03, -9.6724173e-18,  8.2750460e-03,  1.8624326e-02,
            2.5445996e-02,  2.1282293e-02,  2.4025036e-17, -3.9675705e-02,
            -9.2092186e-02, -1.4542012e-01, -1.8531199e-01,  8.0040973e-01,
            -1.8531199e-01, -1.4542012e-01, -9.2092186e-02, -3.9675705e-02,
            2.4025036e-17,  2.1282293e-02,  2.5445996e-02,  1.8624326e-02,
            8.2750460e-03, -9.6724173e-18, -3.9514871e-03, -4.2366693e-03,
            -2.7904883e-03, -1.2040846e-03,  8.5304705e-18
    };

    // Declare time checking variables
    bool previousTimeSet = false;
    auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();

    // pre-allocate memory for vectors
    if (exp.DATA_SEGMENT_LENGTH > sess.dataSegment.max_size()) {
        std::cout << "dataSegment Error: Requested size exceeds the maximum allowable size for the vector." << std::endl;
        sess.errorOccurred = true;
    } else {
        sess.dataSegment.reserve(exp.DATA_SEGMENT_LENGTH);
    }

    if (exp.NUM_PACKS_DETECT > sess.dataTimes.max_size()) {
        std::cout << "dataTimes Error: Requested size exceeds the maximum allowable size for the vector." << std::endl;
        sess.errorOccurred = true;
    } else {
        sess.dataTimes.reserve(exp.NUM_PACKS_DETECT);
    }

    int paddedLength = filterWeightsFloat.size() + channelSize - 1;
    int fftOutputSize = (paddedLength / 2) + 1;
    cout << "Padded size: " << paddedLength << endl;

    // Matrices for (transformed) channel data
    static Eigen::MatrixXf channelData(paddedLength, exp.NUM_CHAN);
    static Eigen::MatrixXcf savedFFTs(fftOutputSize, exp.NUM_CHAN); // save the FFT transformed channels

    /* Zero-pad filter weights to the length of the signal                     */
    std::vector<float> paddedFilterWeights(paddedLength, 0.0f);
    std::copy(filterWeightsFloat.begin(),filterWeightsFloat.end(),paddedFilterWeights.begin());
    delete filterWeightsFloat.data();

    // Create frequency domain filter
    Eigen::VectorXcf filterFreq(fftOutputSize);
    fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(paddedLength, paddedFilterWeights.data(), reinterpret_cast<fftwf_complex*>(filterFreq.data()), FFTW_ESTIMATE);
    fftwf_execute(fftFilter);
    fftwf_destroy_plan(fftFilter);
    delete paddedFilterWeights.data();

    // Container for pulling bytes from buffer (dataBuffer)
    std::vector<uint8_t> dataBytes;

    // Create FFTW objects for channel data
    exp.fftForChannels.resize(exp.NUM_CHAN);
    for (int i = 0; i < exp.NUM_CHAN; i++) {
        exp.fftForChannels[i] = fftwf_plan_dft_r2c_1d(paddedLength, channelData.col(i).data(), reinterpret_cast<fftwf_complex*>(savedFFTs.col(i).data()), FFTW_ESTIMATE);
    }

    // set the frequency of file writes
    const std::chrono::milliseconds FLUSH_INTERVAL(1000);
    const size_t BUFFER_SIZE_THRESHOLD = 1000; // Adjust as needed
    auto lastFlushTime = std::chrono::steady_clock::now();

    vTaskDelay(pdMS_TO_TICKS(10));

    while (!sess.errorOccurred) {

        sess.dataTimes.clear();
        sess.dataSegment.clear();
        sess.dataBytesSaved.clear();

        if (sess.dataSegment.size() >= exp.DATA_SEGMENT_LENGTH) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        while (sess.dataSegment.size() < exp.DATA_SEGMENT_LENGTH) {

            auto startLoop = std::chrono::steady_clock::now();

            sess.dataBufferLock.lock();   // give this thread exclusive rights to modify the shared dataBytes variable
            size_t queueSize = sess.dataBuffer.size();
            if (queueSize < 1) {
                sess.dataBufferLock.unlock();
                //cout << "Sleeping: " << endl;
               // std::this_thread::sleep_for(80ms);
//                while (true)
//                    printf("%d\n", exp.DATA_SEGMENT_LENGTH);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            else {
                dataBytes = sess.dataBuffer.front();
                sess.dataBuffer.pop();
                // cout << "getting " << sess.dataSegment.size() << " bytes so far" << endl;
                sess.dataBufferLock.unlock();
            }

//            sess.dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error

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
                std::cerr << msg.str() << std::endl;
            }

        }

        /*
         *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
         *   now apply energy detector.
         */

        auto beforePtr = std::chrono::steady_clock::now();
        exp.ProcessFncPtr(sess.dataSegment, channelData, exp.NUM_CHAN);
        auto afterPtr = std::chrono::steady_clock::now();
        std::chrono::duration<double> durationPtr = afterPtr - beforePtr;


        DetectionResult detResult = ThresholdDetect(channelData.col(0), sess.dataTimes, exp.energyDetThresh, exp.SAMPLE_RATE);

        if (detResult.maxPeakIndex < 0){  // if no pulse was detected (maxPeakIndex remains -1) stop processing
            continue;                  // get next dataSegment; return to loop
        }

        /*
         *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
         */

        detectionCounter++;
        std::cout<< "Detected!" << std::endl;

        auto beforeFFTWF = std::chrono::steady_clock::now();
        for (auto& plan : exp.fftForChannels) {
            fftwf_execute(plan);
        }
        auto afterFFTWF = std::chrono::steady_clock::now();
        std::chrono::duration<double> durationFFTWF = afterFFTWF - beforeFFTWF;
        cout << "FFT time: " << durationFFTWF.count() << endl;

        auto beforeFFTW = std::chrono::steady_clock::now();
        for (int i = 0; i < exp.NUM_CHAN; i++) {
            savedFFTs.col(i) = savedFFTs.col(i).array() * filterFreq.array();
        }
        auto afterFFTW = std::chrono::steady_clock::now();
        std::chrono::duration<double> durationFFTW = afterFFTW - beforeFFTW;
        cout << "FFT filter time: " << durationFFTW.count() << endl;
        // printSysMemory();

        auto beforeGCCW = std::chrono::steady_clock::now();
        Eigen::VectorXf resultMatrix = GCC_PHAT_FFTW(savedFFTs, exp.inverseFFT, exp.interp, paddedLength, exp.NUM_CHAN, exp.SAMPLE_RATE);
        auto afterGCCW = std::chrono::steady_clock::now();
        std::chrono::duration<double> durationGCCW = afterGCCW - beforeGCCW;
        cout << "GCC time: " << durationGCCW.count() << endl;
        // printSysMemory();

        Eigen::VectorXf DOAs = DOA_EstimateVerticalArray(resultMatrix, exp.speedOfSound, exp.chanSpacing);
        // cout << "DOAs: " << DOAs.transpose() << endl;

        // Write to buffers
        sess.peakAmplitudeBuffer.push_back(detResult.peakAmplitude);
        sess.peakTimesBuffer.push_back(detResult.peakTimes);
        sess.resultMatrixBuffer.push_back(resultMatrix);
        sess.DOAsBuffer.push_back(DOAs);

        auto currentTime = std::chrono::steady_clock::now();
        if (sess.peakAmplitudeBuffer.size() >= BUFFER_SIZE_THRESHOLD) {//|| currentTime - lastFlushTime >= FLUSH_INTERVAL) {
            cout << "Flushing buffers of length: " << sess.peakAmplitudeBuffer.size() << endl;

            auto beforeW = std::chrono::steady_clock::now();
            // print data rows
            for (size_t i = 0; i < sess.peakAmplitudeBuffer.size(); ++i) {
                auto time_point = sess.peakTimesBuffer[i];
                auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
                cout << time_since_epoch.count() << std::setw(20) << sess.peakAmplitudeBuffer[i] << endl;
            }

            auto afterW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationW = afterW - beforeW;
            cout << "Write: " << durationW.count() << endl;

            auto beforeW1 = std::chrono::steady_clock::now();
            cout << "tdoa out" <<endl;
            for (int row = 0; row < sess.resultMatrixBuffer.size(); row++){
                auto time_point = sess.peakTimesBuffer[row];
                auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());

                cout << time_since_epoch.count() << std::setw(20);
                for (int i = 0; i < sess.resultMatrixBuffer[row].size(); ++i) {
                    cout << sess.resultMatrixBuffer[row][i] << " ";
                }
                cout << std::endl;
            }

            auto afterW1 = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationW1 = afterW1 - beforeW1;
            cout << "Write1: " << durationW1.count() << endl;

            auto beforeW2 = std::chrono::steady_clock::now();
            cout << "doa out" <<endl;
            for (int row = 0; row < sess.DOAsBuffer.size(); row++){
                auto time_point = sess.peakTimesBuffer[row];
                auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());

                cout << time_since_epoch.count() << std::setw(20);
                for (int i = 0; i < sess.DOAsBuffer[row].size(); ++i) {
                    cout << sess.DOAsBuffer[row][i] << " ";
                }
                cout << std::endl;
            }

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
#else
    try {

        // the number of samples per channel within a dataSegment
        int channelSize = exp.DATA_SEGMENT_LENGTH / exp.NUM_CHAN; 
        
        // Read filter weights from file 
        std::vector<double> filterWeights = ReadFIRFilterFile(exp.filterWeights);
        
        // Convert filter coefficients to float
        std::vector<float> filterWeightsFloat(filterWeights.begin(), filterWeights.end());        
        
        // Declare time checking variables
        bool previousTimeSet = false;
        auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
        
        // pre-allocate memory for vectors
        sess.dataSegment.reserve(exp.DATA_SEGMENT_LENGTH);
        sess.dataTimes.reserve(exp.NUM_PACKS_DETECT);

        int paddedLength = filterWeightsFloat.size() + channelSize - 1;
        int fftOutputSize = (paddedLength / 2) + 1;
        cout << "Padded size: " << paddedLength << endl;
        
        // Matrices for (transformed) channel data
        static Eigen::MatrixXf channelData(paddedLength, exp.NUM_CHAN);
        static Eigen::MatrixXcf savedFFTs(fftOutputSize, exp.NUM_CHAN); // save the FFT transformed channels
        
        /* Zero-pad filter weights to the length of the signal                     */
        std::vector<float> paddedFilterWeights(paddedLength, 0.0f);
        std::copy(filterWeightsFloat.begin(),filterWeightsFloat.end(),paddedFilterWeights.begin());

        // Create frequency domain filter
        Eigen::VectorXcf filterFreq(fftOutputSize);
        fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(paddedLength, paddedFilterWeights.data(), reinterpret_cast<fftwf_complex*>(filterFreq.data()), FFTW_ESTIMATE);
        fftwf_execute(fftFilter);
        fftwf_destroy_plan(fftFilter);
        
        // Container for pulling bytes from buffer (dataBuffer)
        std::vector<uint8_t> dataBytes;

        // Create FFTW objects for channel data
        exp.fftForChannels.resize(exp.NUM_CHAN);
        for (int i = 0; i < exp.NUM_CHAN; i++) {
            exp.fftForChannels[i] = fftwf_plan_dft_r2c_1d(paddedLength, channelData.col(i).data(), reinterpret_cast<fftwf_complex*>(savedFFTs.col(i).data()), FFTW_ESTIMATE);
        }
        
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
                    std::this_thread::sleep_for(80ms);
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
            
            auto beforePtr = std::chrono::steady_clock::now();
            exp.ProcessFncPtr(sess.dataSegment, channelData, exp.NUM_CHAN);
            auto afterPtr = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationPtr = afterPtr - beforePtr;

            
            DetectionResult detResult = ThresholdDetect(channelData.col(0), sess.dataTimes, exp.energyDetThresh, exp.SAMPLE_RATE);

            if (detResult.maxPeakIndex < 0){  // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue;                  // get next dataSegment; return to loop
            }
            
            /*
             *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
             */
            
            detectionCounter++;
            
            auto beforeFFTWF = std::chrono::steady_clock::now();
            for (auto& plan : exp.fftForChannels) {
                fftwf_execute(plan);
            }
            auto afterFFTWF = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFFTWF = afterFFTWF - beforeFFTWF;
            cout << "FFT time: " << durationFFTWF.count() << endl;

            auto beforeFFTW = std::chrono::steady_clock::now();
            for (int i = 0; i < exp.NUM_CHAN; i++) {
                savedFFTs.col(i) = savedFFTs.col(i).array() * filterFreq.array();
            }
            auto afterFFTW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFFTW = afterFFTW - beforeFFTW;
            cout << "FFT filter time: " << durationFFTW.count() << endl;
            
            auto beforeGCCW = std::chrono::steady_clock::now();
            Eigen::VectorXf resultMatrix = GCC_PHAT_FFTW(savedFFTs, exp.inverseFFT, exp.interp, paddedLength, exp.NUM_CHAN, exp.SAMPLE_RATE);
            auto afterGCCW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationGCCW = afterGCCW - beforeGCCW;
            cout << "GCC time: " << durationGCCW.count() << endl;
            
            Eigen::VectorXf DOAs = DOA_EstimateVerticalArray(resultMatrix, exp.speedOfSound, exp.chanSpacing);
            cout << "DOAs: " << DOAs.transpose() << endl;
           
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
#endif

#ifdef PICO
    if (sess.errorOccurred) {
        sess.dataBufferLock.unlock();
        sess.peakAmplitudeBuffer.clear();
        sess.peakTimesBuffer.clear();
        sess.resultMatrixBuffer.clear();
        sess.DOAsBuffer.clear();
    }
    xEventGroupSetBits(xEventGroup, PROCESSOR_DONE_BIT);
#endif
}

#ifdef PICO
void PrintAllTasksStackUsage(void)
{
    UBaseType_t uxTaskCount = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxTaskCount * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL)
    {
        UBaseType_t uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxTaskCount, NULL);

        printf("Task Name\tStack High Water Mark (words)\n");
        for (UBaseType_t i = 0; i < uxArraySize; i++)
        {
            printf("%s\t%u\n", pxTaskStatusArray[i].pcTaskName, pxTaskStatusArray[i].usStackHighWaterMark);
        }

        vPortFree(pxTaskStatusArray);
    }
    else
    {
        printf("Error: Could not allocate memory for task status array.\n");
    }
}
void vMemoryMonitorTask(void *pvParameters)
{
    for (;;)
    {
        // Monitor heap usage
        size_t currentFreeHeap = xPortGetFreeHeapSize();
        size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();

        printf("Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
               currentFreeHeap, minFreeHeap);

        // Monitor stack usage for all tasks
        PrintAllTasksStackUsage();

        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for monitoring interval
    }
}


void BlinkTask(__unused void *params){
    // Use FreeRTOS delay so that the OS can context switch to other tasks
    while (true) {
        int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        if (link_status != CYW43_LINK_UP) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
            printf("Wi-Fi disconnected, attempting to reconnect...\n");

//            const char* ssid = "3187D";
//            const char* password = "inputrain524";
//            const char* ssid = "SpectrumSetup-D8";
//            const char* password = "urbanfarmer157";
//            const char* ssid = "SSH";
//            const char* password = "19990114";
            const char* ssid = "LeoZ";
            const char* password = "LZ19990114";
            while (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
                printf("Failed to reconnect. Retrying...\n");
                vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds before retrying
            }
            printf("Reconnected to Wi-Fi!\n");
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // Handle stack overflow (e.g., log error, reset system)
    printf("Stack overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for(;;); // Loop forever to halt the system
}

void  vApplicationMallocFailedHook(){
    printf("Malloc Failed\n");
    taskDISABLE_INTERRUPTS();
    for(;;); // Loop forever to halt the system
}

void UdpListenerTask(void* pvParameters) {
    printf("Started Creating Listener Task\n");
    auto* params = static_cast<std::tuple<Session*, unsigned int>*>(pvParameters);
    UdpListener(*std::get<0>(*params), std::get<1>(*params));
    printf("Deleting Listener Task\n");
    vTaskDelete(NULL);
}

void DataProcessorTask(void* pvParameters) {
    printf("Started Creating Processor Task\n");
    auto* params = static_cast<std::tuple<Session*, Experiment*>*>(pvParameters);
    DataProcessor(*std::get<0>(*params), *std::get<1>(*params));
    printf("Deleting Processor Task\n");
    vTaskDelete(NULL);
}

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
               result->ssid, result->rssi, result->channel,
               result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
               result->auth_mode);
    }
    return 0;
}

void MainTask(__unused void* pvParameters){
//    size_t currentFreeHeap = xPortGetFreeHeapSize();
//    size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();
//    printf("Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
//           currentFreeHeap, minFreeHeap);

    Session sess;
    if (sess.datagramSocket < 0) {
        printf("Error creating socket\n");
        return;
    }

//    currentFreeHeap = xPortGetFreeHeapSize();
//    minFreeHeap = xPortGetMinimumEverFreeHeapSize();
//    printf("After socket Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
//           currentFreeHeap, minFreeHeap);

    Experiment exp;

//    currentFreeHeap = xPortGetFreeHeapSize();
//    minFreeHeap = xPortGetMinimumEverFreeHeapSize();
//    printf("After exp Current Free Heap: %u bytes, Minimum Ever Free Heap: %u bytes\n",
//           currentFreeHeap, minFreeHeap);

//    exp.energyDetThresh = 120;
    exp.HEAD_SIZE = 12;
    exp.MICRO_INCR = 1240;
    exp.NUM_CHAN = 4;
    exp.SAMPS_PER_CHANNEL = 124;
    exp.BYTES_PER_SAMP = 2;
    exp.DATA_SIZE = exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN * exp.BYTES_PER_SAMP;
    exp.PACKET_SIZE = exp.HEAD_SIZE + exp.DATA_SIZE;
    exp.REQUIRED_BYTES = exp.DATA_SIZE + exp.HEAD_SIZE;
    exp.DATA_BYTES_PER_CHANNEL = exp.SAMPS_PER_CHANNEL * exp.BYTES_PER_SAMP;

    sess.UDP_IP = "127.0.0.1";                                         // IP address of data logger or simulator
    sess.UDP_PORT = 1045;
    int firmwareVersion = 1240;
//    exp.energyDetThresh = 2500;
    exp.energyDetThresh = 120;

//    cout << "Listening to IP address " << sess.UDP_IP.c_str() << " and port " << sess.UDP_PORT << endl;

    //import variables according to firmware version specified
    cout << "Firmware version: " << firmwareVersion << endl;
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
    cout << "Packet Size: " << exp.PACKET_SIZE << endl;
    cout << "Detecting over a time window of " << exp.TIME_WINDOW << " seconds, using " << exp.NUM_PACKS_DETECT <<  " packets" << endl;

    TaskHandle_t blinkHandle;
    auto blinkRslt = xTaskCreate(BlinkTask, "BlinkTask", configMINIMAL_STACK_SIZE, nullptr, BLINK_TASK_PRIORITY, &blinkHandle);
    if (blinkRslt != pdPASS) {
        printf("Error creating BlinkTask: %ld\n",  blinkRslt);
        for(;;); // Loop forever to halt the system
    }

    // set the task to run on core 0
    vTaskCoreAffinitySet(blinkHandle, ( 1 << 0 ));

//    auto memoryMinitorRslt = xTaskCreate(vMemoryMonitorTask, "MemoryMonitorTask", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY,
//                                         nullptr);
//    if (memoryMinitorRslt != pdPASS){
//        printf("Error creating MemoryMonitorTask: %ld\n", memoryMinitorRslt);
//        for(;;); // Loop forever to halt the system
//    }

    while (true) {
        cout << "Restarting Threads ..." << endl;

        RestartListener(sess);
        // printSysMemory();
        std::cout<<"Check Unlock =" << sess.dataBufferLock.isUnlock() <<std::endl;
        std::tuple<Session *, unsigned int> listenerParams = std::make_tuple(&sess, exp.PACKET_SIZE);
        std::tuple<Session *, Experiment *> processorParams = std::make_tuple(&sess, &exp);

        TaskHandle_t listenHandle;
        auto listenRslt = xTaskCreate(UdpListenerTask, "UdpListenerTask", 2048, &listenerParams, LISTEN_TASK_PRIORITY, &listenHandle);
        if (listenRslt != pdPASS) {
            printf("Error creating ListenTask: %ld\n", listenRslt);
            for(;;); // Loop forever to halt the system
        }
        // set the listen task to run on core 1 (core 1 only do listen)
        vTaskCoreAffinitySet(listenHandle, (1<<1));

        TaskHandle_t processHandle;
        auto processRslt = xTaskCreate(DataProcessorTask, "DataProcessorTask", 2048, &processorParams, PROCESS_TASK_PRIORITY, &processHandle);
        if (processRslt != pdPASS){
            printf("Error creating ListenTask: %ld\n", processRslt);
            for(;;); // Loop forever to halt the system
        }
        // set the data processor to run on core 0
        vTaskCoreAffinitySet(processHandle, (1<<0));


        // tasks join
        EventBits_t uxBits = xEventGroupWaitBits(
                xEventGroup,
                LISTENER_DONE_BIT |            // Wait for listener to complete
                PROCESSOR_DONE_BIT,        // Wait for processor to complete
                pdTRUE,                        // Clear bits on exit
                pdTRUE,                        // Wait for all bits to be set
                portMAX_DELAY                  // Wait indefinitely
        );

        if (
            (uxBits & (LISTENER_DONE_BIT | PROCESSOR_DONE_BIT)) ==
            (LISTENER_DONE_BIT | PROCESSOR_DONE_BIT)
//                (uxBits & LISTENER_DONE_BIT )== LISTENER_DONE_BIT
            ) {
            printf("All worker tasks have completed. Main task continues.\n");
            if (eTaskGetState(listenHandle) != eDeleted) {
                vTaskDelete(listenHandle);
                std::cout << "listener deleted!" << std::endl;
            }

            if (eTaskGetState(processHandle) != eDeleted) {
                vTaskDelete(processHandle);
                std::cout << "processor deleted!" << std::endl;
            }
            printf("All tasks is deleted\n");
        }


        if (sess.errorOccurred) {
            cout << "Error Occur!" << endl;
        }
        else {
            cout << "Unknown problem occurred" << endl;
        }

        // Destroy FFTWF objects
        for (auto& plan : exp.fftForChannels) {
            fftwf_destroy_plan(plan);
            plan = nullptr;
        }

        fftwf_destroy_plan(exp.inverseFFT);
        exp.inverseFFT = nullptr;

        sess.errorOccurred = false;
    }
}


void Cyw43ArchInitTask(void *pvParameters){
    // Initialize the Wi-Fi driver
    if (cyw43_arch_init()) {
        while (true)
            printf("Failed to initialize Wi-Fi driver\n");
    } else {
        printf("Wi-Fi driver initialized successfully\n");
    }

    // Signal that Wi-Fi initialization is complete
    xSemaphoreGive(deviceInitSemaphore);
    vTaskDelete(NULL);  // Delete the task after initialization
}

void WiFiInitTask(void *pvParameters){

    if (xSemaphoreTake(deviceInitSemaphore, portMAX_DELAY) == pdTRUE){
        cyw43_arch_enable_sta_mode();

//        const char* ssid = "SpectrumSetup-D8";
//        const char* password = "urbanfarmer157";
      const char* ssid = "LeoZ";
      const char* password = "LZ19990114";
//      const char* ssid = "3187D";
//      const char* password = "inputrain524";
//      const char* ssid = "SSH";
//      const char* password = "19990114";


        while (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("scanning wifi.\n");
            cyw43_wifi_scan_options_t scan_options = {0};
            cyw43_wifi_scan(&cyw43_state, &scan_options, nullptr,
                            [](void *env, const cyw43_ev_scan_result_t *result)->int{
                                if (result) {
                                    printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
                                           result->ssid, result->rssi, result->channel,
                                           result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
                                           result->auth_mode);
                                }
                                return 0;
                            }
            );
            printf("failed to connect.\n");
            printf("try again. \n");
        }
        printf("Connected to Wi-Fi!\n");

        int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        printf("Link status: %d\n", link_status);

        TaskHandle_t mainHandle;
        auto main_rslt = xTaskCreate(
                MainTask,
                "MainThread",
                1024, // this is the size for the stack, and in words (for pico, 4 byte a word)
                NULL,
                MAIN_TASK_PRIORITY,
                &mainHandle
        );
        if (main_rslt != pdPASS) {
            printf("Error creating main task: %d\n", main_rslt);
        } else {
            printf("main task created successfully\n");
        }

        // assign main task to core 0
        vTaskCoreAffinitySet(mainHandle, ( 1 << 0 ));
        vTaskDelete(NULL);
    }

}
#endif

#ifdef PICO
int main() {
    stdio_init_all();

    deviceInitSemaphore = xSemaphoreCreateBinary();

    // Create the Wi-Fi initialization task on Core 0
    TaskHandle_t deviceInitHandle;
    xTaskCreate(Cyw43ArchInitTask, "Device Init Task", configMINIMAL_STACK_SIZE, NULL, 2, &deviceInitHandle);
    vTaskCoreAffinitySet(deviceInitHandle, (1 << 0));

    TaskHandle_t initHandle;
    auto init_rslt = xTaskCreate(
            WiFiInitTask,
            "WiFiInit",
            1024, // this is the size for the stack, and in words (for pico, 4 byte a word)
            NULL,
            MAIN_TASK_PRIORITY,
            &initHandle
    );
    if (init_rslt != pdPASS) {
        printf("Error creating wifi task: %d\n", init_rslt);
    } else {
        printf("wifi init task created successfully\n");
    }

    // assign main task to core 0
    vTaskCoreAffinitySet(initHandle, ( 1 << 0 ));
    vTaskStartScheduler();
}
#else
int main(int argc, char *argv[]) {
    #ifdef DEBUG
        std::cout << "Running Debug Mode" << std::endl;
    #else
        std::cout << "Running Release Mode" << std::endl;
    #endif

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
    const std::string path = "config_files/" + std::to_string(firmwareVersion) + "_config.txt";
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
        for (auto& plan : exp.fftForChannels) {
            fftwf_destroy_plan(plan);
            plan = nullptr;
        }

        fftwf_destroy_plan(exp.inverseFFT);
        exp.inverseFFT = nullptr;

        sess.errorOccurred = false;
    }
}
#endif
