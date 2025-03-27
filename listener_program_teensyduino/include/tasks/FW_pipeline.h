#pragma once

// #include "../ML/onnx_model.h"
#include "algorithms/doa_utils.h"
// #include "algorithms/fir_filter.h"
#include "algorithms/frequency_domain_detectors.h"
// #include "algorithms/gcc_phat.h"
#include "algorithms/hydrophone_position_processing.h"
#include "algorithms/time_domain_detectors.h"
#include "firmware_1240.h"
#include "shared_data_manager.h"
// #include "../tracker/tracker.h"

#include "pch.h"
#include "utils.h"
// class ONNXModel;

struct Pipeline {
  // Private member variables
  float mSpeedOfSound;
  std::string mReceiverPositionsPath;
  std::vector<std::vector<uint8_t>> dataBytes;
  std::vector<TimePoint> dataTimes;

  std::unique_ptr<const Firmware1240> mFirmwareConfig;
  Eigen::MatrixXf mChannelData;
  // std::unique_ptr<IFrequencyDomainStrategy> mFilter;
  std::unique_ptr<ITimeDomainDetector> mTimeDomainDetector;
  std::unique_ptr<IFrequencyDomainDetector> mFrequencyDomainDetector;
  // std::unique_ptr<ONNXModel> mOnnxModel = nullptr;
  // std::unique_ptr<Tracker> mTracker = nullptr;
  // GCC_PHAT mComputeTDOAs;
};

void FW_InitPipeline(const PipelineVariables& pipelineVariables);
void PipelineTask(void*);
