#pragma once

#include "../firmware_config.h"

#include "../algorithms/doa_utils.h"
#include "../algorithms/gcc_phat.h"
#include "../algorithms/hydrophone_position_processing.h"
#include "../algorithms/fir_filter.h"
#include "../algorithms/threshold_detectors.h"
#include "../algorithms/IMU_processor.h"

#include "../pch.h"
#include "../shared_data_manager.h"
#include "../io/buffer_writer.h"
#include "../tracker/tracker.h"

#include "processor_thread_utils.h"

#include "../ML/onnx_model.h"
#include "../tracker/tracker.h"
#include "../io/socket_manager.h"

#include "../main_utils.h"
using TimePoint = std::chrono::system_clock::time_point;

class Pipeline
{
public:

    Pipeline(SharedDataManager& sharedSess, const PipelineVariables& pipelineVariables) : sess(sharedSess) {

        float speedOfSound = pipelineVariables.speedOfSound;
        float energyDetectionThreshold = pipelineVariables.energyDetectionThreshold;
        float amplitudeDetectionThreshold = pipelineVariables.amplitudeDetectionThreshold;

        if (pipelineVariables.filterWeightsPath.empty()){
            std::cout << "no filter" << std::endl;
            filter = std::make_unique<FrequencyDomainNoFilterStrategy>(firmwareConfig.CHANNEL_SIZE, firmwareConfig.NUM_CHAN);
        }
        else{
            filter = std::make_unique<FrequencyDomainFilterStrategy>(pipelineVariables.filterWeightsPath, firmwareConfig.CHANNEL_SIZE, firmwareConfig.NUM_CHAN);
            std::cout << "using filter" << std::endl;
        }


        if (pipelineVariables.enableTracking)
        {
            tracker = std::make_unique<Tracker>(0.04f, 15, 4, "",
                                                            pipelineVariables.clusterFrequencyInSeconds,
                                                            pipelineVariables.clusterWindowInSeconds);
        }

        if (pipelineVariables.useImu)
        {
            firmwareConfig.imuManager = std::make_unique<ImuProcessor>(firmwareConfig.IMU_BYTE_SIZE);
        }

        if (!pipelineVariables.onnxModelPath.empty())
        {
            onnxModel = std::make_unique<ONNXModel>(pipelineVariables.onnxModelPath, pipelineVariables.onnxModelNormalizationPath);
        }
    }

    void process(){
        try
        {
            dataProcessor();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error occured in data processor thread: \n";

            std::stringstream msg; // compose message to dispatch
            msg << e.what() << std::endl;
            std::cerr << msg.str();

            try
            {
                writeDataToCerr(dataTimes, dataBytesSaved);
            }
            catch (...)
            {
                std::cerr << "failed to write data to cerr \n";
                sess.errorOccurred = true;
            }
            sess.errorOccurred = true;
        }
    }

    // public member variables
    std::chrono::seconds programRuntime;
    TimePoint programStartTime;

private:

    // Instantiate firmware configuration varaibles

    SharedDataManager& sess;
    FirmwareConfig firmwareConfig;
    
    Eigen::MatrixXf channelData; // time domain data (inputs)

    std::string detectionOutputFile = "";

    float energyDetectionThreshold;
    float amplitudeDetectionThreshold;
    float speedOfSound;

    std::string receiverPositionsPath = "";


    // Buffer objects
    std::vector<std::vector<uint8_t>> dataBytesSaved;
    std::vector<std::chrono::system_clock::time_point> dataTimes; // timestamps of UDP packet

    std::unique_ptr<SocketManager> socketManger = nullptr;
    std::unique_ptr<IFrequencyDomainStrategy> filter = nullptr;
    std::unique_ptr<ONNXModel> onnxModel = nullptr;
    std::unique_ptr<Tracker> tracker = nullptr;
    
