#include "utils.h"
#include "process_data.h"
#include "socket_manager.h"
#include "custom_types.h"
#include "tracker.h"
#include "buffer_writer.h"

using TimePoint = std::chrono::system_clock::time_point;

void PrintMode()
{
#ifdef DEBUG
    std::cout << "Running Debug Mode" << std::endl;
#else
    std::cout << "Running Release Mode" << std::endl;
#endif
}
std::string TimePointToString(const TimePoint &timePoint)
{
    // Convert time_point to time_t to get calendar time
    std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
    std::tm *tm = std::gmtime(&timeT); // Convert to UTC (or use std::localtime for local time)

    // Format the calendar time (year, month, day, hour, minute, second)
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");

    // Extract the microseconds from the time_point
    auto duration = timePoint.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration) % 1000000;

    // Add the microseconds to the formatted time string
    oss << "." << std::setw(6) << std::setfill('0') << microseconds.count(); // Zero-pad to 6 digits

    return oss.str();
}

void ParseJSONConfig(SocketManager &sess, ExperimentRuntime &expRuntime, char *argv[])
{

    std::string filepath = argv[1];
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open JSON file: " + filepath);
    }

    nlohmann::json jsonConfig;
    file >> jsonConfig;

    sess.UDP_IP = jsonConfig.at("IPAddress").get<std::string>();

    if (sess.UDP_IP == "self")
    {
        sess.UDP_IP = "127.0.0.1";
    }

    sess.UDP_PORT = jsonConfig.at("Port").get<int>();
    expRuntime.speedOfSound = jsonConfig.at("SpeedOfSound").get<float>();
    expRuntime.energyDetThresh = jsonConfig.at("EnergyDetectionThreshold").get<float>();
    expRuntime.ampDetThresh = jsonConfig.at("AmplitudeDetectionThreshold").get<float>();
    expRuntime.programRuntime = std::chrono::seconds(std::stoi(argv[2]));
    expRuntime.filterWeights = jsonConfig.at("FilterWeights").get<std::string>();
    expRuntime.receiverPositions = jsonConfig.at("ReceiverPositions").get<std::string>();
    expRuntime.onnxModelPath = jsonConfig.at("ONNX_model_path").get<std::string>();
    expRuntime.onnxModelScaling = jsonConfig.at("ONNX_model_normalization").get<std::string>();

    expRuntime.trackerUse = jsonConfig.at("Enable_Tracking").get<bool>();
}

