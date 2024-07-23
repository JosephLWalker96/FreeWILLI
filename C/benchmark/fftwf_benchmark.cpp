#include <benchmark/benchmark.h>
#include <fftw3.h>
#include "../src/utils.h"
#include <eigen3/Eigen/Dense>
#include "../src/TDOA_estimation.h"
#include "../src/custom_types.h"

vector<float> filterWeightsFloat = {
        1.74828306e-18,  5.00045521e-04,  3.26087846e-04, -3.51028225e-04,
        -6.21824599e-04,  3.01953952e-18,  7.72954111e-04,  5.38776400e-04,
        -6.10068855e-04, -1.11984343e-03, -3.92734007e-18,  1.44093584e-03,
        1.00811040e-03, -1.13872261e-03, -2.07602193e-03,  2.34343683e-17,
        2.61441207e-03,  1.80583511e-03, -2.01286756e-03, -3.62103679e-03,
        -3.93142381e-17,  4.44449137e-03,  3.03346478e-03, -3.34367192e-03,
        -5.95336041e-03,  5.04536926e-18,  7.17840475e-03,  4.86350735e-03,
        -5.32739986e-03, -9.43708737e-03, -1.59606293e-17,  1.13061549e-02,
        7.65155827e-03, -8.38513248e-03, -1.48861921e-02,  1.05360099e-16,
        1.80237354e-02,  1.23082365e-02, -1.36526346e-02, -2.46247806e-02,
        -2.13420798e-17,  3.12395004e-02,  2.20646149e-02, -2.55674894e-02,
        -4.88431839e-02,  5.33652866e-17,  7.46162882e-02,  6.18804948e-02,
        -9.32437971e-02, -3.02566854e-01,  6.00220103e-01, -3.02566854e-01,
        -9.32437971e-02,  6.18804948e-02,  7.46162882e-02,  5.33652866e-17,
        -4.88431839e-02, -2.55674894e-02,  2.20646149e-02,  3.12395004e-02,
        -2.13420798e-17, -2.46247806e-02, -1.36526346e-02,  1.23082365e-02,
        1.80237354e-02,  1.05360099e-16, -1.48861921e-02, -8.38513248e-03,
        7.65155827e-03,  1.13061549e-02, -1.59606293e-17, -9.43708737e-03,
        -5.32739986e-03,  4.86350735e-03,  7.17840475e-03,  5.04536926e-18,
        -5.95336041e-03, -3.34367192e-03,  3.03346478e-03,  4.44449137e-03,
        -3.93142381e-17, -3.62103679e-03, -2.01286756e-03,  1.80583511e-03,
        2.61441207e-03,  2.34343683e-17, -2.07602193e-03, -1.13872261e-03,
        1.00811040e-03,  1.44093584e-03, -3.92734007e-18, -1.11984343e-03,
        -6.10068855e-04,  5.38776400e-04,  7.72954111e-04,  3.01953952e-18,
        -6.21824599e-04, -3.51028225e-04,  3.26087846e-04,  5.00045521e-04,
        1.74828306e-18
};

static void BM_FFT(benchmark::State& state) {
        int channelSize = (int)(0.01 * 100000);
        unsigned long paddedLength = filterWeightsFloat.size() + channelSize - 1;

        // randomly generating signal data
        Eigen::MatrixXf data(paddedLength, channelSize);
        std::mt19937 generator(0);
        // this generator will generate 1 ramdom number between -1 and 1 at a time
        std::uniform_real_distribution<float> distribution(-1.0, 1.0);
        for (int i = 0; i < channelSize; i++) {
            for (int j = 0; j < paddedLength; j++) {
                data(j, i) = distribution(generator);
            }
        }

        std::vector<float> paddedFilterWeights(paddedLength, 0.0f);
        std::copy(filterWeightsFloat.begin(),filterWeightsFloat.end(),paddedFilterWeights.begin());

        vector<fftwf_plan> plans;
        plans.push_back(
                fftwf_plan_dft_r2c_1d(
                        paddedLength,
                        paddedFilterWeights.data(),
                        (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * paddedLength),
                        FFTW_ESTIMATE
                        ));
        for (int i = 0; i < channelSize; i++) {
            plans.push_back(
                    fftwf_plan_dft_r2c_1d(
                            paddedLength,
                            data.col(i).data(),
                            (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * paddedLength),
                            FFTW_ESTIMATE
                            ));
        }

        for (auto _ : state) {
            for (int i = 0; i < channelSize+1; i++) {
                fftwf_execute(plans[i]);
            }
        }

}

