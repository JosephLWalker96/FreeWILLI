#pragma once
#include "../ML/onnx_model.h"
#include "../algorithms/doa_utils.h"
#include "../algorithms/fir_filter.h"
#include "../algorithms/frequency_domain_detectors.h"
#include "../algorithms/gcc_phat.h"
#include "../algorithms/hydrophone_position_processing.h"
#include "../algorithms/imu_processing.h"
#include "../algorithms/time_domain_detectors.h"
#include "../firmware_1240.h"
#include "../io/buffer_writer.h"
#include "../io/socket_manager.h"
#include "../main_utils.h"
#include "../pch.h"
#include "../shared_data_manager.h"
#include "../tracker/tracker.h"
#include "processor_thread_utils.h"

using TimePoint = std::chrono::system_clock::time_point;

class Pipeline
{
   public:
    Pipeline(SharedDataManager& sharedSess, const PipelineVariables& pipelineVariables);

    void process();

    // Public member variables
    std::chrono::seconds programRuntime;
    TimePoint programStartTime;

   private:
    // Private member variables
    ObservationBuffer observationBuffer;

    SharedDataManager& sess;

    Eigen::MatrixXf channelData;
    std::string detectionOutputFile;
    float speedOfSound;
    std::string receiverPositionsPath;
    // std::vector<std::vector<uint8_t>> dataBytesSaved;
    std::vector<std::vector<uint8_t>> dataBytes;
    std::vector<TimePoint> dataTimes;

    std::unique_ptr<Firmware1240> firmwareConfig = nullptr;
    std::unique_ptr<ITimeDomainDetector> timeDomainDetector = nullptr;
    std::unique_ptr<IFrequencyDomainStrategy> filter = nullptr;
    std::unique_ptr<IFrequencyDomainDetector> frequencyDomainDetector = nullptr;
    std::unique_ptr<ONNXModel> onnxModel = nullptr;
    std::unique_ptr<Tracker> tracker = nullptr;

    void dataProcessor();

    void initilializeOutputFiles();
    void terminateProgramIfNecessary();
    void obtainAndProcessByteData(bool& previousTimeSet, TimePoint& previousTime);
};