void PrintTimes(const std::span<TimePoint> timestamps)
{
    /**
     * @brief Prints the timestamps provided in the input vector.
     *
     * This function prints the timestamps provided in the input vector
     * in the format "YYYY-MM-DD HH:MM:SS:Microseconds".
     *
     * @param timestamps A vector of TimePoint objects representing the timestamps to be printed.
     *                   TimePoint is a type alias for a time point based on std::chrono::system_clock.
     */

    for (auto &timestamp : timestamps)
    {
        std::time_t timeRepresentation = std::chrono::system_clock::to_time_t(timestamp);
        std::tm timeData = *std::localtime(&timeRepresentation);
        auto microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count() % 1000000;
        std::stringstream msg; // compose message to dispatch
        msg << "Timestamp: "
            << timeData.tm_year + 1900 << '-'
            << timeData.tm_mon + 1 << '-'
            << timeData.tm_mday << ' '
            << timeData.tm_hour << ':'
            << timeData.tm_min << ':'
            << timeData.tm_sec << ':'
            << microSeconds << std::endl;

        std::cout << msg.str();
    }
}
/*
void InitiateOutputFile(std::string &outputFile, TimePoint& currentTime, const int NUM_CHAN)
{

    outputFile = "deployment_files/" + TimePointToString(currentTime);

    std::stringstream msg; // compose message to dispatch
    msg << "created and writting to file: " << outputFile << std::endl;
    std::cout << msg.str();

    // Open the file in write mode and clear its contents if it exists, create a new file otherwise
    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::trunc);
    if (file.is_open())
    {
        file << "usec since Unix Start" << std::setw(20) << "Energy     " << "DOA_x    " << "DOA_y   " << "DOA_z   ";

        for (int sig = 1; sig < NUM_CHAN; ++sig)
        {
            for (int ref = sig + 1; ref < NUM_CHAN + 1; ++ref)
            {
                file << "TDOA" + std::to_string(sig * 10 + ref) << "  ";
            }
        }

        for (int sig = 1; sig < NUM_CHAN; ++sig)
        {
            for (int ref = sig + 1; ref < NUM_CHAN + 1; ++ref)
            {
                file << "Xcorr" + std::to_string(sig * 10 + ref) << "   ";
            }
        }

        file << std::endl;
        file.close();
    }
    else
    {
        std::stringstream throwMsg; // compose message to dispatch
        throwMsg << "Error: Unable to open file for writing: " << outputFile << std::endl;
        throw std::runtime_error(throwMsg.str());
    }
}
*/
auto ReadFIRFilterFile(const std::string &fileName) -> std::vector<float>
{
    /**
     * @brief Reads a file containing the FIR filter taps and returns the values as an Armadillo column vector.
     *
     * @param fileName The name (or path) of the file to read.
     *                 The file should contain comma-separated numeric values on each line.
     * @return arma::Col<double> An Armadillo column vector containing the numeric values
     *                           read from the file.
     * @throws std::runtime_error If the file cannot be opened.
     */

    std::ifstream inputFile(fileName);
    if (!inputFile.is_open())
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Unable to open filter file '" << fileName << "'." << std::endl;
        throw std::ios_base::failure(msg.str());
    }
    std::string line;
    std::vector<float> filterValues;
    while (std::getline(inputFile, line))
    {
        // std::vector<float> values;
        std::stringstream stringStream(line);
        std::string token;

        while (std::getline(stringStream, token, ','))
        {
            try
            {
                float value = std::stof(token);
                filterValues.push_back(value);
            }
            catch (const std::invalid_argument &e)
            {
                std::stringstream errMsg; // compose message to dispatch
                errMsg << "Invalid numeric value: " << token << std::endl;
                std::cerr << errMsg.str();
            }
        }
    }
    return filterValues;
}

void ClearQueue(std::queue<std::vector<uint8_t>> &fullQueue)
{
    /**
     * @brief This function effectively clears the given queue by swapping it with an
     * empty queue, thus removing all its elements.
     *
     * @param q The queue to be cleared. This queue holds vectors of uint8_t.
     */

    std::queue<std::vector<uint8_t>> empty;
    std::swap(fullQueue, empty);
}

bool WithProbability(double probability)
{
    /**
     * @brief Generates a boolean value based on the given probability.
     * This fucntion is used for testing.
     *
     * @param probability The probability (between 0.0 and 1.0) of returning true.
     * @return bool Returns true with the specified probability, otherwise returns false.
     */

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double randomValue = dis(gen);
    return randomValue < probability;
}

Eigen::MatrixXf LoadHydrophonePositions(const std::string &filename)
{
    /**
     * @brief Loads hydrophone positions from a CSV file and calculates relative positions.
     *
     * @param filename The name (or path) of the CSV file containing the hydrophone positions.
     * @return Eigen::MatrixXd A matrix containing the relative positions between hydrophones.
     *
     * @throws std::ios_base::failure if the file cannot be opened.
     */
    std::ifstream inputFile(filename);

    if (!inputFile.is_open())
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Unable to open filter file '" << filename << "'." << std::endl;
        throw std::ios_base::failure(msg.str());
    }

    std::cout << "Reading hydrophone positions..." << std::endl;

    std::vector<std::vector<float>> temp_positions;
    std::string line;
    std::string token;

    // First, read the data into a temporary vector of vectors to determine dimensions
    int numRows = 0;
    int numCols = 0;
    while (std::getline(inputFile, line))
    {
        std::stringstream ss(line);
        std::vector<float> row_data;
        while (std::getline(ss, token, ','))
        {
            row_data.push_back(std::stod(token)); // Convert token to double
        }
        if (numCols == 0)
        {
            numCols = row_data.size(); // Set number of columns based on the first row
        }
        temp_positions.push_back(row_data);
        numRows++;
    }

    // Now create the Eigen matrix with determined dimensions
    Eigen::MatrixXf positions(numRows, numCols);

    // Populate the Eigen matrix with the data from the temp_positions vector
    for (int i = 0; i < numRows; ++i)
    {
        for (int j = 0; j < numCols; ++j)
        {
            positions(i, j) = temp_positions[i][j];
        }
    }

    // Print positions for debugging
    // std::cout << "Printing positions: " << std::endl;
    // std::cout << positions << std::endl;

    // Initialize relativePositions with the correct size
    int numRelativePositions = numRows * (numRows - 1) / 2;
    Eigen::MatrixXf relativePositions(numRelativePositions, numCols);

    int index = 0;
    for (int i = 0; i < numRows; i++)
    {
        for (int j = i + 1; j < numRows; j++)
        {
            for (int k = 0; k < numCols; k++)
            {
                relativePositions(index, k) = positions(j, k) - positions(i, k);
            }
            index++;
        }
    }

    /*std::cout << "Relative positions" << std::endl;
    for (int i = 0; i < relativePositions.rows(); i++)
    {
        for (int j = 0; j < relativePositions.cols(); j++)
        {
            std::cout << relativePositions(i,j) << " ";
        }
        std::cout << std::endl;
    }
    */

    return relativePositions;
}

