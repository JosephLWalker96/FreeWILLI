#include "../runtime_config.h"
#include "../firmware_config.h"

#include "../algorithms/doa_utils.h"
#include "../algorithms/gcc_phat.h"
#include "../algorithms/fir_iir_filtering.h"
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
void dataProcessor(Session &sess, FirmwareConfig &firmwareConfig, RuntimeConfig &runtimeConfig)
{
    try
    {
        // Initialization
        Eigen::VectorXcf filterFreq;
        std::vector<float> paddedFilterWeights;

        int paddedLength = firmwareConfig.CHANNEL_SIZE;
        if (!runtimeConfig.filterWeightsPath.empty())
        {
            initializeFilterWeights(runtimeConfig.filterWeightsPath, firmwareConfig.CHANNEL_SIZE, filterFreq, paddedFilterWeights);
            paddedLength = paddedFilterWeights.size();
        }
        int fftOutputSize = (paddedLength / 2) + 1;

        static Eigen::MatrixXf channelData(paddedLength, firmwareConfig.NUM_CHAN);
        static Eigen::MatrixXcf savedFFTs(fftOutputSize, firmwareConfig.NUM_CHAN);

        setupFftPlans(paddedLength, fftOutputSize, firmwareConfig.NUM_CHAN, channelData, savedFFTs, runtimeConfig.forwardFFT);

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
        while (!sess.errorOccurred)
        {

            sess.dataTimes.clear();
            sess.dataSegment.clear();
            sess.dataBytesSaved.clear();
            TimePoint currentTimestamp;

            while (sess.dataSegment.size() < firmwareConfig.DATA_SEGMENT_LENGTH)
            {

                auto startLoop = std::chrono::system_clock::now();
                waitForData(sess, observationBuffer, runtimeConfig, dataBytes);

                sess.dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error

                currentTimestamp = generateTimestamp(dataBytes, firmwareConfig.NUM_CHAN);
                sess.dataTimes.push_back(currentTimestamp);
                bool dataError = checkForDataErrors(sess, dataBytes, firmwareConfig.MICRO_INCR, previousTimeSet, previousTime, firmwareConfig.PACKET_SIZE);

                if (!dataError) [[likely]]
                {
                    auto beforeConvert = std::chrono::steady_clock::now();
                    convertAndAppend(sess.dataSegment, dataBytes, firmwareConfig.DATA_SIZE, firmwareConfig.HEAD_SIZE); // bytes data is decoded and appended to sess.dataSegment
                    auto afterConvert = std::chrono::steady_clock::now();
                    std::chrono::duration<double> durationConvert = afterConvert - beforeConvert;
                    // std::cout << "convertAndAppend: " << durationConvert.count() << std::endl;
                }

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

            auto beginLatency = std::chrono::steady_clock::now();

            auto beforePtr = std::chrono::steady_clock::now();
            firmwareConfig.ProcessFncPtr(sess.dataSegment, channelData, firmwareConfig.NUM_CHAN);
            auto afterPtr = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationPtr = afterPtr - beforePtr;
            // std::cout << "durationPtr: " << durationPtr.count() << std::endl;

            if (runtimeConfig.tracker)
            {
                runtimeConfig.tracker->scheduleCluster();
            }

            DetectionResult threshResult = detectTimeDomainThreshold(channelData.col(0), sess.dataTimes,
                                                                     runtimeConfig.amplitudeDetectionThreshold, firmwareConfig.SAMPLE_RATE);
            if (threshResult.maxPeakIndex < 0)
            {
                continue;
            }

            // auto beforeFilter = std::chrono::steady_clock::now();
            performFrequencyDomainFIRFiltering(
                channelData,              // Zero-padded time-domain data
                filterFreq,               // Frequency domain filter (FIR taps in freq domain)
                runtimeConfig.forwardFFT, // FFT plan
                savedFFTs);               // Output of FFT transformed time-domain data
            // auto afterFilter = std::chrono::steady_clock::now();
            // std::chrono::duration<double> durationFilter = afterFilter - beforeFilter;

            DetectionResult detResult = detectFrequencyDomainThreshold(savedFFTs.col(0), sess.dataTimes,
                                                                       runtimeConfig.energyDetectionThreshold, firmwareConfig.SAMPLE_RATE);

            if (detResult.maxPeakIndex < 0)
            {             // if no pulse was detected (maxPeakIndex remains -1) stop processing
                continue; // get next dataSegment; return to loop
            }

            /*
             *  Pulse detected. Now process the channels filtering, TDOA & DOA estimation.
             */

            sess.detectionCounter++;
            // std::cout << "Filter runtime: " << durationFilter.count() << std::endl;

            auto beforeGCCW = std::chrono::steady_clock::now();
            // std::tuple<Eigen::VectorXf, Eigen::VectorXf> tdoasAndXCorrAmps = computeGccPhat(savedFFTs, runtimeConfig.inverseFFT, paddedLength, firmwareConfig.NUM_CHAN, firmwareConfig.SAMPLE_RATE);
            std::tuple<Eigen::VectorXf, Eigen::VectorXf> tdoasAndXCorrAmps = computeTDOAs.process(savedFFTs);
            auto afterGCCW = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationGCCW = afterGCCW - beforeGCCW;
            std::cout << "GCC time: " << durationGCCW.count() << std::endl;

            Eigen::VectorXf tdoaVector = std::get<0>(tdoasAndXCorrAmps);
            Eigen::VectorXf XCorrAmps = std::get<1>(tdoasAndXCorrAmps);
            // std::cout << "TDOAs: " << tdoaVector.transpose() << std::endl;
            //  std::cout << "GCC time: " << durationGCCW.count() << std::endl;

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
            std::chrono::duration<double> latencyMeasure = endLatency - beginLatency;
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
            writeDataToCerr(sess.dataTimes, sess.dataBytesSaved);
        }
        catch (...)
        {
            std::cerr << "failed to write data to cerr \n";
            sess.errorOccurred = true;
        }
        sess.errorOccurred = true;
    }
}
