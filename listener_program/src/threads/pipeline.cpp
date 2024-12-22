#include "pipeline.h"

/**
 * @brief Constructs a Pipeline object and initializes necessary components.
 *
 * @param sharedSess Reference to a SharedDataManager object for managing shared resources.
 * @param pipelineVariables Configuration parameters for the pipeline.
 */
Pipeline::Pipeline(SharedDataManager &sharedSess, const PipelineVariables &pipelineVariables)
    : sess(sharedSess),
      speedOfSound(pipelineVariables.speedOfSound),
      receiverPositionsPath(pipelineVariables.receiverPositionsPath)
    {
    
    if (pipelineVariables.filterWeightsPath.empty()) {
        filter = std::make_unique<FrequencyDomainNoFilterStrategy>(firmwareConfig.CHANNEL_SIZE, firmwareConfig.NUM_CHAN);
    } else {
        filter = std::make_unique<FrequencyDomainFilterStrategy>(pipelineVariables.filterWeightsPath, firmwareConfig.CHANNEL_SIZE, firmwareConfig.NUM_CHAN);
    }

    if (pipelineVariables.enableTracking) {
        tracker = std::make_unique<Tracker>(0.04f, 15, 4, "",
                                            pipelineVariables.clusterFrequencyInSeconds,
                                            pipelineVariables.clusterWindowInSeconds);
    }

    if (pipelineVariables.useImu) {
        firmwareConfig.imuManager = std::make_unique<ImuProcessor>(firmwareConfig.IMU_BYTE_SIZE);
    }
    if (!pipelineVariables.onnxModelPath.empty()) {
        onnxModel = std::make_unique<ONNXModel>(pipelineVariables.onnxModelPath, pipelineVariables.onnxModelNormalizationPath);
    }
    timeDomainDetector = std::make_unique<PeakAmplitudeDetector>(pipelineVariables.amplitudeDetectionThreshold);
    frequencyDomainDetector = std::make_unique<FrequencyDomainDetector>(pipelineVariables.energyDetectionThreshold);
}

/**
 * @brief Processes the data pipeline in a loop until termination conditions are met.
 */
void Pipeline::process() {
    try {
        dataProcessor();
    } catch (const std::exception &e) {
        std::cerr << "Error occurred in data processor thread:\n";

        std::stringstream msg;
        msg << e.what() << std::endl;
        std::cerr << msg.str();

        try {
            writeDataToCerr(dataTimes, dataBytesSaved);
        } catch (...) {
            std::cerr << "Failed to write data to cerr\n";
            sess.errorOccurred = true;
        }
        sess.errorOccurred = true;
    }
}

/**
 * @brief Processes data segments from the shared buffer.
 *
 * Applies filters, performs analysis, and manages data processing pipeline operations.
 */
