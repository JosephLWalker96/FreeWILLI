#include "../pch.h"
#include "processor_thread_utils.h"
#include "../tracker/tracker.h"
#include "../algorithms/linear_algebra_utils.h"
#include "../io/buffer_writer.h"
#include "../session.h"

/**
 * @brief Loads hydrophone positions from a CSV file into an Eigen matrix.
 *
 * This function reads a CSV file containing hydrophone positions and stores the data in an Eigen matrix.
 *
 * @param filename The name (or path) of the CSV file containing hydrophone positions.
 * @return Eigen::MatrixXf A matrix containing the hydrophone positions.
 * @throws std::ios_base::failure If the file cannot be opened or read.
 */
Eigen::MatrixXf loadHydrophonePositionsFromFile(const std::string &filename)
{
    std::ifstream inputFile(filename);

    if (!inputFile.is_open())
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Unable to open filter file '" << filename << "'." << std::endl;
        throw std::ios_base::failure(msg.str());
    }

    std::cout << "Reading hydrophone positions..." << std::endl;

    std::vector<std::vector<float>> tempPositions;
    std::string line;
    std::string token;

    // First, read the data into a temporary vector of vectors to determine dimensions
    int numRows = 0;
    int numCols = 0;
    while (std::getline(inputFile, line))
    {
        std::stringstream ss(line);
        std::vector<float> rowData;
        while (std::getline(ss, token, ','))
        {
            rowData.push_back(std::stof(token)); // Convert token to double
        }
        if (numCols == 0)
        {
            numCols = rowData.size(); // Set number of columns based on the first row
        }
        tempPositions.push_back(rowData);
        numRows++;
    }

    // Now create the Eigen matrix with determined dimensions
    Eigen::MatrixXf positions(numRows, numCols);

    // Populate the Eigen matrix with the data from the temp_positions vector
    for (int i = 0; i < numRows; ++i)
    {
        for (int j = 0; j < numCols; ++j)
        {
            positions(i, j) = tempPositions[i][j];
        }
    }
    return positions;
}

/**
 * @brief Computes relative positions between hydrophones.
 *
 * This function calculates relative positions between all pairs of hydrophones based on their absolute positions.
 *
 * @param positions An Eigen matrix containing the absolute positions of hydrophones.
 * @return Eigen::MatrixXf A matrix containing the relative positions between hydrophones.
 */
Eigen::MatrixXf calculateRelativePositions(const Eigen::MatrixXf &positions)
{
    int numRows = positions.rows();
    int numCols = positions.cols();
    int numRelativePositions = numRows * (numRows - 1) / 2;

    Eigen::MatrixXf relativePositions(numRelativePositions, numCols);

    int index = 0;
    for (int i = 0; i < numRows; ++i)
    {
        for (int j = i + 1; j < numRows; ++j)
        {
            for (int k = 0; k < numCols; ++k)
            {
                relativePositions(index, k) = positions(j, k) - positions(i, k);
            }
            index++;
        }
    }

    return relativePositions;
}

/**
 * @brief Loads hydrophone positions from a CSV file and calculates relative positions.
 *
 * This function reads a CSV file containing hydrophone positions, converts the data into an Eigen matrix,
 * and calculates relative positions between all hydrophone pairs.
 *
 * @param filename The name (or path) of the CSV file containing the hydrophone positions.
 * @return Eigen::MatrixXf A matrix containing the relative positions between hydrophones.
 * @throws std::ios_base::failure If the file cannot be opened or read.
 */
Eigen::MatrixXf getHydrophoneRelativePositions(const std::string &filename)
{
    Eigen::MatrixXf positions = loadHydrophonePositionsFromFile(filename);

    return calculateRelativePositions(positions);
}

/**
 * @brief Precomputes hydrophone-related matrices required for TDOA and DOA estimation.
 *
 * @param receiverPositionsPath Path to the file containing hydrophone positions.
 * @param precomputedP Reference to a matrix for storing the precomputed P matrix.
 * @param basisMatrixU Reference to a matrix for storing the U matrix from SVD decomposition.
 * @param rankOfHydrophoneMatrix Reference to an integer for storing the rank of the hydrophone position matrix.
 */
void hydrophoneMatrixDecomposition(const Eigen::MatrixXf hydrophonePositions, Eigen::MatrixXf &precomputedP,
                                   Eigen::MatrixXf &basisMatrixU, int &rankOfHydrophoneMatrix)
{
    // Compute SVD and rank
    auto svdDecomposition = computeSvd(hydrophonePositions);
    rankOfHydrophoneMatrix = computeRank(hydrophonePositions);

    // Precompute matrices
    precomputedP = precomputePseudoInverse(svdDecomposition);
    basisMatrixU = svdDecomposition.matrixU();
}

