#include "utils.h"

using std::endl;
using std::cerr;
using std::cout;
using TimePoint = std::chrono::system_clock::time_point;

void PrintTimes(const std::span<TimePoint> timestamps) {
    /**
    * @brief Prints the timestamps provided in the input vector.
    *
    * This function prints the timestamps provided in the input vector
    * in the format "YYYY-MM-DD HH:MM:SS:Microseconds".
    *
    * @param timestamps A vector of TimePoint objects representing the timestamps to be printed.
    *                   TimePoint is a type alias for a time point based on std::chrono::system_clock.
    */
    
    for (auto& timestamp : timestamps) {
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
            << microSeconds << endl;
        
        cout << msg.str();
    }
}

int RestartListener(Session& sess){
     /**
     * @brief (Re)starts the udp listener. It closes the existing socket connection and creates a new one.
     * Additionally, it clears the buffer (dataBuffer) and the segment to be processed (dataSegment) 
     * as well as the vector containing the timestamps (dataTimes).
     *
     * @param sess A reference to the Session object representing the listener session.
     */
    
    cout << "restarting listener: \n";
    std::cout << sess.dataBuffer.size() << std::endl;

    if (close(sess.datagramSocket) == -1) {
#ifdef PICO
        std::cout <<  "Failed to close socket" << std::endl;
        return 1;
#else
        throw std::runtime_error("Failed to close socket \n");
#endif
    }


    sess.datagramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sess.datagramSocket == -1) {
#ifdef PICO
        std::cout << "Error creating socket" << std::endl;
        return 1;
#else
        throw std::runtime_error("Error creating socket \n");
#endif
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
#ifdef PICO
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(sess.UDP_PORT);
    // Print server IP address and port
    char server_ip[16];
    ip4addr_ntoa_r(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA]), server_ip, sizeof(server_ip));
    printf("Listening from ip %s at port %d\n", server_ip,  ntohs(serverAddr.sin_port));
#else
    serverAddr.sin_addr.s_addr = inet_addr(sess.UDP_IP.c_str());
    serverAddr.sin_port = htons(sess.UDP_PORT);
#endif

    if (sess.UDP_IP == "192.168.100.220"){
        // REMARK: we must print these 3 lines to have correct comparison above...
        // It's weird but it somehow works
        cout<<"Client Addr: "<<sess.UDP_IP<<endl;
        cout<<"Wake Up Addr: "<<"192.168.100.220"<<endl;
        cout<<(sess.UDP_IP == "192.168.100.220")<<endl;

        cout << "Sending wake up data to IP address to data logger \n";
        const char* m1 = "Open";
        unsigned char m2[96] = {0};
        unsigned char message[100];
        std::memcpy(message, m1,4);
        std::memcpy(message + 4, m2, 96);
        if (sendto(sess.datagramSocket, message, sizeof(message), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
#ifdef PICO
            std::cout << "Error sending wake-up data packet to data logger" << std::endl;
            return 1;
#else
            throw std::runtime_error("Error sending wake-up data packet to data logger \n");
#endif
        }
    }
    else if (bind(sess.datagramSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef PICO
        std::cout << "Error binding socket" << std::endl;
        return 1;
#else
        throw std::runtime_error("Error binding socket \n");
#endif
    }

    ClearQueue(sess.dataBuffer);
    sess.dataSegment.clear();
    sess.dataTimes.clear();

    return 0;
}

bool ProcessFile(Experiment& exp, const std::string& fileName) {
    /**
    * @brief Processes a configuration file and initializes global variables accordingly.
    *
    * @param fileName The name (or path) of the configuration file to process.
    */
    
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        return 1;
    }
    inputFile >> exp.HEAD_SIZE >> exp.MICRO_INCR >> exp.NUM_CHAN >> exp.SAMPS_PER_CHANNEL >> exp.BYTES_PER_SAMP;
    exp.DATA_SIZE = exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN * exp.BYTES_PER_SAMP; 
    
    exp.PACKET_SIZE = exp.HEAD_SIZE + exp.DATA_SIZE;
    exp.REQUIRED_BYTES = exp.DATA_SIZE + exp.HEAD_SIZE;
    exp.DATA_BYTES_PER_CHANNEL = exp.SAMPS_PER_CHANNEL * exp.BYTES_PER_SAMP;
    return 0;
}

int InitiateOutputFile(std::string& outputFile, std::tm& timeStruct, int64_t microSec, std::string& feature){

    outputFile = "deployment_files/"  + std::to_string(timeStruct.tm_year + 1900) + '-' + std::to_string(timeStruct.tm_mon + 1) + '-' + 
                     std::to_string(timeStruct.tm_mday) + '-' + std::to_string(timeStruct.tm_hour) + '-' + std::to_string(timeStruct.tm_min) + '-' +
                     std::to_string(timeStruct.tm_sec) + '-' + std::to_string(microSec) + '_' + feature;
    
    std::stringstream msg; // compose message to dispatch
    msg << "created and writting to file: " << outputFile << endl;
    cout << msg.str();

#ifndef PICO
    // Open the file in write mode and clear its contents if it exists, create a new file otherwise
    std::ofstream file(outputFile, std::ofstream::out | std::ofstream::trunc);
    if (file.is_open()) {
        file << "Timestamp (microseconds)" << std::setw(20) << "Peak Amplitude" << endl;
        file.close();
    }
    else {
        std::stringstream throwMsg; // compose message to dispatch
        throwMsg << "Error: Unable to open file for writing: " << outputFile << endl;
        throw std::runtime_error(throwMsg.str());
    }
#endif
}