void Pipeline::dataProcessor() {
    int paddedLength = filter->getPaddedLength();
    channelData.resize(firmwareConfig.NUM_CHAN, paddedLength);
    channelData.setZero();
    filter->initialize(channelData);

    Eigen::MatrixXf hydrophonePositions = getHydrophoneRelativePositions(receiverPositionsPath);
    auto [precomputedP, basisMatrixU, rankOfHydrophoneMatrix] = hydrophoneMatrixDecomposition(hydrophonePositions);

    
    bool previousTimeSet = false;
    auto previousTime = TimePoint::min();
    std::vector<std::vector<uint8_t>> dataBytes(firmwareConfig.NUM_PACKS_DETECT);

    GCC_PHAT computeTDOAs(paddedLength, firmwareConfig.NUM_CHAN, firmwareConfig.SAMPLE_RATE);
    while (!sess.errorOccurred) {
        
        obtainAndProcessByteData(dataBytes,previousTimeSet, previousTime);
        
        initilializeOutputFiles(dataTimes[0]); // uses the first time stamp from the first set of data

        terminateProgramIfNecessary(); // terminate if we have reached specified duty cycle 
        

        observationBuffer.flushBufferIfNecessary(detectionOutputFile);
        if (tracker) {
            tracker->scheduleCluster();
        }

        if(!timeDomainDetector->detect(channelData.row(0))){
            break;
        }

        filter->apply(channelData);
        Eigen::MatrixXcf savedFFTs = filter->getFrequencyDomainData();

        if(!frequencyDomainDetector->detect(savedFFTs.col(0))){
            break;
        }

        sess.detectionCounter++;
        auto beforeGCC = std::chrono::steady_clock::now();
        auto tdoasAndXCorrAmps = computeTDOAs.process(savedFFTs);
        auto afterGCC = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = afterGCC - beforeGCC;
        std::cout << "GCC time: " << duration.count() << std::endl;

        Eigen::VectorXf tdoaVector = std::get<0>(tdoasAndXCorrAmps);
        Eigen::VectorXf DOAs = computeDoaFromTdoa(precomputedP, basisMatrixU, speedOfSound, tdoaVector, rankOfHydrophoneMatrix);
        Eigen::VectorXf AzEl = convertDoaToElAz(DOAs);
        std::cout << "AzEl: " << AzEl << std::endl;

        observationBuffer.appendToBuffer(timeDomainDetector->getLastDetection(), 
                                        DOAs[0], DOAs[1], DOAs[2], tdoaVector, 
                                        std::get<1>(tdoasAndXCorrAmps), 
                                        dataTimes[0]);

        if (tracker) {
            tracker->updateTrackerBuffer(DOAs);
            if (tracker->mIsTrackerInitialized) {
                tracker->updateKalmanFiltersContinuous(DOAs, dataTimes[0]);
            }
        }

        if (onnxModel) {
            std::vector<float> input_tensor_values = getExampleClick();
            onnxModel->runInference(input_tensor_values);
        }
    }
}


void Pipeline::obtainAndProcessByteData(std::vector<std::vector<uint8_t>>& dataBytes, bool& previousTimeSet, TimePoint& previousTime){
    dataTimes.clear();
    dataBytesSaved.clear();
    std::vector<TimePoint> currentTimestamp;

    waitForData(sess, dataBytes, firmwareConfig.NUM_PACKS_DETECT);
    dataBytesSaved = dataBytes;
    auto beforeGCC = std::chrono::steady_clock::now();
    currentTimestamp = firmwareConfig.generateTimestamp(dataBytes, firmwareConfig.NUM_CHAN);
    auto afterGCC = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = afterGCC - beforeGCC;
    std::cout << "Timestamps: " << duration.count() << std::endl;

    dataTimes = currentTimestamp;

    //firmwareConfig.throwIfDataErrors(dataBytes, firmwareConfig.MICRO_INCR, previousTimeSet, previousTime, currentTimestamp, firmwareConfig.PACKET_SIZE);
    //previousTime = currentTimestamp;

    previousTimeSet = true;

    auto before2l = std::chrono::steady_clock::now();
    for (int nthPacketInSegment = 0; nthPacketInSegment < firmwareConfig.NUM_PACKS_DETECT; ++nthPacketInSegment) {
        firmwareConfig.insertDataIntoChannelMatrix(channelData, dataBytes[nthPacketInSegment], nthPacketInSegment);

        //if (firmwareConfig.imuManager) {
        //    firmwareConfig.imuManager->setRotationMatrix(dataBytes[nthPacketInSegment]);
        //}
    }

    auto after2l = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration2l = after2l - before2l;
    std::cout << "append : " << duration2l.count() << std::endl;
}

void Pipeline::initilializeOutputFiles(TimePoint& timepoint){
    if ((detectionOutputFile).empty()) [[unlikely]]
    {
        std::string outputFile = "deployment_files/" + convertTimePointToString(dataTimes[0]);
        detectionOutputFile = outputFile;

        if (tracker)
        {
            tracker->mOutputfile = outputFile + "_tracker";
        }
        observationBuffer.initializeOutputFile(outputFile, firmwareConfig.NUM_CHAN);
    }
}

/**
 * @brief Determines if the program should terminate based on the runtime duration.
 *
 */
void Pipeline::terminateProgramIfNecessary() {
    TimePoint currentTime = std::chrono::system_clock::now();
    auto elapsedTime = currentTime - programStartTime;
    if (elapsedTime >= programRuntime){
        observationBuffer.write(detectionOutputFile);
        std::cout << "Terminating program... duration reached\n";
        std::exit(0);
    }
}