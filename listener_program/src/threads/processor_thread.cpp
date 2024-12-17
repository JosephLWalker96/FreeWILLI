#include "../runtime_config.h"
#include "../firmware_config.h"

#include "../algorithms/doa_utils.h"
#include "../algorithms/gcc_phat.h"
// #include "../algorithms/fir_iir_filtering.h"
#include "../algorithms/fir_filter.h"
#include "../algorithms/threshold_detectors.h"
#include "../algorithms/IMU_processor.h"

#include "../pch.h"
#include "../session.h"
#include "../io/buffer_writer.h"
#include "../tracker/tracker.h"

#include "processor_thread_utils.h"

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Processes data segments from a shared buffer, applies filters, and performs analysis.
 *
 * @param sess Reference to a Session object for managing shared buffers and synchronization.
 * @param firmwareConfig Reference to an FirmwareConfig object containing configuration details.
 * @param runtimeConfig Reference to an ExperimentRuntime object containing runtime-specific details.
 */
void dataProcessor(SharedDataManager &sess, FirmwareConfig &firmwareConfig, RuntimeConfig &runtimeConfig)
{
    try
    {

        int paddedLength = runtimeConfig.filter->getPaddedLength();
        std::cout << "paddedLength: " << paddedLength << std::endl;

        runtimeConfig.channelData.resize(firmwareConfig.NUM_CHAN, paddedLength);
        runtimeConfig.channelData.setZero();
        runtimeConfig.filter->initialize(runtimeConfig.channelData);

        Eigen::MatrixXf hydrophonePositions = getHydrophoneRelativePositions(runtimeConfig.receiverPositionsPath);

        Eigen::MatrixXf P, U; // SVD matrices of hydrophonePositions
        int rankOfH;
        hydrophoneMatrixDecomposition(hydrophonePositions, P, U, rankOfH);

        ObservationBuffer observationBuffer;

        // Declare time-checking variables
        bool previousTimeSet = false;
        auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();

        // Container for pulling bytes from the buffer
        std::vector<uint8_t> dataBytes;
        

        GCC_PHAT computeTDOAs(paddedLength, firmwareConfig.NUM_CHAN, firmwareConfig.SAMPLE_RATE);

        std::cout << "Ready to process data..." << std::endl;
        std::chrono::duration<double> durationConvert;
        while (!sess.errorOccurred)
        {

            runtimeConfig.dataTimes.clear();
            runtimeConfig.dataBytesSaved.clear();
            TimePoint currentTimestamp;

            for (int nthPacketInSegment = 0; nthPacketInSegment < firmwareConfig.NUM_PACKS_DETECT; nthPacketInSegment++){

                auto startLoop = std::chrono::system_clock::now();
                waitForData(sess, observationBuffer, runtimeConfig, dataBytes);

                runtimeConfig.dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error

                currentTimestamp = generateTimestamp(dataBytes, firmwareConfig.NUM_CHAN);
                runtimeConfig.dataTimes.push_back(currentTimestamp);
                bool dataError = checkForDataErrors(sess, dataBytes, firmwareConfig.MICRO_INCR, previousTimeSet, previousTime, currentTimestamp, firmwareConfig.PACKET_SIZE);

                if (dataError) [[unlikely]]
                {
                    std::stringstream errorMsg;
                    errorMsg << "Error: Time not incremented by " << firmwareConfig.MICRO_INCR << std::endl;
                    std::cerr << errorMsg.str();
                    writeDataToCerr(runtimeConfig.dataTimes, runtimeConfig.dataBytesSaved);
                    runtimeConfig.dataTimes.clear();
                    runtimeConfig.dataBytesSaved.clear();
                    previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
                    previousTimeSet = false;
                }

                auto beforeConvert = std::chrono::steady_clock::now();
                firmwareConfig.convertAndInsertData(runtimeConfig.channelData, dataBytes, nthPacketInSegment); // bytes data is decoded and appended to sess.dataSegment
                auto afterConvert = std::chrono::steady_clock::now();
                durationConvert = afterConvert - beforeConvert;

                if ((runtimeConfig.detectionOutputFile).empty()) [[unlikely]]
                {
                    initializeOutputFiles(runtimeConfig.detectionOutputFile, runtimeConfig.tracker, currentTimestamp, firmwareConfig.NUM_CHAN);
                }

                if (runtimeConfig.imuManager)
                {
                    auto beforePtr = std::chrono::steady_clock::now();
                    runtimeConfig.imuManager->setRotationMatrix(dataBytes);
                    auto afterPtr = std::chrono::steady_clock::now();
                    std::chrono::duration<double> durationPtr = afterPtr - beforePtr;
                    std::cout << "durationPtr: " << durationPtr.count() << std::endl;
                    std::cout << runtimeConfig.imuManager->mRotationMatrix << std::endl;
                }
            }

            /*
             *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
             *   now apply energy detector.
             */

            //auto beginLatency = std::chrono::steady_clock::now();

            if (runtimeConfig.tracker)
            {
                runtimeConfig.tracker->scheduleCluster();
            }

            //std::cout << "channel 1 samples" << runtimeConfig.channelData.row(0).head(10) << std::endl;
             
            std::cout << "time row1 head" << runtimeConfig.channelData.row(0).head(5) << std::endl;
            //std::cout << "time row2 head" << runtimeConfig.channelData.row(1).head(5) << std::endl;
            //std::cout << "time row3 head" << runtimeConfig.channelData.row(2).head(5) << std::endl;
            //std::cout << "time row4 head" << runtimeConfig.channelData.row(3).head(5) << std::endl;

            std::cout << "time row1 tail" << runtimeConfig.channelData.row(0).tail(5) << std::endl;
            //std::cout << "time row2 tail" << runtimeConfig.channelData.row(1).tail(5) << std::endl;
            //std::cout << "time row3 tail" << runtimeConfig.channelData.row(2).tail(5) << std::endl;
            //std::cout << "time row4 tail" << runtimeConfig.channelData.row(3).tail(5) << std::endl;
            

            DetectionResult threshResult = detectTimeDomainThreshold(runtimeConfig.channelData.row(0), runtimeConfig.dataTimes,
                                                                     runtimeConfig.amplitudeDetectionThreshold, firmwareConfig.SAMPLE_RATE);
            if (threshResult.maxPeakIndex < 0)
            {
                continue;
            }

            //std::cout << "BeforeFilt: " << std::endl;
            auto beforeFilt = std::chrono::steady_clock::now();
            runtimeConfig.filter->apply(runtimeConfig.channelData);
            auto afterFilt = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFilt = afterFilt - beforeFilt;
            //std::cout << "durationFilt: " << durationFilt.count() << std::endl;
            // std::cout << "after filt " << &channelData << std::endl;
            Eigen::MatrixXcf savedFFTs = runtimeConfig.filter->getFrequencyDomainData();
            
            std::cout << "savedFFTs col1 head" << savedFFTs.col(0).head(5) << std::endl;
            //std::cout << "savedFFTs col2 head" << savedFFTs.col(1).head(5) << std::endl;
            //std::cout << "savedFFTs col3 head" << savedFFTs.col(2).head(5) << std::endl;
            //std::cout << "savedFFTs col4 head" << savedFFTs.col(3).head(5) << std::endl;

            std::cout << "savedFFTs col1 tail" << savedFFTs.col(0).tail(5) << std::endl;
            //std::cout << "savedFFTs col2 tail" << savedFFTs.col(1).tail(5) << std::endl;
            //std::cout << "savedFFTs col3 tail" << savedFFTs.col(2).tail(5) << std::endl;
            //std::cout << "savedFFTs col4 tail" << savedFFTs.col(3).tail(5) << std::endl;
            //std::exit(0);
            

            DetectionResult detResult = detectFrequencyDomainThreshold(savedFFTs.col(0), runtimeConfig.dataTimes,
                                                                       runtimeConfig.energyDetectionThreshold, firmwareConfig.SAMPLE_RATE);

            if (detResult.maxPeakIndex < 0)
            {             // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue; // get next dataSegment; return to loop
            }

            /*
             *  Pulse detected. Now process the channels according to runtimeConfig settings.
             */

            sess.detectionCounter++;
            // std::cout << "Filter runtime: " << durationFilter.count() << std::endl;

            auto beforeGCCW = std::chrono::steady_clock::now();
            std::tuple<Eigen::VectorXf, Eigen::VectorXf> tdoasAndXCorrAmps = computeTDOAs.process(savedFFTs);
            auto afterGCCW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationGCCW = afterGCCW - beforeGCCW;
            std::cout << "GCC time: " << durationGCCW.count() << std::endl;

            Eigen::VectorXf tdoaVector = std::get<0>(tdoasAndXCorrAmps);
            Eigen::VectorXf XCorrAmps = std::get<1>(tdoasAndXCorrAmps);
            std::cout << "TDOAs: " << tdoaVector.transpose() << std::endl;
            std::cout << "GCC time: " << durationGCCW.count() << std::endl;
            // auto beforeDOA = std::chrono::steady_clock::now();
            Eigen::VectorXf DOAs = computeDoaFromTdoa(P, U, runtimeConfig.speedOfSound, tdoaVector, rankOfH);

            Eigen::VectorXf AzEl = convertDoaToElAz(DOAs);
            // auto afterDOA = std::chrono::steady_clock::now();
            // std::chrono::duration<double> durationDOA = afterDOA - beforeDOA;
            // std::cout << "DOA time: " << durationDOA.count() << std::endl;

            // Eigen::VectorXf DOAs = TDOA_To_DOA_VerticalArray(tdoaVector, 1500.0, chanSpacing);
            std::cout << "AzEl: " << AzEl.transpose() << std::endl;
            // std::cout << "Energy: " << detResult.peakAmplitude << std::endl;
            // std::cout << "tdoa: " << tdoaVector.transpose() << std::endl;

            // Write to buffers
            observationBuffer.appendToBuffer(detResult.peakAmplitude, DOAs[0], DOAs[1], DOAs[2], tdoaVector, XCorrAmps, threshResult.peakTimes);

            // UpdatetrackerLabel tracker buffer and filters with last observation
            int trackerLabel = -1;
            if (runtimeConfig.tracker)
            {
                runtimeConfig.tracker->updateTrackerBuffer(DOAs);
                if (runtimeConfig.tracker->mIsTrackerInitialized)
                {
                    trackerLabel = runtimeConfig.tracker->updateKalmanFiltersContinuous(DOAs, threshResult.peakTimes);
                    saveSpectraForTraining("training_data.csv", trackerLabel, savedFFTs.col(0));
                    std::cout << "tracker label: " << trackerLabel << std::endl;
                }
            }

            // Normalize the input data
            if (runtimeConfig.onnxModel)
            {
                std::vector<float> input_tensor_values = getExampleClick();
                // Run inference
                auto beforeClass = std::chrono::steady_clock::now();
                std::vector<float> predictions = runtimeConfig.onnxModel->runInference(input_tensor_values);
                auto afterClass = std::chrono::steady_clock::now();
                std::chrono::duration<double> durationClass = afterClass - beforeClass;
                /*
                std::cout << "classifier runtime: " << durationClass.count() << std::endl;
                for (int i = 0; i < predictions.size(); i++)
                {
                    std::cout << predictions[i] << " ";
                }
                std::cout << std::endl;
                */
            }

            auto endLatency = std::chrono::steady_clock::now();
            //std::chrono::duration<double> latencyMeasure = endLatency - beginLatency;
            // std::cout << "Latency: " << latencyMeasure.count() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error occured in data processor thread: \n";

        std::stringstream msg; // compose message to dispatch
        msg << e.what() << std::endl;
        std::cerr << msg.str();

        try
        {
            writeDataToCerr(runtimeConfig.dataTimes, runtimeConfig.dataBytesSaved);
        }
        catch (...)
        {
            std::cerr << "failed to write data to cerr \n";
            sess.errorOccurred = true;
        }
        sess.errorOccurred = true;
    }
}
