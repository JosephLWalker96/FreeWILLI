#pragma once
#include "../pch.h"

auto GCC_PHAT(Eigen::MatrixXcf &savedFFTs, fftwf_plan &inverseFFT,
              const int interp, int &paddedLength,
              const int &NUM_CHAN, const int &SAMPLE_RATE)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>;

std::tuple<float, float> estimateTdoaAndPeak(const Eigen::VectorXf &crossCorrelationBuffer,
                                             int bufferLength, int samplingRate);

Eigen::VectorXcf calculateNormalizedCrossSpectra(const Eigen::VectorXcf &inputSignal1,
                                                 const Eigen::VectorXcf &inputSignal2);

auto computeGccPhat(Eigen::MatrixXcf &savedFfts, fftwf_plan &inverseFftPlan,
                    int &paddedLength,
                    const int &numChannels, const int &sampleRate)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>;