#pragma once
#include "../pch.h"

class TimeDomainDetector {
public:
    virtual ~TimeDomainDetector() = default;

    // Pure virtual method to be implemented by derived classes
    virtual bool detect(const Eigen::VectorXf &timeDomainData) = 0;
    virtual float getLastDetection() const = 0;
};

class PeakAmplitudeDetector : public TimeDomainDetector {
private:
    float detectionThreshold;
    float peakAmplitude;

public:
    // Constructor to initialize detectionThreshold
    explicit PeakAmplitudeDetector(float threshold);

    // Override the detect method
    bool detect(const Eigen::VectorXf &timeDomainData) override;
    float getLastDetection() const override;
};