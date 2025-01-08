#include "pipeline.h"

#include "../main_utils.h"
#include "../pch.h"

/**
 * @brief Constructs a Pipeline object and initializes necessary components.
 *
 * @param sharedSess Reference to a SharedDataManager object for managing shared
 * resources.
 * @param pipelineVariables Configuration parameters for the pipeline.
 */
Pipeline::Pipeline(OutputManager& outputManager, SharedDataManager& sharedDataManager,
                   const PipelineVariables& pipelineVariables)
    : mFirmwareConfig(FirmwareFactory::create(pipelineVariables.useImu)),
      mOutputManager(outputManager),
      mSharedDataManager(sharedDataManager),
      mSpeedOfSound(pipelineVariables.speedOfSound),
      mReceiverPositionsPath(pipelineVariables.receiverPositionsPath),
      mFilter(IFrequencyDomainStrategyFactory::create(pipelineVariables.frequencyDomainStrategy,
                                                      pipelineVariables.filterWeightsPath, mChannelData,
                                                      mFirmwareConfig->NUM_CHAN)),
      mTimeDomainDetector(ITimeDomainDetectorFactory::create(pipelineVariables.timeDomainDetector,
                                                             pipelineVariables.timeDomainThreshold)),
      mFrequencyDomainDetector(IFrequencyDomainDetectorFactory::create(pipelineVariables.frequencyDomainDetector,
                                                                       pipelineVariables.energyDetectionThreshold)),
      mTracker(ITracker::create(pipelineVariables)),
      mOnnxModel(IONNXModel::create(pipelineVariables)),
      mChannelData(Eigen::MatrixXf::Zero(mFirmwareConfig->NUM_CHAN, mFirmwareConfig->CHANNEL_SIZE)),
      mComputeTDOAs(mFilter->getPaddedLength(), mFilter->getFrequencyDomainData().rows(), mFirmwareConfig->NUM_CHAN,
                    mFirmwareConfig->SAMPLE_RATE)

{
}

/**
 * @brief Processes the data pipeline in a loop until termination conditions are
 * met.
 */
void Pipeline::process()
{
    try
    {
        dataProcessor();
    } catch (const std::exception& e)
    {
        handleProcessingError(e);
    }
}

/**
 * @brief Processes data segments from the shared buffer.
 *
 * Applies filters, performs analysis, and manages data processing pipeline
 * operations.
 */
void Pipeline::dataProcessor()
{
    Eigen::MatrixXf hydrophonePositions = getHydrophoneRelativePositions(mReceiverPositionsPath);
    auto [precomputedP, basisMatrixU, rankOfHydrophoneMatrix] = hydrophoneMatrixDecomposition(hydrophonePositions);

    bool previousTimeSet = false;
    auto previousTime = TimePoint::min();
    dataBytes.resize(mFirmwareConfig->NUM_PACKS_DETECT);

    // call function once outside of the loop below to initialize files.
    initializeOutputFiles(previousTimeSet, previousTime);

    while (!mSharedDataManager.errorOccurred)
    {
        obtainAndProcessByteData(previousTimeSet, previousTime);
        mOutputManager.terminateProgramIfNecessary();

        mOutputManager.flushBufferIfNecessary();

        if (mTracker)
        {
            mTracker->scheduleCluster();
        }
        if (!mTimeDomainDetector->detect(mChannelData.row(0)))
        {
            continue;
        }

        // std::cout << "apply addr channelData: " << mChannelData.data() << std::endl;
        mFilter->apply();
        Eigen::MatrixXcf savedFFTs = mFilter->getFrequencyDomainData();

        // std::cout << "creat addr mSavedFFTs: " << savedFFTs.data() << std::endl;
        Eigen::MatrixXcf beforeFilter = mFilter->mBeforeFilter;

        if (!mFrequencyDomainDetector->detect(savedFFTs.col(0)))
        {
            continue;
        }

        mSharedDataManager.detectionCounter++;
        auto beforeGCC = std::chrono::steady_clock::now();
        auto tdoasAndXCorrAmps = mComputeTDOAs.process(savedFFTs);
        auto afterGCC = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = afterGCC - beforeGCC;
        std::cout << "GCC time: " << duration.count() << std::endl;

        Eigen::VectorXf tdoaVector = std::get<0>(tdoasAndXCorrAmps);
        Eigen::VectorXf DOAs =
            computeDoaFromTdoa(precomputedP, basisMatrixU, mSpeedOfSound, tdoaVector, rankOfHydrophoneMatrix);
        Eigen::VectorXf AzEl = convertDoaToElAz(DOAs);
        std::cout << "AzEl: " << AzEl << std::endl;

        mOutputManager.appendToBuffer(mTimeDomainDetector->getLastDetection(), DOAs[0], DOAs[1], DOAs[2], tdoaVector,
                                      std::get<1>(tdoasAndXCorrAmps), dataTimes[0]);

        int label = -1;
        if (mTracker)
        {
            mTracker->updateTrackerBuffer(DOAs);
            if (mTracker->mIsTrackerInitialized)
            {
                label = mTracker->updateKalmanFiltersContinuous(DOAs, dataTimes[0]);
                mOutputManager.saveSpectraForTraining("training_data_fill.csv", label, beforeFilter);
            }
        }

        if (mOnnxModel)
        {
            std::vector<float> input_tensor_values = getExampleClick();
            mOnnxModel->runInference(input_tensor_values);
        }
    }
}
void Pipeline::initializeOutputFiles(bool& previousTimeSet, TimePoint& previousTime)
{
    obtainAndProcessByteData(previousTimeSet, previousTime);
    mOutputManager.initializeOutputFile(dataTimes[0], mFirmwareConfig->NUM_CHAN);
    if (mTracker)
    {
        mTracker->initializeOutputFile(dataTimes[0]);
    }
}

void Pipeline::obtainAndProcessByteData(bool& previousTimeSet, TimePoint& previousTime)
{
    mSharedDataManager.waitForData(dataBytes, mFirmwareConfig->NUM_PACKS_DETECT);

    dataTimes = mFirmwareConfig->generateTimestamp(dataBytes, mFirmwareConfig->NUM_CHAN);

    mFirmwareConfig->throwIfDataErrors(dataBytes, mFirmwareConfig->MICRO_INCR, previousTimeSet, previousTime, dataTimes,
                                       mFirmwareConfig->packetSize());

    auto before2l = std::chrono::steady_clock::now();
    mFirmwareConfig->insertDataIntoChannelMatrix(mChannelData, dataBytes);
    auto after2l = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration2l = after2l - before2l;
    // std::cout << "append : " << duration2l.count() << std::endl;
}

/**
 * @brief Handles errors that occur during data processing.
 *
 * @param e The exception thrown during data processing.
 */
void Pipeline::handleProcessingError(const std::exception& e)
{
    std::cerr << "Error occurred in data processor thread:\n";

    // Log the exception message
    std::stringstream msg;
    msg << e.what() << std::endl;
    std::cerr << msg.str();

    // Attempt to write data for debugging
    try
    {
        mOutputManager.writeDataToCerr(dataTimes, dataBytes);
    } catch (...)
    {
        std::cerr << "Failed to write data to cerr\n";
    }

    mSharedDataManager.errorOccurred = true;  // Flag the error in the session
}