/**
 * @brief Waits for and retrieves data from the session buffer, with periodic buffer flushing.
 *
 * @param sess Reference to the Session object for managing the shared buffer.
 * @param observationBuffer Reference to the ObservationBuffer for managing flushed observations.
 * @param runtimeConfig Reference to the RuntimeConfig object for runtime details.
 * @param startLoop The start time of the current loop iteration.
 * @param dataBytes Reference to the vector that will hold the retrieved data bytes.
 */
void waitForData(Session &sess, ObservationBuffer &observationBuffer, RuntimeConfig &runtimeConfig,
                 std::vector<uint8_t> &dataBytes)
{
    while (true)
    {

        auto startLoop = std::chrono::system_clock::now();

        bool gotData = sess.popDataFromBuffer(dataBytes); // Retrieves data or sleeps until available
        if (gotData)
        {
            return;
        }
        if (observationBuffer.timeToFlushBuffer(runtimeConfig, startLoop))
        {
            observationBuffer.write(runtimeConfig.detectionOutputFile);
            observationBuffer.clearBuffer();
        }
        if (shouldTerminateProgram(runtimeConfig, startLoop))
        {
            observationBuffer.write(runtimeConfig.detectionOutputFile);
            std::cout << "Terminating program... duration reached \n";
            std::exit(0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15ms));
    }
}

/**
 * @brief Converts raw data bytes to float values and appends them to the provided data segment.
 *
 * @param dataSegment A vector of floats where the converted data will be stored.
 * @param dataBytes A span of raw data bytes to be converted.
 * @param dataSize The size of the data to process (excluding the header).
 * @param headerSize The size of the header to skip at the beginning of the data bytes.
 */
/*
template <typename Alloc>
void convertAndAppend(std::vector<float, Alloc> &dataSegment, std::span<uint8_t> dataBytes, const int &dataSize, const int &headerSize)
{
    for (size_t i = 0; i < dataSize; i += 2)
    {
        float value = static_cast<float>(static_cast<uint16_t>(dataBytes[headerSize + i]) << 8) +
                      static_cast<float>(dataBytes[headerSize + i + 1]);
        value -= 32768.0f;
        dataSegment.push_back(value);
    }
}
*/

/*
void convertAndAppend(Eigen::MatrixXf &channelMatrix, 
                      std::span<uint8_t> dataBytes, 
                      int dataSize, 
                      int headerSize)
{
    // Each sample = 2 bytes
    const size_t numSamples = dataSize / 2;
    const size_t numRows = channelMatrix.rows();
    const size_t oldCols = channelMatrix.cols();

    // Calculate how many new columns are needed
    const size_t newCols = (numSamples + numRows - 1) / numRows;

    // Resize the matrix to accommodate these new columns
    channelMatrix.conservativeResize(numRows, oldCols + newCols);

    // Pointer to the start of the Eigen matrix's data (column-major layout)
    float* __restrict__ matrixPtr = channelMatrix.data();

    // Pointer to incoming samples (skipping the header)
    const uint8_t* __restrict__ inPtr = dataBytes.data() + headerSize;

    // Compute the offset where the new columns begin
    const size_t startOffset = oldCols * numRows;  

    // Single-pass conversion & storage
    // We'll write each sample directly into the correct memory location.
    #pragma omp simd
    for (size_t i = 0; i < numSamples; ++i)
    {
        // Convert 2-byte sample to float
        uint16_t sample = (static_cast<uint16_t>(inPtr[2 * i]) << 8) | inPtr[2 * i + 1];
        float value = static_cast<float>(sample) - 32768.0f;

        // Column-major: matrixPtr[col * numRows + row]
        // row = i % numRows, col = oldCols + i / numRows
        // => memory offset = startOffset + i
        matrixPtr[startOffset + i] = value;
    }
}
*/
void convertAndAppend(Eigen::MatrixXf &channelMatrix, 
                      std::span<uint8_t> dataBytes, 
                      int dataSize, 
                      int headerSize,
                      int &counter)
{
    // Each sample = 2 bytes
    const size_t numSamples = dataSize / 2;
    const size_t numRows = channelMatrix.rows();

    // Calculate how many columns the new data will occupy
    const size_t newCols = (numSamples + numRows - 1) / numRows;

    // Pointer to the start of the Eigen matrix's data (column-major layout)
    float* __restrict__ matrixPtr = channelMatrix.data();

    // Pointer to incoming samples (skipping the header)
    const uint8_t* __restrict__ inPtr = dataBytes.data() + headerSize;

    // Compute the offset where the new columns begin based on the counter
    const size_t startOffset = counter * numRows;

    // Single-pass conversion & storage
    // We'll write each sample directly into the correct memory location.
    #pragma omp simd
    for (size_t i = 0; i < numSamples; ++i)
    {
        // Convert 2-byte sample to float
        uint16_t sample = (static_cast<uint16_t>(inPtr[2 * i]) << 8) | inPtr[2 * i + 1];
        float value = static_cast<float>(sample) - 32768.0f;

        // Write to matrix at the computed offset
        matrixPtr[startOffset + i] = value;
    }

    // Increment the counter by the number of columns written
    counter += newCols;
}