static void BM_Filter(benchmark::State& state) {
    int channelSize = (int)(0.01 * 100000);
    unsigned long paddedLength = filterWeightsFloat.size() + channelSize - 1;
    unsigned long fftOutputSize = paddedLength / 2 + 1;

    // randomly generating signal data
    Eigen::MatrixXf data(paddedLength, channelSize);
    std::mt19937 generator(0);
    // this generator will generate 1 ramdom number between -1 and 1 at a time
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    for (int i = 0; i < channelSize; i++) {
        for (int j = 0; j < paddedLength; j++) {
            data(j, i) = distribution(generator);
        }
    }

    std::vector<float> paddedFilterWeights(paddedLength, 0.0f);
    std::copy(filterWeightsFloat.begin(),filterWeightsFloat.end(),paddedFilterWeights.begin());

    vector<fftwf_plan> plans;
    Eigen::VectorXcf filterFreq(fftOutputSize);
    plans.push_back(
            fftwf_plan_dft_r2c_1d(
                    paddedLength,
                    paddedFilterWeights.data(),
                    reinterpret_cast<fftwf_complex*>(filterFreq.data()),
                    FFTW_ESTIMATE
            ));

    Eigen::MatrixXcf dataFreq(fftOutputSize, channelSize);
    for (int i = 0; i < channelSize; i++) {
        plans.push_back(
                fftwf_plan_dft_r2c_1d(
                        paddedLength,
                        data.col(i).data(),
                        reinterpret_cast<fftwf_complex *> (dataFreq.col(i).data()),
                        FFTW_ESTIMATE
                ));
    }

    for (int i = 0; i < channelSize+1; i++) {
        fftwf_execute(plans[i]);
    }

    for (auto _ : state) {
        for (int i = 0; i < channelSize; i++) {
            dataFreq.col(i) = dataFreq.col(i).array() * filterFreq.array();
        }
    }
}

static void BM_GCC(benchmark::State& state) {
    auto exp = Experiment();
    int channelSize = (int)(0.01 * 100000);
    unsigned long paddedLength = filterWeightsFloat.size() + channelSize - 1;
    unsigned long fftOutputSize = paddedLength / 2 + 1;

    // randomly generating signal data
    Eigen::MatrixXf data(paddedLength, channelSize);
    std::mt19937 generator(0);
    // this generator will generate 1 ramdom number between -1 and 1 at a time
    std::uniform_real_distribution<float> distribution(-1.0, 1.0);
    for (int i = 0; i < channelSize; i++) {
        for (int j = 0; j < paddedLength; j++) {
            data(j, i) = distribution(generator);
        }
    }

    vector<fftwf_plan> plans;
    std::vector<float> paddedFilterWeights(paddedLength, 0.0f);
    std::copy(filterWeightsFloat.begin(),filterWeightsFloat.end(),paddedFilterWeights.begin());

    Eigen::VectorXcf filterFreq(fftOutputSize);
    auto filterPlan = fftwf_plan_dft_r2c_1d(
                    paddedLength,
                    paddedFilterWeights.data(),
                    reinterpret_cast<fftwf_complex*>(filterFreq.data()),
                    FFTW_ESTIMATE
            );
    fftwf_execute(filterPlan);

    Eigen::MatrixXcf dataFreq(fftOutputSize, channelSize);
    for (int i = 0; i < channelSize; i++) {
        plans.push_back(
                fftwf_plan_dft_r2c_1d(
                        paddedLength,
                        data.col(i).data(),
                        reinterpret_cast<fftwf_complex *> (dataFreq.col(i).data()),
                        FFTW_ESTIMATE
                ));
    }

    for (auto plan : plans) {
        fftwf_execute(plan);
    }

    for (int i = 0; i < channelSize; i++) {
        dataFreq.col(i) = dataFreq.col(i).array() * filterFreq.array();
    }

    fftwf_plan inverseFFT = nullptr;
    const unsigned int sampling_rate = 100000;
    int interp = 1;
    for (auto _ : state) {
        GCC_PHAT_FFTW_E(
                dataFreq,
                exp.inverseFFT,
                exp.interp,
                reinterpret_cast<int &>(paddedLength),
                reinterpret_cast<unsigned int&> (channelSize),
                exp.SAMPLE_RATE
                );
    }

}

BENCHMARK(BM_FFT);
BENCHMARK(BM_Filter);
BENCHMARK(BM_GCC);