#include <benchmark/benchmark.h>
#include <string>
#include <vector>

#include "../src/TDOA_estimation.h"

using TimePoint = std::chrono::system_clock::time_point;

static void BM_TDOA_EstimationSmall(benchmark::State& state){
    // Call the function under test
    std::vector<float> chanSpacing = {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 1.0f};
    Eigen::VectorXf TDOAs(6);

    for (int i = 0; i < chanSpacing.size(); i++) {
        TDOAs(i) = chanSpacing[i] * 0.0001f;
    }

    for (auto _: state){
        Eigen::VectorXf DOAs_est_pos = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
        TDOAs = -1 * TDOAs;
        Eigen::VectorXf DOAs_est_neg = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
    }
}

static void BM_TDOA_Estimation(benchmark::State& state){
    // randomly generating tdoa data
    std::mt19937 generator(0);
    // this generator will generate 1 ramdom number between -0.8/1500 and 0.8/1500 at a time
    std::uniform_real_distribution<float> distribution(-0.8/1500, 0.8/1500);

    // Call the function under test
    std::vector<float> chanSpacing = {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 1.0f};
    Eigen::VectorXf TDOAs(499500);

    for (int i = 0; i < TDOAs.size(); ++i) {
        TDOAs(i) = distribution(generator);
    }

    for (auto _: state){
        Eigen::VectorXf DOAs_est_pos = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
        TDOAs = -1 * TDOAs;
        Eigen::VectorXf DOAs_est_neg = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
    }
}

BENCHMARK(BM_TDOA_EstimationSmall);
BENCHMARK(BM_TDOA_Estimation);