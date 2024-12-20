#pragma once
#include "../firmware_config.h"
#include "../pch.h"
#include "../shared_data_manager.h"
#include "../algorithms/doa_utils.h"
#include "../algorithms/gcc_phat.h"
#include "../algorithms/hydrophone_position_processing.h"
#include "../algorithms/fir_filter.h"
#include "../algorithms/time_domain_detectors.h"
#include "../algorithms/frequency_domain_detectors.h"
#include "../algorithms/IMU_processor.h"
#include "../io/buffer_writer.h"
#include "../io/socket_manager.h"
#include "../ML/onnx_model.h"
#include "../tracker/tracker.h"
#include "processor_thread_utils.h"
#include "../main_utils.h"

using TimePoint = std::chrono::system_clock::time_point;

class Pipeline {
public:
    Pipeline(SharedDataManager &sharedSess, const PipelineVariables &pipelineVariables);

    void process();

    // Public member variables
    std::chrono::seconds programRuntime;
    TimePoint programStartTime;

private:
    // Private member variables
    ObservationBuffer observationBuffer;

    SharedDataManager &sess;
    FirmwareConfig firmwareConfig;

    Eigen::MatrixXf channelData;
    std::string detectionOutputFile;
    float speedOfSound;
    std::string receiverPositionsPath;
    std::vector<std::vector<uint8_t>> dataBytesSaved;
    std::vector<TimePoint> dataTimes;

    std::unique_ptr<IFrequencyDomainStrategy> filter;
    std::unique_ptr<ONNXModel> onnxModel;
    std::unique_ptr<Tracker> tracker;
    std::unique_ptr<TimeDomainDetector> timeDomainDetector; 
    std::unique_ptr<FrequencyDomainDetector> frequencyDomainDetector;

    void dataProcessor();

    void initilializeOutputFiles(TimePoint& timepoint);
    void terminateProgramIfNecessary();
    void obtainAndProcessByteData(std::vector<std::vector<uint8_t>>& dataBytes, bool& previousTimeSet, TimePoint& previousTime);
};