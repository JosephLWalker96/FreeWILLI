#include "utils.h"

#include "pch.h"

/**
 * @brief Parses the JSON configuration file to initialize socket and pipeline variables.
 */
auto parseJsonConfig(const std::string& jsonFilePath) -> std::tuple<SocketVariables, PipelineVariables>
{
    std::ifstream inputFile(jsonFilePath);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Unable to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonConfig;
    inputFile >> jsonConfig;

    SocketVariables socketVariables;
    PipelineVariables pipelineVariables;

    // SocketVariables parameters
    socketVariables.ipAddress = jsonConfig.at("networkIPAddress").get<std::string>();
    socketVariables.port = jsonConfig.at("networkPort").get<int>();

    // PipelineVariables parameters
    pipelineVariables.integrationTesting = jsonConfig.at("enableIntegrationTesting").get<bool>();
    pipelineVariables.firmware = jsonConfig.at("firmware").get<std::string>();
    pipelineVariables.speedOfSound = jsonConfig.at("speedOfSound_mps").get<float>();
    pipelineVariables.loggingDirectory = jsonConfig.at("logDirectory").get<std::string>();
    pipelineVariables.timeDomainDetector = jsonConfig.at("timeDomainDetector").get<std::string>();
    pipelineVariables.timeDomainThreshold = jsonConfig.at("timeDomainThreshold").get<float>();
    pipelineVariables.frequencyDomainStrategy = jsonConfig.at("frequencyDomainStrategy").get<std::string>();
    pipelineVariables.frequencyDomainDetector = jsonConfig.at("frequencyDomainDetector").get<std::string>();
    pipelineVariables.energyDetectionThreshold = jsonConfig.at("frequencyDomainThreshold").get<float>();
    pipelineVariables.filterWeightsPath = jsonConfig.at("filterWeightsFile").get<std::string>();
    pipelineVariables.receiverPositionsPath = jsonConfig.at("receiverPositionsFile").get<std::string>();
    pipelineVariables.enableTracking = jsonConfig.at("enableTracking").get<bool>();
    pipelineVariables.clusterFrequencyInSeconds =
        std::chrono::seconds(jsonConfig.at("clusteringIntervalSeconds").get<int>());
    pipelineVariables.clusterWindowInSeconds =
        std::chrono::seconds(jsonConfig.at("clusteringWindowSeconds").get<int>());
    pipelineVariables.onnxModelPath = jsonConfig.at("onnxModelPath").get<std::string>();
    pipelineVariables.onnxModelNormalizationPath = jsonConfig.at("onnxNormalizationParams").get<std::string>();

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
