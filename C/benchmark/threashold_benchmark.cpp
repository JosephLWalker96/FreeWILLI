#include <benchmark/benchmark.h>
#include "../src/process_data.h"
#include "../src/custom_types.h"
#include <sys/time.h>
using namespace std;

static void BM_Threshold(benchmark::State& state) {
    Experiment exp;
    exp.ProcessFncPtr = ProcessSegmentInterleaved;
    exp.energyDetThresh = 2500;
    exp.SAMPS_PER_CHANNEL = 124;
    exp.HEAD_SIZE = 12;
    exp.BYTES_PER_SAMP = 2;
    exp.REQUIRED_BYTES = 1004;
    exp.MICRO_INCR = 1240;
    exp.NUM_CHAN = 4;
    exp.DATA_BYTES_PER_CHANNEL = 248;
    exp.NUM_PACKS_DETECT = 8;
    exp.DATA_SIZE = 496;
    exp.NUM_PACKS_DETECT = (int)(exp.TIME_WINDOW * 100000 / exp.SAMPS_PER_CHANNEL);
    exp.DATA_SEGMENT_LENGTH = exp.NUM_PACKS_DETECT * exp.SAMPS_PER_CHANNEL * exp.NUM_CHAN;

    // Get the current date and time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);

    std::vector<TimePoint> dataTimes;

    for (int i=0; i < 8; i++){
        // Extract the year, month, day, hour, minute, second, and microseconds
        std::tm timeStruct{};
        timeStruct.tm_year = now_tm.tm_year + 1900 - 2000;
        timeStruct.tm_mon = now_tm.tm_mon + 1;
        timeStruct.tm_mday = now_tm.tm_mday;
        timeStruct.tm_hour = now_tm.tm_hour;
        timeStruct.tm_min = now_tm.tm_min;
        timeStruct.tm_sec = now_tm.tm_sec;

        // Increment the time by a specified number of microseconds
        int MICRO_INCR = 1550 ; // Example increment
        now += std::chrono::microseconds(MICRO_INCR);
        int64_t microSec = now.time_since_epoch().count() % 1000000;
        std::time_t timeResult = std::mktime(&timeStruct);
        auto currentTime = std::chrono::system_clock::from_time_t(timeResult);
        currentTime += std::chrono::microseconds(microSec);
        dataTimes.push_back(currentTime);
    }

    // Declare time checking variables
    bool previousTimeSet = false;
    auto previousTime = std::chrono::time_point<std::chrono::system_clock>::min();

    // Generate the data bytes
    static Eigen::MatrixXf channelData(1092, 1);
    for (int i = 0; i < 1092; i++){
        channelData(i, 0) = 2500.0;
    }

    for (auto _: state){
        ThresholdDetect(channelData.col(0), dataTimes, exp.energyDetThresh, exp.SAMPLE_RATE);
    }
}

BENCHMARK(BM_Threshold);
