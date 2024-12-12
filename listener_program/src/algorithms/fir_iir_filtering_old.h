#pragma once

#include "../pch.h"
void performFrequencyDomainFIRFiltering(
    const Eigen::MatrixXf &timeDomainChannelData,
    const Eigen::VectorXcf &frequencyDomainFilter,
    fftwf_plan &forwardFftPlan,
    Eigen::MatrixXcf &filteredFrequencyData);
