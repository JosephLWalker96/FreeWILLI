#include <benchmark/benchmark.h>
#include <string>
#include <vector>
#include "../src/TDOA_estimation.h"
using namespace std;

using TimePoint = std::chrono::system_clock::time_point;

static void BM_TDOA_Estimation(benchmark::State &state)
{
    // Call the function under test
    std::vector<float> chanSpacing = {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 1.0f};
    Eigen::VectorXf TDOAs(6);

    for (size_t i = 0; i < chanSpacing.size(); i++)
    {
        TDOAs(i) = chanSpacing[i] * 0.0001f;
    }

    for (auto _ : state)
    {
        Eigen::VectorXf DOAs_est_pos = TDOA_To_DOA_VerticalArray(TDOAs, 1500.0, chanSpacing);
        TDOAs = -1 * TDOAs;
        Eigen::VectorXf DOAs_est_neg = TDOA_To_DOA_VerticalArray(TDOAs, 1500.0, chanSpacing);
    }
}

BENCHMARK(BM_TDOA_Estimation);