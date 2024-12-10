#pragma once
#include "../pch.h"

std::tuple<float, float> estimateTdoaAndPeak(const Eigen::VectorXf &crossCorrelationBuffer,
                                             int bufferLength, int samplingRate);

void calculateNormalizedCrossSpectra(const Eigen::VectorXcf &inputSignal1,
                                     const Eigen::VectorXcf &inputSignal2,
                                     Eigen::VectorXcf &normalizedCrossSpectra);

auto computeGccPhat(const Eigen::MatrixXcf &savedFfts, fftwf_plan &inverseFftPlan,
                    int &paddedLength,
                    const int &numChannels, const int &sampleRate)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>;