void ShouldFlushBuffer(ObservationBuffer &observationBuffer, ExperimentRuntime &expRuntime, const std::chrono::system_clock::time_point &startLoop)
{
    int currentBufferSize = observationBuffer.Buffer.peakTimes.size();

    if (currentBufferSize == 0)
    {
        return;
    }

    auto elapsedTime = startLoop - expRuntime.programStartTime;
    auto timeSinceLastWrite = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - observationBuffer._lastFlushTime);

    bool timeToWrite = observationBuffer._flushInterval <= timeSinceLastWrite;
    bool timeToExit = elapsedTime >= expRuntime.programRuntime;
    bool bufferIsFull = currentBufferSize >= observationBuffer._bufferSizeThreshold;

    if (bufferIsFull || timeToExit || timeToWrite)
    {
        std::cout << "Flushing buffers of length: " << currentBufferSize << std::endl;
        observationBuffer.write(expRuntime.detectionOutputFile);
        if (timeToExit)
        {
            std::cout << "Terminating program from inside DataProcessor... duration reached \n";
            std::exit(0);
        }
    }
}

void WritePulseAmplitudes(std::span<float> clickPeakAmps, std::span<TimePoint> timestamps, const std::string &filename)
{
    /**
     * @brief Writes pulse amplitudes and corresponding timestamps to a file.
     *
     * @param clickPeakAmps A reference to a vector of doubles containing pulse amplitudes.
     * @param timestamps A reference to a vector of TimePoint objects representing the timestamps corresponding to the pulse amplitudes.
     * @param filename A string specifying the output file path or name.
     */

    // std::cout << "clickPeakAmps.size(): " << clickPeakAmps.size() << std::endl;
    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open())
    {
        // Check if vectors have the same size
        if (clickPeakAmps.size() != timestamps.size())
        {
            std::cerr << "Error: Click amplitude and timestamp vectors have different sizes. \n";
            return;
        }

        // Write data rows
        for (size_t i = 0; i < clickPeakAmps.size(); ++i)
        {
            auto time_point = timestamps[i];
            auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
            outfile << time_since_epoch.count() << std::setw(20) << clickPeakAmps[i] << std::endl;
        }
    }
    else
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Could not open file " << filename << std::endl;
        std::cerr << msg.str();
    }
    outfile.close();
}