std::vector<double> ReadFIRFilterFile(const std::string& fileName) {
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
    if (!inputFile.is_open()) {
#ifdef PICO
        cout << "Error: Unable to open filter file '" << fileName << "." << endl;
        return {};
#else
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Unable to open filter file '" << fileName << "'." << endl;
        throw std::ios_base::failure(msg.str());
#endif
    }
    std::string line;
    std::vector<double> filterValues;
    while (std::getline(inputFile, line)){
        std::vector<double> values;
        std::stringstream stringStream(line);
        std::string token;
        
        while(std::getline(stringStream,token, ',')){
#ifdef PICO
            char* end;
            double value = std::strtod(token.c_str(), &end);
            if (*end == '\0') {
                filterValues.push_back(value);
            } else {
                std::cout << "Invalid numeric value: " << token << std::endl;
            }
#else
            try {
                double value = std::stod(token);
                filterValues.push_back(value);
                //cout << value << " ";
            } catch(const std::invalid_argument& e) {
                std::stringstream errMsg; // compose message to dispatch
                errMsg << "Invalid numeric value: " << token << endl;
                cerr << errMsg.str();
            }
#endif
        }
    }
    //arma::Col<double> filter(filterValues);
    return filterValues;
}

void ClearQueue(std::queue<std::vector<uint8_t>>& fullQueue) {
    /**
    * @brief This function effectively clears the given queue by swapping it with an
    * empty queue, thus removing all its elements.
    *
    * @param q The queue to be cleared. This queue holds vectors of uint8_t.
    */
    
    std::queue<std::vector<uint8_t>> empty;
    std::swap(fullQueue, empty);
}

bool WithProbability(double probability){
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

#include <malloc.h>
void printSysMemory() {
    struct mallinfo mi = mallinfo();
    printf("Total non-mmapped bytes (arena):       %d\n", mi.arena);
    printf("Number of free chunks (ordblks):        %d\n", mi.ordblks);
    printf("Number of free fastbin blocks (smblks): %d\n", mi.smblks);
    printf("Number of mapped regions (hblks):       %d\n", mi.hblks);
    printf("Space allocated in mmapped regions (hblkhd): %d\n", mi.hblkhd);
    printf("Maximum total allocated space (usmblks):    %d\n", mi.usmblks);
    printf("Total allocated space (uordblks):       %d\n", mi.uordblks);
    printf("Total free space (fordblks):            %d\n", mi.fordblks);
    printf("Topmost releasable block (keepcost):    %d\n", mi.keepcost);
}

#ifndef PICO
void WritePulseAmplitudes(std::span<float> clickPeakAmps, std::span<TimePoint> timestamps, const std::string& filename) {
    /**
    * @brief Writes pulse amplitudes and corresponding timestamps to a file.
    *
    * @param clickPeakAmps A reference to a vector of doubles containing pulse amplitudes.
    * @param timestamps A reference to a vector of TimePoint objects representing the timestamps corresponding to the pulse amplitudes.
    * @param filename A string specifying the output file path or name.
    */
    

    //cout << "clickPeakAmps.size(): " << clickPeakAmps.size() << endl;
    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open()) {
        // Check if vectors have the same size
        if (clickPeakAmps.size() != timestamps.size()) {
            cerr << "Error: Click amplitude and timestamp vectors have different sizes. \n";
            return;
        }

        // Write data rows
        for (size_t i = 0; i < clickPeakAmps.size(); ++i) {
            auto time_point = timestamps[i];
            auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
            outfile << time_since_epoch.count() << std::setw(20) << clickPeakAmps[i] << endl;
        }

    } 
    else {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Could not open file " << filename << endl;
        cerr << msg.str();
    }
    outfile.close();
}

void WriteArray(const std::span<Eigen::VectorXf> array, const std::span<TimePoint> timestamps, const std::string& filename) {
    /**
    * @brief Writes pulse amplitudes and corresponding timestamps to a file.
    *
    * @param array A reference to an Eigen vector of doubles containing pulse amplitudes.
    * @param timestamps A reference to a vector of TimePoint objects representing the timestamps corresponding to the pulse amplitudes.
    * @param filename A string specifying the output file path or name.
    */
    
    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open()) {
        // Check if vectors have the same size (uncomment if needed)
        /*
        if (array.size() != timestamps.size()) {
            std::cerr << "Error: Click amplitude and timestamp vectors have different sizes." << std::endl;
            return;
        }
        */
        
        // Write data rows
        
        for (int row = 0; row < array.size(); row++){
            auto time_point = timestamps[row];
            auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch());
            
            outfile << time_since_epoch.count() << std::setw(20);
            for (int i = 0; i < array[row].size(); ++i) {
                outfile << array[row][i] << " ";
            }
            outfile << std::endl;
        }

    } else {
        std::stringstream msg; // compose message to dispatch
        msg << "Error: Could not open file " << filename << std::endl;
        std::cerr << msg.str();
    }
    outfile.close();
}

void WriteDataToCerr(std::span<TimePoint> dataTimes, std::vector<std::vector<uint8_t>> dataBytesSaved){
    std::stringstream msg; // compose message to dispatch
    msg << "Timestmaps of data causing error: \n";
    for (const auto timestamp : dataTimes){
        auto convertedTime = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count(); 
        msg << convertedTime << endl;
    }
    msg << endl;
    
    msg << "Errored bytes of last packets: " << endl;
    for (const auto byteArray : dataBytesSaved){
        msg << endl;
        for (const auto data : byteArray){
            msg << std::setw(2) << std::setfill('0') << static_cast<int>(data);
        }
        msg << endl;
    }
    msg << endl;
    cerr << msg.str();
}
#endif
