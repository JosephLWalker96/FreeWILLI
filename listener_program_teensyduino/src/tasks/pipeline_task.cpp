#include <Arduino.h>
#include "tasks/FW_pipeline.h"

#include "pch.h"
#include "utils.h"

static void ProcessByteData(bool& previousTimeSet, TimePoint& previousTime);

Pipeline p;

// Equivalent to Pipeline::Pipeline() Constructor
void FW_InitPipeline(const PipelineVariables& pipelineVariables)
{
  // Initialize the pipeline struct
  Serial.println("Initializing Pipeline");

  p.mFirmwareConfig = FirmwareFactory::create(pipelineVariables.useImu);
  p.mSpeedOfSound = pipelineVariables.speedOfSound;
  p.mReceiverPositionsPath = pipelineVariables.receiverPositionsPath;
  p.mTimeDomainDetector = ITimeDomainDetectorFactory::create(
    pipelineVariables.timeDomainDetector, pipelineVariables.timeDomainThreshold);
  p.mFrequencyDomainDetector = IFrequencyDomainDetectorFactory::create(
    pipelineVariables.frequencyDomainDetector, pipelineVariables.energyDetectionThreshold);
  p.mChannelData = Eigen::MatrixXf::Zero(p.mFirmwareConfig->NUM_CHAN, p.mFirmwareConfig->CHANNEL_SIZE);

  // mFilter(IFrequencyDomainStrategyFactory::create(
  //     pipelineVariables.frequencyDomainStrategy, pipelineVariables.filterWeightsPath, mChannelData,
  //     mFirmwareConfig->NUM_CHAN)),
  // mTracker(ITracker::create(pipelineVariables)),
  // mOnnxModel(IONNXModel::create(pipelineVariables)),

  Serial.println("Pipeline Init Complete!");
  Serial.flush();
}

void PipelineTask(void*)
{
  Eigen::MatrixXf hydrophonePositions = getHydrophoneRelativePositions(p.mReceiverPositionsPath);
  auto [precomputedP, basisMatrixU, rankOfHydrophoneMatrix] = hydrophoneMatrixDecomposition(hydrophonePositions);

  bool previousTimeSet = false;
  auto previousTime = TimePoint::min();
  p.dataBytes.resize(p.mFirmwareConfig->NUM_PACKS_DETECT);

  while (true)
  {
    ProcessByteData(previousTimeSet, previousTime);
    // mOutputManager.terminateProgramIfNecessary();

    // mOutputManager.flushBufferIfNecessary();

    /*
    if (mTracker)
    {
      mTracker->scheduleCluster();
    }
    */

    if (!p.mTimeDomainDetector->detect(p.mChannelData.row(0)))
    {
      continue;
    }
    std::cout << p.mTimeDomainDetector->getLastDetection() << std::endl;
    std::cout << "Test Code!";

    /*
    mFilter->apply();
    Eigen::MatrixXcf savedFFTs = mFilter->getFrequencyDomainData();

    Eigen::MatrixXcf beforeFilter = mFilter->mBeforeFilter;

    if (!mFrequencyDomainDetector->detect(savedFFTs.col(0)))
    {
      continue;
    }
    */

    /*
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
    */

    // mSharedDataManager.detectionCounter++;
    /*
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

    mOutputManager.appendToBuffer(
        mTimeDomainDetector->getLastDetection(), DOAs[0], DOAs[1], DOAs[2], tdoaVector,
        std::get<1>(tdoasAndXCorrAmps), dataTimes[0]);

    if (mTracker)
    {
      int label = -1;
      mTracker->updateTrackerBuffer(DOAs);
      if (mTracker->mIsTrackerInitialized)
      {
          label = mTracker->updateKalmanFiltersContinuous(DOAs, dataTimes[0]);
          // mOutputManager.saveSpectraForTraining("training_data_fill.csv", label, beforeFilter);
      }
    }
    */
  }
}

static void ProcessByteData(bool& previousTimeSet, TimePoint& previousTime)
{
  // YJ// Read bytes from queue.

  p.dataTimes = p.mFirmwareConfig->generateTimestamp(p.dataBytes, p.mFirmwareConfig->NUM_CHAN);

  p.mFirmwareConfig->throwIfDataErrors(
    p.dataBytes, p.mFirmwareConfig->MICRO_INCR, previousTimeSet, previousTime, p.dataTimes,
        p.mFirmwareConfig->packetSize());

  auto before2l = std::chrono::steady_clock::now();
  p.mFirmwareConfig->insertDataIntoChannelMatrix(p.mChannelData, p.dataBytes);
  auto after2l = std::chrono::steady_clock::now();
  std::chrono::duration<double> duration2l = after2l - before2l;
}
