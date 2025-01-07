#include "output_manager.h"

OutputManager::OutputManager(std::chrono::seconds programRuntime)
    : mFlushInterval(std::chrono::seconds(30)),
      mBufferSizeThreshold(1000),
      mLastFlushTime(std::chrono::steady_clock::now()),
      mProgramRuntime(programRuntime),
      mProgramStartTime(std::chrono::system_clock::now())
{
}

/**
 * @brief Initializes an output file for storing computed values.
 *
 * This function generates an output file name based on the current timestamp and initializes the file
 * for writing. It writes column headers to the file, including labels for PeakTime, Amplitude, DOA, TDOA,
 * and XCorr.
 *
 * @param numChannels The number of channels in the data, used to generate TDOA and XCorr labels.
 * @throws std::runtime_error If the file cannot be opened for writing.
 */
void OutputManager::initializeOutputFile(const TimePoint& timestamp, const int numChannels)
{
    mDetectionOutputFile = "deployment_files/" + convertTimePointToString(timestamp);
    // initializeOutputFile(numChannels);
    std::cout << "Creating and writing to file: " << mDetectionOutputFile << std::endl;

    std::ofstream file(mDetectionOutputFile, std::ofstream::out | std::ofstream::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Unable to open file for writing: " + mDetectionOutputFile);
    }

    std::vector<std::string> columnNames = {"PeakTime", "Amplitude", "DOA_x", "DOA_y", "DOA_z"};

    // Generate TDOA and XCorr labels
    std::vector<std::string> tdoaLabels = generateChannelComboLabels("TDOA", numChannels);
    std::vector<std::string> xcorrLabels = generateChannelComboLabels("XCorr", numChannels);

    // Combine all column names
    columnNames.insert(columnNames.end(), tdoaLabels.begin(), tdoaLabels.end());
    columnNames.insert(columnNames.end(), xcorrLabels.begin(), xcorrLabels.end());

    // Write the column names to the file, separated by commas
    for (size_t i = 0; i < columnNames.size(); ++i)
    {
        file << columnNames[i];
        if (i < columnNames.size() - 1) file << ",";
    }
    file << std::endl;

    file.close();
}

/**
 * @brief Generates a vector of labels with a given prefix for channel combinations.
 *
 * This function generates a list of labels based on the specified prefix and channel count.
 * Each label represents a unique combination of channels, encoded as "prefixXY" where X
 * and Y are the signal and reference channel numbers.
 *
 * @param labelPrefix A string prefix to prepend to each generated label.
 * @param numChannels The total number of channels for which labels are generated.
 * @return A vector of strings containing the generated labels.
 */
std::vector<std::string> OutputManager::generateChannelComboLabels(const std::string& labelPrefix, int numChannels)
{
    std::vector<std::string> labels;

    // Generate labels for all unique channel combinations
    for (int signalChannel = 1; signalChannel < numChannels; ++signalChannel)
    {
        for (int referenceChannel = signalChannel + 1; referenceChannel <= numChannels; ++referenceChannel)
        {
            labels.push_back(labelPrefix + std::to_string(signalChannel * 10 + referenceChannel));
        }
    }

    return labels;
}

/**
 * @brief Writes the buffer data to the output file and resets the flush time.
 */
void OutputManager::write()
{
    auto beforeFlush = std::chrono::steady_clock::now();
    appendBufferToFile();
    auto afterFlush = std::chrono::steady_clock::now();
    std::chrono::duration<double> durationFlush = afterFlush - beforeFlush;

    mLastFlushTime = std::chrono::steady_clock::now();
}

/**
 * @brief Clears all data from the buffer.
 */
void OutputManager::clearBuffer()
{
    mBuffer.mAmps.clear();
    mBuffer.mDoaX.clear();
    mBuffer.mDoaY.clear();
    mBuffer.mDoaZ.clear();
    mBuffer.mTdoaVector.clear();
    mBuffer.mXCorrAmps.clear();
    mBuffer.mPeakTimes.clear();
}

/**
 * @brief Appends a single data row to the buffer.
 */
void OutputManager::appendToBuffer(const float peakAmp, const float doaX, const float doaY, const float doaZ,
                                   const Eigen::VectorXf& tdoaVector, const Eigen::VectorXf& xCorrAmps,
                                   const TimePoint& peakTime)
{
    mBuffer.mAmps.push_back(peakAmp);
    mBuffer.mDoaX.push_back(doaX);
    mBuffer.mDoaY.push_back(doaY);
    mBuffer.mDoaZ.push_back(doaZ);
    mBuffer.mTdoaVector.push_back(tdoaVector);
    mBuffer.mXCorrAmps.push_back(xCorrAmps);
    mBuffer.mPeakTimes.push_back(peakTime);
}

/**
 * @brief Appends buffer data to the specified file.
 */