void PrintFirstFiveValues(const Eigen::MatrixXf &savedFFTs, const Eigen::MatrixXf &invFFT)
{
    int numRows = 5; // savedFFTs.rows();
    int numCols = 4; // std::min(5, static_cast<int>(savedFFTs.cols()));  // Ensure we only access up to 5 elements

    std::cout << "First 5 values from each row of savedFFTs:" << std::endl;
    for (int i = 0; i < numRows; ++i)
    {
        std::cout << "Row " << i + 1 << ": ";
        for (int j = 0; j < numCols; ++j)
        {
            std::cout << savedFFTs(i, j) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\nFirst 5 values from each row of invFFT:" << std::endl;
    for (int i = 0; i < numRows; ++i)
    {
        std::cout << "Row " << i + 1 << ": ";
        for (int j = 0; j < numCols; ++j)
        {
            std::cout << invFFT(i, j) << " ";
        }
        std::cout << std::endl;
    }
}

void WriteArray(const std::span<Eigen::VectorXf> array, const std::span<TimePoint> timestamps, const std::string &filename)
{
    /**
     * @brief Writes pulse amplitudes and corresponding timestamps to a file.
     *
     * @param array A reference to an Eigen vector of doubles containing pulse amplitudes.
     * @param timestamps A reference to a vector of TimePoint objects representing the timestamps corresponding to the pulse amplitudes.
     * @param filename A string specifying the output file path or name.
     */

    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open())
    {
        // Check if vectors have the same size (uncomment if needed)
        /*
        if (array.size() != timestamps.size()) {
            std::cerr << "Error: Click amplitude and timestamp vectors have different sizes." << std::endl;
            return;
        }
        */

        // Write data rows

        for (size_t row = 0; row < array.size(); row++)
        {
            auto time_point = timestamps[row];
            auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());

            outfile << time_since_epoch.count() << std::setw(20);
            for (int i = 0; i < array[row].size(); ++i)
            {
                outfile << std::fixed << std::setprecision(5) << std::right << array[row][i] << " ";
            }
            outfile << std::endl;
        }
    }
    else
    {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Could not open file " << filename << std::endl;
        std::cerr << msg.str();
    }
    outfile.close();
}

auto GetExampleClick() -> std::vector<float>
{
    std::vector<float> input_tensor_values = {
        92.1948, 96.1963, 101.122, 104.773, 107.151, 108.565, 109.293, 109.541, 109.451, 109.168,
        108.921, 108.999, 109.475, 110.056, 110.395, 110.349, 109.977, 109.466, 109.018, 108.68,
        108.28, 107.571, 106.516, 105.596, 105.651, 106.584, 107.503, 108.037, 108.182, 107.898,
        107.157, 105.957, 104.561, 103.774, 104.144, 104.945, 105.38, 105.203, 104.43, 103.139,
        101.439, 99.5627, 97.9943, 97.3363, 97.4403, 97.6794, 97.7522, 97.6871, 97.5253, 97.2252,
        96.8271, 96.475, 96.1248, 95.6222, 94.9328, 94.1597, 93.6609, 93.7281, 94.0786, 94.1562,
        93.6164, 92.3879, 90.4895, 88.4291, 87.6331, 88.309, 88.8974, 88.7232, 88.0315, 87.4041,
        86.8748, 85.9485, 84.4688, 82.9816, 81.7751, 79.9937, 78.3869, 79.4843, 80.6382, 80.135,
        78.6906, 78.3676, 80.49, 83.8795, 86.785, 88.5948, 89.3266, 89.1901, 88.2955, 86.7553,
        84.8847, 83.6774, 84.5187, 86.541, 88.1887, 89.0825, 89.3545, 89.3343, 89.3493, 89.5171,
        89.751, 89.7528, 89.2266, 88.0493, 86.4237, 85.5187, 86.7017, 88.6813, 90.1444, 90.944,
        91.2794, 91.3301, 91.2617, 91.0691, 90.6381, 89.8405, 88.4528, 86.3709, 84.3802, 84.9515,
        87.2957, 89.235, 90.4691, 91.233, 91.6813, 91.9263, 92.2548, 92.8246, 93.2115, 92.905,
        91.6683, 89.3315, 85.8135, 81.2775, 76.8763, 77.3899, 81.2361, 84.1679, 85.5111, 85.1557,
        82.9623, 81.2194, 84.8278, 88.5298, 90.6789, 91.8252, 92.4424, 92.7654, 92.9029, 92.9495,
        92.9146, 92.6751, 92.1435, 91.2272, 90.0295, 89.3202, 89.9183, 91.1307, 91.9888, 92.2035,
        91.9298, 91.4551, 91.174, 91.2775, 91.702, 92.2776, 92.9613, 93.6848, 94.2458, 94.4455,
        94.2079, 93.6266, 92.9577, 92.4459, 92.1938, 92.2134, 92.6957, 93.7765, 95.1637, 96.4559,
        97.3677};
    return input_tensor_values;
}