/**
 * @brief Generates a timestamp from raw data bytes.
 *
 * @param dataBytes A vector of bytes representing the timestamp data.
 * @param numChannels The number of data channels (not directly used in this function but passed for compatibility).
 * @return A `TimePoint` representing the timestamp.
 * @throws std::runtime_error if `std::mktime` fails to convert the timestamp.
 */
TimePoint generateTimestamp(std::vector<uint8_t> &dataBytes, const int numChannels)
{
    std::tm timeStruct{};
    timeStruct.tm_year = static_cast<int>(dataBytes[0]) + 2000 - 1900;
    timeStruct.tm_mon = static_cast<int>(dataBytes[1]) - 1;
    timeStruct.tm_mday = static_cast<int>(dataBytes[2]);
    timeStruct.tm_hour = static_cast<int>(dataBytes[3]);
    timeStruct.tm_min = static_cast<int>(dataBytes[4]);
    timeStruct.tm_sec = static_cast<int>(dataBytes[5]);

    int64_t microseconds = (static_cast<int64_t>(dataBytes[6]) << 24) +
                           (static_cast<int64_t>(dataBytes[7]) << 16) +
                           (static_cast<int64_t>(dataBytes[8]) << 8) +
                           static_cast<int64_t>(dataBytes[9]);

    std::time_t timeResult = std::mktime(&timeStruct);

    if (timeResult == std::time_t(-1))
    {
        throw std::runtime_error("Error: failure in mktime.");
    }

    auto currentTime = std::chrono::system_clock::from_time_t(timeResult);
    currentTime += std::chrono::microseconds(microseconds);

    return currentTime;
}

/**
 * @brief Checks for data errors in a session based on timing and packet size.
 *
 * @param session The session containing data times and saved bytes.
 * @param dataBytes A vector of bytes representing the received data.
 * @param microIncrement The expected microsecond increment between timestamps.
 * @param isPreviousTimeSet A reference to a boolean indicating if the previous timestamp was set.
 * @param previousTime A reference to the previous timestamp for comparison.
 * @param packetSize The expected size of a data packet.
 * @return True if errors are found, false otherwise.
 * @throws std::runtime_error if the data packet size is incorrect.
 */
bool checkForDataErrors(Session &session, std::vector<uint8_t> &dataBytes, const int microIncrement, bool &isPreviousTimeSet,
                        TimePoint &previousTime, const int packetSize)
{
    auto currentTime = session.dataTimes.back();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count();

    if (isPreviousTimeSet && (elapsedTime != microIncrement))
    {
        std::stringstream errorMsg;
        errorMsg << "Error: Time not incremented by " << microIncrement << ". Elapsed time: " << elapsedTime << std::endl;

        std::cerr << errorMsg.str();
        writeDataToCerr(session.dataTimes, session.dataBytesSaved);
        session.dataTimes.clear();
        session.dataSegment.clear();
        session.dataBytesSaved.clear();
        previousTime = std::chrono::time_point<std::chrono::system_clock>::min();
        isPreviousTimeSet = false;
        return true;
    }
    else if (dataBytes.size() != packetSize)
    {
        std::stringstream errorMsg;
        errorMsg << "Error: Incorrect number of bytes in packet. Expected: " << packetSize
                 << ", Received: " << dataBytes.size() << std::endl;
        throw std::runtime_error(errorMsg.str());
    }

    previousTime = currentTime;
    isPreviousTimeSet = true;
    return false;
}