void OutputManager::appendBufferToFile()
{
    std::ofstream file(mDetectionOutputFile, std::ofstream::out | std::ofstream::app);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Unable to open file for appending: " + mDetectionOutputFile);
    }

    size_t dataSize = mBuffer.mAmps.size();
    if (mBuffer.mDoaX.size() != dataSize || mBuffer.mDoaY.size() != dataSize || mBuffer.mDoaZ.size() != dataSize ||
        mBuffer.mTdoaVector.size() != dataSize || mBuffer.mXCorrAmps.size() != dataSize ||
        mBuffer.mPeakTimes.size() != dataSize)
    {
        throw std::runtime_error("Error: Mismatched buffer sizes in BufferStruct.");
    }

    int numChannelPairs = mBuffer.mTdoaVector[0].size();

    for (size_t i = 0; i < dataSize; ++i)
    {
        std::vector<std::string> rowData;

        auto timePoint = mBuffer.mPeakTimes[i];
        auto timeSinceEpoch = std::chrono::duration_cast<std::chrono::microseconds>(timePoint.time_since_epoch());
        rowData.emplace_back(std::to_string(timeSinceEpoch.count()));

        rowData.push_back(std::to_string(mBuffer.mAmps[i]));
        rowData.push_back(std::to_string(mBuffer.mDoaX[i]));
        rowData.push_back(std::to_string(mBuffer.mDoaY[i]));
        rowData.push_back(std::to_string(mBuffer.mDoaZ[i]));

        Eigen::VectorXf tdoaVec = mBuffer.mTdoaVector[i];
        if (tdoaVec.size() != numChannelPairs)
        {
            throw std::runtime_error("Error: Inconsistent TDOA vector size at index " + std::to_string(i));
        }
        for (int j = 0; j < tdoaVec.size(); ++j)
        {
            rowData.push_back(std::to_string(tdoaVec[j]));
        }

        Eigen::VectorXf xcorrVec = mBuffer.mXCorrAmps[i];
        if (xcorrVec.size() != numChannelPairs)
        {
            throw std::runtime_error("Error: Inconsistent XCorr vector size at index " + std::to_string(i));
        }
        for (int j = 0; j < xcorrVec.size(); ++j)
        {
            rowData.push_back(std::to_string(xcorrVec[j]));
        }

        for (size_t k = 0; k < rowData.size(); ++k)
        {
            file << rowData[k];
            if (k < rowData.size() - 1) file << ",";
        }
        file << std::endl;
    }

    file.close();
}

/**
 * @brief Determines if the buffer should be flushed based on its size or time since the last flush.
 */
void OutputManager::flushBufferIfNecessary()
{
    int bufferSize = mBuffer.mPeakTimes.size();

    if (bufferSize == 0)
    {
        return;
    }

    auto timeSinceLastFlush =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - mLastFlushTime);

    if (bufferSize >= mBufferSizeThreshold || mFlushInterval <= timeSinceLastFlush)
    {
        write();
        clearBuffer();
    }
}

/**
 * @brief Writes error-causing data and associated data bytes to the
 * standard error stream.
 *
 */
void OutputManager::writeDataToCerr(std::span<TimePoint> errorTimestamps,
                                    const std::vector<std::vector<uint8_t>>& erroredDataBytes)
{
    std::stringstream errorMessage;  // Compose message to dispatch

    // Add timestamps to the error message
    errorMessage << "Timestamps of data causing error: \n";
    for (const auto& timestamp : errorTimestamps)
    {
        auto convertedTime =
            std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
        errorMessage << convertedTime << std::endl;
    }
    errorMessage << std::endl;

    // Add errored byte data to the error message
    errorMessage << "Errored bytes of last packets: " << std::endl;
    for (const auto& byteArray : erroredDataBytes)
    {
        errorMessage << std::endl;
        for (const auto& dataByte : byteArray)
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
 * @brief Save frequency domain data for training.
 *
 * @param filename The name of the file to save the data.
 * @param label The label to save as the first column.
 * @param frequencyDomainData The frequency domain data (complex values).
 */
void OutputManager::saveSpectraForTraining(const std::string& filename, int label,
                                           const Eigen::VectorXcf& frequencyDomainData)
{
    // Compute the magnitude of the spectra
    Eigen::VectorXf spectraToSave = frequencyDomainData.array().abs();

    // Check if the file exists
    std::ifstream fileCheck(filename);
    bool fileExists = fileCheck.good();
    fileCheck.close();

    // Open the file in append mode if it exists, or create a new file if it
    // doesn't
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

/**
 * @brief Determines if the program should terminate based on the runtime
 * duration.
 *
 */
void OutputManager::terminateProgramIfNecessary()
{
    TimePoint currentTime = std::chrono::system_clock::now();
    auto elapsedTime = currentTime - mProgramStartTime;
    if (elapsedTime >= mProgramRuntime)
    {
        write();
        std::cout << "Terminating program... duration reached\n";
        std::exit(0);
    }
}