void WriteDataToCerr(std::span<TimePoint> dataTimes, std::vector<std::vector<uint8_t>> dataBytesSaved)
{
    std::stringstream msg; // compose message to dispatch
    msg << "Timestmaps of data causing error: \n";
    for (const auto timestamp : dataTimes)
    {
        auto convertedTime = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
        msg << convertedTime << std::endl;
    }
    msg << std::endl;

    msg << "Errored bytes of last packets: " << std::endl;
    for (const auto &byteArray : dataBytesSaved)
    {
        msg << std::endl;
        for (const auto data : byteArray)
        {
            msg << std::setw(2) << std::setfill('0') << static_cast<int>(data);
        }
        msg << std::endl;
    }
    msg << std::endl;
    std::cerr << msg.str();
}

// Function to generate labels with a given prefix
std::vector<std::string> GenerateLabels(const std::string &prefix, int NUM_CHAN)
{
    std::vector<std::string> labels;
    for (int sig = 1; sig < NUM_CHAN; ++sig)
    {
        for (int ref = sig + 1; ref <= NUM_CHAN; ++ref)
        {
            labels.push_back(prefix + std::to_string(sig * 10 + ref));
        }
    }
    return labels;
}

int GetNumberOfChannelPairs(int NUM_CHAN)
{
    return NUM_CHAN * (NUM_CHAN - 1) / 2;
}

void InitiateOutputFiles(std::string &outputFile, std::unique_ptr<Tracker> &tracker, TimePoint &currentTime, const int NUM_CHAN)
{
    outputFile = "deployment_files/" + TimePointToString(currentTime);

    if (tracker)
    {
        tracker->outputfile = outputFile + "_tracker";
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
    std::vector<std::string> tdoaLabels = GenerateLabels("TDOA", NUM_CHAN);
    std::vector<std::string> xcorrLabels = GenerateLabels("XCorr", NUM_CHAN);

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

void AppendBufferToFile(const std::string &outputFile, const BufferStruct &buffer)
{
    // Open the file in append mode
    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::app);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Unable to open file for appending: " + outputFile);
    }

    // Verify that all vectors have the same size
    size_t dataSize = buffer.amps.size();
    if (buffer.DOA_x.size() != dataSize ||
        buffer.DOA_y.size() != dataSize ||
        buffer.DOA_z.size() != dataSize ||
        buffer.tdoaVector.size() != dataSize ||
        buffer.XCorrAmps.size() != dataSize ||
        buffer.peakTimes.size() != dataSize)
    {
        throw std::runtime_error("Error: Mismatched buffer sizes in BufferStruct.");
    }

    int numChannelPairs = buffer.tdoaVector[0].size();

    // For each data row
    for (size_t i = 0; i < dataSize; ++i)
    {
        std::vector<std::string> rowData;

        // Add peak time
        auto time_point = buffer.peakTimes[i];
        auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
        rowData.emplace_back(std::to_string(time_since_epoch.count()));
        // rowData.push_back(TimePointToString(buffer.peakTimes[i]));

        // Add amplitude
        rowData.push_back(std::to_string(buffer.amps[i]));

        // Add DOA values
        rowData.push_back(std::to_string(buffer.DOA_x[i]));
        rowData.push_back(std::to_string(buffer.DOA_y[i]));
        rowData.push_back(std::to_string(buffer.DOA_z[i]));

        // Add TDOA values
        Eigen::VectorXf tdoaVec = buffer.tdoaVector[i];
        // std::cout << "tdoaVec: " << tdoaVec.size() << " numChannelPairs: " << numChannelPairs <<std::endl;
        if (tdoaVec.size() != numChannelPairs)
        {
            throw std::runtime_error("Error: Inconsistent TDOA vector size at index " + std::to_string(i));
        }
        for (int j = 0; j < tdoaVec.size(); ++j)
        {
            rowData.push_back(std::to_string(tdoaVec[j]));
        }

        // Add XCorr values
        Eigen::VectorXf xcorrVec = buffer.XCorrAmps[i];
        if (xcorrVec.size() != numChannelPairs)
        {
            throw std::runtime_error("Error: Inconsistent XCorr vector size at index " + std::to_string(i));
        }
        for (int j = 0; j < xcorrVec.size(); ++j)
        {
            rowData.push_back(std::to_string(xcorrVec[j]));
        }

        // Write row data to file
        for (size_t k = 0; k < rowData.size(); ++k)
        {
            file << rowData[k];
            if (k < rowData.size() - 1)
                file << ",";
        }
        file << std::endl;
    }

    file.close();
}
