#include <benchmark/benchmark.h>
#include <string>
#include "../src/utils.h"
#include "../src/custom_types.h"
#include "../src/process_data.h"

using TimePoint = std::chrono::system_clock::time_point;

static void BM_ConvertData(benchmark::State& state) {
    std::vector<uint8_t> dataBytes = {0x80, 0x04,0x80, 0x0d,0x80, 0x0e}; // Sample data bytes (modify as needed)
    unsigned int DATA_SIZE = dataBytes.size();
    unsigned int HEAD_SIZE = 0;
    std::vector<float> expectedResult = {4.0,13.0,14.0}; // Expected converted value (modify as needed)

    // Create an empty vector to store the converted data
    std::vector<float> dataSegment;

    for (auto _ : state)
        ConvertData(dataSegment, dataBytes, DATA_SIZE, HEAD_SIZE);
}

BENCHMARK(BM_ConvertData);