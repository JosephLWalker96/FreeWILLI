#include <benchmark/benchmark.h>
#include <string>
#include "../src/utils.h"
#include "../src/custom_types.h"
#include "../src/process_data.h"

using TimePoint = std::chrono::system_clock::time_point;

static void BM_ConvertDataSmall(benchmark::State& state) {
    std::vector<uint8_t> dataBytes = {0x80, 0x04,0x80, 0x0d,0x80, 0x0e}; // Sample data bytes (modify as needed)
    unsigned int DATA_SIZE = dataBytes.size();
    unsigned int HEAD_SIZE = 0;
    std::vector<float> expectedResult = {4.0,13.0,14.0}; // Expected converted value (modify as needed)

    // Create an empty vector to store the converted data
    std::vector<float> dataSegment;

    for (auto _ : state)
        ConvertData(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE);
}

static void BM_ConvertData(benchmark::State& state) {
    std::vector<uint8_t> dataBytes = std::vector<uint8_t>(496, 0); // Sample data bytes (modify as needed)
    for (unsigned char & i : dataBytes) {
        i = (uint8_t) (rand() % 256);
    }

    unsigned int DATA_SIZE = dataBytes.size();
    unsigned int HEAD_SIZE = 12;

    // Create an empty vector to store the converted data
    std::vector<float> dataSegment;
    for (auto _ : state)
        ConvertData(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE);
}

static void BM_ConvertDataSimulation(benchmark::State& state) {
    // init and randomize
    auto receivedData = std::vector<uint8_t>(3968, 0);
    for (unsigned char & i : receivedData) {
        i = (uint8_t) (rand() % 256);
    }
    unsigned int DATA_SIZE = 496;
    unsigned int HEAD_SIZE = 12;

    for (auto _ : state) {
        vector<float> dataSegment;
        for (unsigned int i = 0; i < receivedData.size(); i += DATA_SIZE) {
            auto dataBytes = std::vector<uint8_t>(receivedData.begin() + i, receivedData.begin() + i + 496);
            ConvertData(
                    dataSegment,
                    dataBytes,
                    reinterpret_cast<unsigned int &>(DATA_SIZE),
                    reinterpret_cast<unsigned int &>(HEAD_SIZE)
            );
        }
    }
}

static void BM_ProcessSegmentInterleaved(benchmark::State& state){
    unsigned int NUM_CHAN = 4;
    unsigned int SAMPS_PER_CHANNEL = 124;
    unsigned int NUM_PACKS_DETECT = 8;
    unsigned int DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN;
    std::vector<float>& dataSegment = *new std::vector<float>(DATA_SEGMENT_LENGTH, 0);
    static Eigen::MatrixXf channelData(1092, 4);

    for (auto _ : state){
        ProcessSegmentInterleaved(dataSegment, channelData, NUM_CHAN);
    }
}

BENCHMARK(BM_ConvertDataSmall);
BENCHMARK(BM_ConvertData);
BENCHMARK(BM_ConvertDataSimulation);
BENCHMARK(BM_ProcessSegmentInterleaved);
