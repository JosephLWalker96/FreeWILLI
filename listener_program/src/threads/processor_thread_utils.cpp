#include "../pch.h"
#include "processor_thread_utils.h"
#include "../tracker/tracker.h"
#include "../io/buffer_writer.h"
#include "../shared_data_manager.h"


/**
 * @brief Waits for and retrieves data from the session buffer, with periodic buffer flushing.
 *
 * @param sess Reference to the SharedDataManager object for managing resources shared across threads.
 * @param observationBuffer Reference to the ObservationBuffer for managing flushed observations.
 * @param runtimeConfig Reference to the RuntimeConfig object for runtime details.
 * @param startLoop The start time of the current loop iteration.
 * @param dataBytes Reference to the vector that will hold the retrieved data bytes.
 */
void waitForData(SharedDataManager &sess, std::vector<uint8_t> &dataBytes)
{
    while (true)
    {

        //auto startLoop = std::chrono::system_clock::now();
        bool gotData = sess.popDataFromBuffer(dataBytes); // Retrieves data or sleeps until available
        if (gotData)
        {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15ms));
    }
}

/**
 * @brief Writes error-causing timestamps and associated data bytes to the standard error stream.
 *
 * This function logs the timestamps and corresponding byte data that caused an error to `std::cerr`.
 * Timestamps are converted to microseconds since the epoch, and byte data is formatted in hexadecimal
 * with zero padding for readability.
 *
 * @param errorTimestamps A span of TimePoint objects representing the timestamps of the errored data.
 * @param erroredDataBytes A vector of vectors containing the byte data associated with the errored timestamps.
 */
void writeDataToCerr(std::span<TimePoint> errorTimestamps, std::vector<std::vector<uint8_t>> erroredDataBytes)
{
    std::stringstream errorMessage; // Compose message to dispatch

    // Add timestamps to the error message
    errorMessage << "Timestamps of data causing error: \n";
    for (const auto &timestamp : errorTimestamps)
    {
        auto convertedTime = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
        errorMessage << convertedTime << std::endl;
    }
    errorMessage << std::endl;

    // Add errored byte data to the error message
    errorMessage << "Errored bytes of last packets: " << std::endl;
    for (const auto &byteArray : erroredDataBytes)
    {
        errorMessage << std::endl;
        for (const auto &dataByte : byteArray)
        {
            errorMessage << std::setw(2) << std::setfill('0') << static_cast<int>(dataByte);
        }
        errorMessage << std::endl;
    }
    errorMessage << std::endl;

    // Write the composed message to the standard error stream
    std::cerr << errorMessage.str();
}


/**
 * @brief Determines if the program should terminate based on the runtime duration.
 *
 * @param runtimeConfig Experiment runtime configuration and state.
 * @param startLoop The time point at the start of the current loop iteration.
 * @return true if the program should terminate, false otherwise.
 */
bool shouldTerminateProgram(const RuntimeConfig &runtimeConfig)
{

    TimePoint currentTime = std::chrono::system_clock::now();
    auto elapsedTime = currentTime - runtimeConfig.programStartTime;
    return elapsedTime >= runtimeConfig.programRuntime;
}


/**
 * @brief Converts a `TimePoint` object to a formatted string representation.
 *
 * This function converts a `TimePoint` object into a string representation that includes
 * the date, time, and microseconds. The output is formatted as "YYYY-MM-DD HH:MM:SS.mmmmmm".
 *
 * @param timePoint A `TimePoint` object representing the time to be converted.
 * @return A string representing the formatted date and time with microseconds.
 */
std::string convertTimePointToString(const TimePoint &timePoint)
{
    // Convert TimePoint to time_t to get calendar time
    std::time_t calendarTime = std::chrono::system_clock::to_time_t(timePoint);
    std::tm *utcTime = std::gmtime(&calendarTime); // Convert to UTC (or use std::localtime for local time)

    // Format the calendar time (year, month, day, hour, minute, second)
    std::ostringstream formattedTimeStream;
    formattedTimeStream << std::put_time(utcTime, "%y%m%d_%H%M%S");

    // Extract the microseconds from the TimePoint
    auto durationSinceEpoch = timePoint.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(durationSinceEpoch) % 1000000;

    // Add the microseconds to the formatted time string
    formattedTimeStream << "_" << std::setw(6) << std::setfill('0') << microseconds.count(); // Zero-pad to 6 digits

    return formattedTimeStream.str();
}

/**
 * @brief Save frequency domain data for training.
 *
 * @param filename The name of the file to save the data.
 * @param label The label to save as the first column.
 * @param frequencyDomainData The frequency domain data (complex values).
 */
void saveSpectraForTraining(const std::string &filename, int label, const Eigen::VectorXcf &frequencyDomainData)
{
    // Compute the magnitude of the spectra
    Eigen::VectorXf spectraToSave = frequencyDomainData.array().abs();

    // Check if the file exists
    std::ifstream fileCheck(filename);
    bool fileExists = fileCheck.good();
    fileCheck.close();

    // Open the file in append mode if it exists, or create a new file if it doesn't
    std::ofstream file(filename, std::ios::out | std::ios::app);
    if (!file)
    {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return;
    }

    // If the file doesn't exist, write the header
    if (!fileExists)
    {
        file << "label";
        for (int i = 0; i < spectraToSave.size(); ++i)
        {
            file << ",spectra_" << i;
        }
        file << "\n";
    }

    // Write the label and spectra values
    file << label;
    for (int i = 0; i < spectraToSave.size(); ++i)
    {
        file << "," << std::fixed << std::setprecision(6) << spectraToSave[i];
    }
    file << "\n";

    // Close the file
    file.close();
}
