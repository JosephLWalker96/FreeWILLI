#include "pipeline.h"

#include "algorithms/linear_algebra_utils.h"
#include "pch.h"
#include "utils.h"

/**
 * @brief Constructs a Pipeline object and initializes necessary components.
 *
 * @param sharedSess Reference to a SharedDataManager object for managing shared
 * resources.
 * @param pipelineVariables Configuration parameters for the pipeline.
 */
Pipeline::Pipeline(
    OutputManager& outputManager, SharedDataManager& sharedDataManager, const PipelineVariables& pipelineVariables)
    : mFirmwareConfig(FirmwareFactory::create(pipelineVariables.firmware)),
      mOutputManager(outputManager),
      mSharedDataManager(sharedDataManager),
      mSpeedOfSound(pipelineVariables.speedOfSound),
      mReceiverPositionsPath(pipelineVariables.receiverPositionsPath),
      mFilter(
          IFrequencyDomainStrategyFactory::create(
              pipelineVariables.frequencyDomainStrategy, pipelineVariables.filterWeightsPath, mChannelData,
              mFirmwareConfig->numChannels())),
      mTimeDomainDetector(
          ITimeDomainDetectorFactory::create(
              pipelineVariables.timeDomainDetector, pipelineVariables.timeDomainThreshold)),
      mFrequencyDomainDetector(
          IFrequencyDomainDetectorFactory::create(
              pipelineVariables.frequencyDomainDetector, pipelineVariables.energyDetectionThreshold)),
      mTracker(ITracker::create(pipelineVariables)),
      mOnnxModel(IONNXModel::create(pipelineVariables)),
      mChannelData(Eigen::MatrixXf::Zero(mFirmwareConfig->numChannels(), mFirmwareConfig->channelSize())),
      mComputeTDOAs(
          mFilter->getPaddedLength(), mFilter->getFrequencyDomainData().rows(), mFirmwareConfig->numChannels(),
          mFirmwareConfig->sampleRate())

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
    }
    catch (const std::exception& e)
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

    auto svdDecomposition = computeSvd(hydrophonePositions);
    auto [cachedLeastSquaresResult, rankOfHydrophoneMatrix] =
        precomputePseudoInverseAndRank(svdDecomposition, mSpeedOfSound);

    // precompute the leastsquares matrix. Use for efficient DOA estiation
    // Eigen::MatrixXf cachedLeastSquaresResult = precomputedInverse * mSpeedOfSound;

    bool previousTimeSet = false;
    auto previousTime = TimePoint::min();
    dataBytes.resize(mFirmwareConfig->numPacketsToDetect());

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

        // std::cout << "apply addr channelData: " << mChannelData.data() <<
        // std::endl;
        mFilter->apply();
        Eigen::MatrixXcf savedFFTs = mFilter->getFrequencyDomainData();

        // std::cout << "creat addr mSavedFFTs: " << savedFFTs.data() <<
        // std::endl;
        Eigen::MatrixXcf beforeFilter = mFilter->mBeforeFilter;

        if (!mFrequencyDomainDetector->detect(savedFFTs.col(0)))
        {
            continue;
        }

        if (mOnnxModel)
        {
            // std::vector<float> input_tensor_values = getExampleClick();
            Eigen::VectorXf spectraToInference = beforeFilter.array().abs();

            // std::cout << "Inference spectra: " << std::endl;
            // std::cout << spectraToInference.tail(500).head(5).transpose() << std::endl;
            // std::cout << spectraToInference.tail(500).tail(5).transpose() << std::endl;

            Eigen::VectorXf spectraToInferenceFinal = spectraToInference.tail(500);
            std::vector<float> spectraVector(
                spectraToInferenceFinal.data(), spectraToInferenceFinal.data() + spectraToInferenceFinal.size());
            std::vector<float> output = mOnnxModel->runInference(spectraVector);
            // std::cout << "Classification: \n";
            // for (const auto& val : output)
            //{
            //     std::cout << val << " ";
            // }
            // std::cout << std::endl;
            if (output[1] < output[0])
            {
                std::cout << "Noise detected: \n";
                continue;
            }
        }
        mSharedDataManager.detectionCounter++;
        auto beforeGCC = std::chrono::steady_clock::now();
        auto tdoasAndXCorrAmps = mComputeTDOAs.process(savedFFTs);
        auto afterGCC = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = afterGCC - beforeGCC;
        std::cout << "GCC time: " << duration.count() << std::endl;

        Eigen::VectorXf tdoaVector = std::get<0>(tdoasAndXCorrAmps);
        Eigen::VectorXf directionOfArrival =
            computeDoaFromTdoa(cachedLeastSquaresResult, tdoaVector, rankOfHydrophoneMatrix);
        Eigen::VectorXf azimuthAndElevation = convertDoaToElAz(directionOfArrival);
        std::cout << "AzEl: " << azimuthAndElevation << std::endl;

        mOutputManager.appendToBuffer(
            mTimeDomainDetector->getLastDetection(), directionOfArrival[0], directionOfArrival[1],
            directionOfArrival[2], tdoaVector, std::get<1>(tdoasAndXCorrAmps), dataTimes[0]);

        if (mTracker)  // check
        {
            [[maybe_unused]] int label = -1;
            mTracker->updateTrackerBuffer(directionOfArrival);
            if (mTracker->mIsTrackerInitialized)
            {
                label = mTracker->updateKalmanFiltersContinuous(
                    directionOfArrival, dataTimes[0]);  // NOLINT(clang-analyzer-deadcode.DeadStores)
                // mOutputManager.saveSpectraForTraining("training_data_fill.csv", label, beforeFilter);
            }
        }

        if (mFirmwareConfig->getImuManager())
        {
            std::cout << mFirmwareConfig->getImuManager()->getRotationMatrix();
            std::cout << std::endl;
        }
    }
}
void Pipeline::initializeOutputFiles(bool& previousTimeSet, TimePoint& previousTime)
{
    obtainAndProcessByteData(previousTimeSet, previousTime);
    mOutputManager.initializeOutputFile(dataTimes[0], mFirmwareConfig->numChannels());
    if (mTracker)
    {
        mTracker->initializeOutputFile(dataTimes[0]);
    }
}

void Pipeline::obtainAndProcessByteData(bool& previousTimeSet, TimePoint& previousTime)
{
    mSharedDataManager.waitForData(dataBytes, mFirmwareConfig->numPacketsToDetect());

    dataTimes = mFirmwareConfig->generateTimestamp(dataBytes);

    mFirmwareConfig->throwIfDataErrors(dataBytes, previousTimeSet, previousTime, dataTimes);

    // auto before2l = std::chrono::steady_clock::now();
    mFirmwareConfig->insertDataIntoChannelMatrix(mChannelData, dataBytes);
    // auto after2l = std::chrono::steady_clock::now();
    // std::chrono::duration<double> duration2l = after2l - before2l;
    //  std::cout << "append : " << duration2l.count() << std::endl;
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
    }
    catch (...)
    {
        std::cerr << "Failed to write data to cerr\n";
    }

    mSharedDataManager.errorOccurred = true;  // Flag the error in the session
}