    /**
    * @brief Processes data segments from a shared buffer, applies filters, and performs analysis.
    *
     * @param sess Reference to a Session object for managing shared buffers and synchronization.
     * @param firmwareConfig Reference to an FirmwareConfig object containing configuration details.
     * @param runtimeConfig Reference to an ExperimentRuntime object containing runtime-specific details.
     */
    void dataProcessor()
    {

        int paddedLength = filter->getPaddedLength();
        std::cout << "paddedLength: " << paddedLength << std::endl;

        channelData.resize(firmwareConfig.NUM_CHAN, paddedLength);
        channelData.setZero();
        filter->initialize(channelData);

        Eigen::MatrixXf hydrophonePositions = getHydrophoneRelativePositions(receiverPositionsPath);

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

            dataTimes.clear();
            dataBytesSaved.clear();
            TimePoint currentTimestamp;

            for (int nthPacketInSegment = 0; nthPacketInSegment < firmwareConfig.NUM_PACKS_DETECT; nthPacketInSegment++){

                auto startLoop = std::chrono::system_clock::now();
                waitForData(sess, dataBytes);
                
                dataBytesSaved.push_back(dataBytes); // save bytes in case they need to be saved to a file in case of error

                currentTimestamp = firmwareConfig.generateTimestamp(dataBytes, firmwareConfig.NUM_CHAN);
                dataTimes.push_back(currentTimestamp);

                firmwareConfig.throwIfDataErrors(dataBytes, firmwareConfig.MICRO_INCR, previousTimeSet, previousTime, currentTimestamp, firmwareConfig.PACKET_SIZE);

                previousTime = currentTimestamp;
                previousTimeSet = true;

                auto beforeConvert = std::chrono::steady_clock::now();
                firmwareConfig.convertAndInsertData(channelData, dataBytes, nthPacketInSegment); // bytes data is decoded and appended to sess.dataSegment
                auto afterConvert = std::chrono::steady_clock::now();
                durationConvert = afterConvert - beforeConvert;


                if (firmwareConfig.imuManager)
                {
                    auto beforePtr = std::chrono::steady_clock::now();
                    firmwareConfig.imuManager->setRotationMatrix(dataBytes);
                    auto afterPtr = std::chrono::steady_clock::now();
                    std::chrono::duration<double> durationPtr = afterPtr - beforePtr;
                    std::cout << "Set rotation matrix time: " << durationPtr.count() << std::endl;
                    std::cout << firmwareConfig.imuManager->mRotationMatrix << std::endl;
                }
            }

            /*
            *   Exited inner loop - dataSegment has been filled to 'DATA_SEGMENT_LENGTH' length
            */

            auto processingTimeBegin = std::chrono::steady_clock::now();

            if ((detectionOutputFile).empty()) [[unlikely]]
            {
                std::string outputFile = "deployment_files/" + convertTimePointToString(currentTimestamp);
                detectionOutputFile = outputFile;

                if (tracker)
                {
                    tracker->mOutputfile = outputFile + "_tracker";
                }
                observationBuffer.initializeOutputFile(outputFile, firmwareConfig.NUM_CHAN);
            }

            if (shouldTerminateProgram())
            {
                observationBuffer.write(detectionOutputFile);
                std::cout << "Terminating program... duration reached \n";
                std::exit(0);
            }

            observationBuffer.flushBufferIfNecessary(detectionOutputFile); // observationBuffer should not need this argument

            if (tracker)
            {
                tracker->scheduleCluster();
            }

            //std::cout << "channel 1 samples" << channelData.row(0).head(10) << std::endl;
            
            //std::cout << "time row1 head" << channelData.row(0).head(5) << std::endl;
            //std::cout << "time row2 head" << channelData.row(1).head(5) << std::endl;
            //std::cout << "time row3 head" << channelData.row(2).head(5) << std::endl;
            //std::cout << "time row4 head" << channelData.row(3).head(5) << std::endl;

            //std::cout << "time row1 tail" << channelData.row(0).tail(5) << std::endl;
            //std::cout << "time row2 tail" << channelData.row(1).tail(5) << std::endl;
            //std::cout << "time row3 tail" << channelData.row(2).tail(5) << std::endl;
            //std::cout << "time row4 tail" << channelData.row(3).tail(5) << std::endl;
            

            DetectionResult threshResult = detectTimeDomainThreshold(channelData.row(0), dataTimes,
                                                                    amplitudeDetectionThreshold, firmwareConfig.SAMPLE_RATE);
            if (threshResult.maxPeakIndex < 0)
            {
                continue;
            }

            //std::cout << "BeforeFilt: " << std::endl;
            auto beforeFilt = std::chrono::steady_clock::now();
            filter->apply(channelData);
            auto afterFilt = std::chrono::steady_clock::now();
            std::chrono::duration<double> durationFilt = afterFilt - beforeFilt;
            //std::cout << "durationFilt: " << durationFilt.count() << std::endl;
            // std::cout << "after filt " << &channelData << std::endl;
            Eigen::MatrixXcf savedFFTs = filter->getFrequencyDomainData();
            
            //std::cout << "savedFFTs col1 head" << savedFFTs.col(0).head(5) << std::endl;
            //std::cout << "savedFFTs col2 head" << savedFFTs.col(1).head(5) << std::endl;
            //std::cout << "savedFFTs col3 head" << savedFFTs.col(2).head(5) << std::endl;
            //std::cout << "savedFFTs col4 head" << savedFFTs.col(3).head(5) << std::endl;

            //std::cout << "savedFFTs col1 tail" << savedFFTs.col(0).tail(5) << std::endl;
            //std::cout << "savedFFTs col2 tail" << savedFFTs.col(1).tail(5) << std::endl;
            //std::cout << "savedFFTs col3 tail" << savedFFTs.col(2).tail(5) << std::endl;
            //std::cout << "savedFFTs col4 tail" << savedFFTs.col(3).tail(5) << std::endl;
            //std::exit(0);
            

            DetectionResult detResult = detectFrequencyDomainThreshold(savedFFTs.col(0), dataTimes,
                                                                    energyDetectionThreshold, firmwareConfig.SAMPLE_RATE);

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
            Eigen::VectorXf DOAs = computeDoaFromTdoa(P, U, speedOfSound, tdoaVector, rankOfH);

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
            if (tracker)
            {
                tracker->updateTrackerBuffer(DOAs);
                if (tracker->mIsTrackerInitialized)
                {
                    trackerLabel = tracker->updateKalmanFiltersContinuous(DOAs, threshResult.peakTimes);
                    saveSpectraForTraining("training_data.csv", trackerLabel, savedFFTs.col(0));
                    std::cout << "tracker label: " << trackerLabel << std::endl;
                }
            }

            // Normalize the input data
            if (onnxModel)
            {
                std::vector<float> input_tensor_values = getExampleClick();
                // Run inference
                auto beforeClass = std::chrono::steady_clock::now();
                std::vector<float> predictions = onnxModel->runInference(input_tensor_values);
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

    /**
     * @brief Determines if the program should terminate based on the runtime duration.
     *
     * @param runtimeConfig Experiment runtime configuration and state.
     * @param startLoop The time point at the start of the current loop iteration.
     * @return true if the program should terminate, false otherwise.
     */
    bool shouldTerminateProgram()
    {

        TimePoint currentTime = std::chrono::system_clock::now();
        auto elapsedTime = currentTime - programStartTime;
        return elapsedTime >= programRuntime;
    }
};