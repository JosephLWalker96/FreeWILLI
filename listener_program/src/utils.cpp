#include "utils.h"

#include "pch.h"

/**
 * @brief Parses the JSON configuration file to initialize socket and pipeline variables.
 */
auto parseJsonConfig() -> std::tuple<SocketVariables, PipelineVariables>
{
    // We do not parse JSON file for microcontroller implementation
    /*
    std::ifstream inputFile(jsonFilePath);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Unable to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonConfig;
    inputFile >> jsonConfig;
    */

    SocketVariables socketVariables;
    PipelineVariables pipelineVariables;

    // Configure SocketVariables parameters
    // socketVariables.udpIp = jsonConfig.at("IPAddress").get<std::string>();
    socketVariables.udpIp = "self";  // Tanish
    // socketVariables.udpPort = jsonConfig.at("Port").get<int>();
    socketVariables.udpPort = 1045;  // Tanish

    // Configure PipelineVariables parameters
    // pipelineVariables.useImu = jsonConfig.at("Firmware1240_with_IMU").get<bool>();
    pipelineVariables.useImu = false;  // Tanish
    // pipelineVariables.speedOfSound = jsonConfig.at("SpeedOfSound").get<float>();
    pipelineVariables.speedOfSound = 1482.965459;  // Tanish
    // pipelineVariables.timeDomainDetector = jsonConfig.at("Time_Domain_Detector").get<std::string>();
    pipelineVariables.timeDomainDetector = "PeakAmplitude";  // Tanish
    // pipelineVariables.timeDomainThreshold = jsonConfig.at("Time_Domain_Threshold").get<float>();
    pipelineVariables.timeDomainThreshold = 50;  // Tanish

    // Variables defined bellow are not used

    // pipelineVariables.frequencyDomainStrategy = jsonConfig.at("Frequency_Domain_Strategy").get<std::string>();
    pipelineVariables.frequencyDomainStrategy = "Filter";  // Tanish
    // pipelineVariables.frequencyDomainDetector = jsonConfig.at("Frequency_Domain_Detector").get<std::string>();
    pipelineVariables.frequencyDomainDetector = "AverageEnergy";  // Tanish
    // pipelineVariables.energyDetectionThreshold = jsonConfig.at("Frequency_Domain_Threshold").get<float>();
    pipelineVariables.energyDetectionThreshold = 100;  // Tanish
    // pipelineVariables.filterWeightsPath = jsonConfig.at("FilterWeights").get<std::string>();
    pipelineVariables.filterWeightsPath = "filters/highpass_taps@101_cutoff@20k_window@hamming_fs@100k.txt";  // Tanish
    // pipelineVariables.receiverPositionsPath = jsonConfig.at("ReceiverPositions").get<std::string>();
    pipelineVariables.receiverPositionsPath = "receiver_pos/SOCAL_H_72_HS_harp4chPar_recPos.txt";  // Tanish
    // pipelineVariables.enableTracking = jsonConfig.at("Enable_Tracking").get<bool>();
    pipelineVariables.enableTracking = false;  // Tanish
    // pipelineVariables.clusterFrequencyInSeconds =
    // std::chrono::seconds(jsonConfig.at("Cluster_Frequency_In_Seconds").get<int>());
    pipelineVariables.clusterFrequencyInSeconds = std::chrono::seconds(60);  // Tanish
    // pipelineVariables.clusterWindowInSeconds =
    // std::chrono::seconds(jsonConfig.at("Cluster_Window_In_Seconds").get<int>());
    pipelineVariables.clusterWindowInSeconds = std::chrono::seconds(30);  // Tanish
    // pipelineVariables.onnxModelPath = jsonConfig.at("ONNX_model_path").get<std::string>();
    pipelineVariables.onnxModelPath = "";  // Tanish
    //    pipelineVariables.onnxModelNormalizationPath = jsonConfig.at("ONNX_model_normalization").get<std::string>();
    pipelineVariables.onnxModelNormalizationPath = "../../TestOnnx/scaler_params.json";  // Tanish

    return std::make_tuple(socketVariables, pipelineVariables);
}

/**
 * @brief Prints whether the program is running in Debug or Release mode.
 */
void printMode()
{
#ifdef DEBUG
    std::cout << "Running Debug Mode" << std::endl;
#else
    std::cout << "Running Release Mode" << std::endl;
#endif
}

/**
 * @brief Converts a `TimePoint` object to a formatted string representation.
 *
 * This function converts a `TimePoint` object into a string representation that
 * includes the date, time, and microseconds. The output is formatted as
 * "YYYY-MM-DD HH:MM:SS.mmmmmm".
 *
 * @param timePoint A `TimePoint` object representing the time to be converted.
 * @return A string representing the formatted date and time with microseconds.
 */
std::string convertTimePointToString(const TimePoint& timePoint)
{
    // Convert TimePoint to time_t to get calendar time
    std::time_t calendarTime = std::chrono::system_clock::to_time_t(timePoint);
    std::tm* utcTime = std::gmtime(&calendarTime);  // Convert to UTC (or use std::localtime
                                                    // for local time)

    // Format the calendar time (year, month, day, hour, minute, second)
    std::ostringstream formattedTimeStream;
    formattedTimeStream << std::put_time(utcTime, "%y%m%d_%H%M%S");

    // Extract the microseconds from the TimePoint
    auto durationSinceEpoch = timePoint.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(durationSinceEpoch) % 1000000;

    // Add the microseconds to the formatted time string
    formattedTimeStream << "_" << std::setw(6) << std::setfill('0') << microseconds.count();  // Zero-pad to 6 digits

    return formattedTimeStream.str();
}