/**
 * @brief Processes interleaved data into separate channels.
 *
 * @param interleavedData A span of floats containing interleaved data from multiple channels.
 * @param channelMatrix A reference to an Eigen matrix to store the separated channel data. Each column represents a channel.
 * @param numChannels The number of channels in the interleaved data.
 */
void processSegmentInterleaved(std::span<float> interleavedData, Eigen::MatrixXf &channelMatrix,
                               const int numChannels)
{
    // Calculate the number of samples per channel
    size_t numSamples = interleavedData.size() / numChannels;

#ifdef __ARM_NEON
    // Add ARM NEON-specific optimization code here, if applicable
#else
    // Add non-ARM implementation or fallback code here, if necessary
#endif

    // Ensure the channelMatrix has the correct dimensions
    // channelMatrix.resize(numSamples, numChannels); // Uncomment if resizing is required

    // Iterate through the interleaved data and assign values to the corresponding channel matrix entries
    for (unsigned int channel = 0; channel < numChannels; ++channel)
    {
        for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            channelMatrix(sampleIndex, channel) = interleavedData[sampleIndex * numChannels + channel];
        }
    }
}

/*
void processSegmentInterleavedNEW(std::span<float> interleavedData, Eigen::MatrixXf &channelMatrix, const int numChannels)
{
    size_t totalSamples = interleavedData.size();
    size_t numSamples = totalSamples / numChannels;

    channelMatrix.resize(numSamples, numChannels);

    const float *inputData = interleavedData.data();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float *outPtr = &channelMatrix(0, channel);
        const float *inPtr = inputData + channel;

        size_t i = 0;
        for (; i + 4 <= numSamples; i += 4)
        {
            // Gather the elements
            float t0 = inPtr[i * (size_t)numChannels];
            float t1 = inPtr[(i + 1) * (size_t)numChannels];
            float t2 = inPtr[(i + 2) * (size_t)numChannels];
            float t3 = inPtr[(i + 3) * (size_t)numChannels];

            float32x2_t low = vld1_f32(&t0);  // loads t0,t1
            float32x2_t high = vld1_f32(&t2); // loads t2,t3
            float32x4_t vec = vcombine_f32(low, high);

            vst1q_f32(&outPtr[i], vec);
        }

        // Handle remainder (not a multiple of 4)
        for (; i < numSamples; ++i)
        {
            outPtr[i] = inPtr[i * (size_t)numChannels];
        }
    }
}
*/

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
 * @brief Initializes an output file for storing tracking results.
 *
 * This function generates an output file name based on the current timestamp and initializes the file
 * for writing. It writes column headers to the file, including labels for PeakTime, Amplitude, DOA, TDOA,
 * and XCorr. If a tracker object is provided, its output file is also set.
 *
 * @param outputFile A reference to a string where the output file name will be stored.
 * @param tracker A unique pointer to the Tracker object for storing tracking results.
 * @param currentTime A TimePoint representing the current time, used to generate the file name.
 * @param numChannels The number of channels in the data, used to generate TDOA and XCorr labels.
 * @throws std::runtime_error If the file cannot be opened for writing.
 */
void initializeOutputFiles(std::string &outputFile, std::unique_ptr<Tracker> &tracker,
                           TimePoint &currentTime, const int numChannels)
{
    outputFile = "deployment_files/" + convertTimePointToString(currentTime);

    if (tracker)
    {
        tracker->mOutputfile = outputFile + "_tracker";
    }

    std::cout << "Created and writing to file: " << outputFile << std::endl;

    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Unable to open file for writing: " + outputFile);
    }

    std::vector<std::string> columnNames = {
        "PeakTime",
        "Amplitude",
        "DOA_x",
        "DOA_y",
        "DOA_z"};

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
        if (i < columnNames.size() - 1)
            file << ",";
    }
    file << std::endl;

    file.close();
}

/**
 * @brief Determines if the program should terminate based on the runtime duration.
 *
 * @param runtimeConfig Experiment runtime configuration and state.
 * @param startLoop The time point at the start of the current loop iteration.
 * @return true if the program should terminate, false otherwise.
 */
bool shouldTerminateProgram(const RuntimeConfig &runtimeConfig, const TimePoint &startLoop)
{
    auto elapsedTime = startLoop - runtimeConfig.programStartTime;
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
std::string convertTimePointToString(const std::chrono::system_clock::time_point &timePoint)
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
std::vector<std::string> generateChannelComboLabels(const std::string &labelPrefix, int numChannels)
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
