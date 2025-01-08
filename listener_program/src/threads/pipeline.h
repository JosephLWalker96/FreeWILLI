#pragma once

#include "../ML/onnx_model.h"
#include "../algorithms/doa_utils.h"
#include "../algorithms/fir_filter.h"
#include "../algorithms/frequency_domain_detectors.h"
#include "../algorithms/gcc_phat.h"
#include "../algorithms/hydrophone_position_processing.h"
#include "../algorithms/time_domain_detectors.h"
#include "../firmware_1240.h"
#include "../io/output_manager.h"
#include "../io/socket_manager.h"
#include "../shared_data_manager.h"
#include "../tracker/tracker.h"

class PipelineVariables;

class Pipeline
{
   public:
    Pipeline(OutputManager& outputManager, SharedDataManager& sharedSess, const PipelineVariables& pipelineVariables);

    void process();

   private:
    // Private member variables
    OutputManager& mOutputManager;
    SharedDataManager& mSharedDataManager;

    const float mSpeedOfSound;
    std::string mReceiverPositionsPath;
    std::vector<std::vector<uint8_t>> dataBytes;
    std::vector<TimePoint> dataTimes;

    std::unique_ptr<const Firmware1240> mFirmwareConfig = nullptr;
    Eigen::MatrixXf mChannelData;
    std::unique_ptr<IFrequencyDomainStrategy> mFilter = nullptr;
    std::unique_ptr<ITimeDomainDetector> mTimeDomainDetector = nullptr;
    std::unique_ptr<IFrequencyDomainDetector> mFrequencyDomainDetector = nullptr;
    std::unique_ptr<ONNXModel> mOnnxModel = nullptr;
    std::unique_ptr<Tracker> mTracker = nullptr;
    GCC_PHAT mComputeTDOAs;
    void dataProcessor();
    void initializeOutputFiles(bool& previousTimeSet, TimePoint& previousTime);
    void obtainAndProcessByteData(bool& previousTimeSet, TimePoint& previousTime);
    void handleProcessingError(const std::exception& e);
};