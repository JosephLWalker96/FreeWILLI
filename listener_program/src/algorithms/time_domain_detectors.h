#pragma once
#include "../pch.h"

class TimeDomainDetector {
public:
    virtual ~TimeDomainDetector() = default;

    // Pure virtual method to be implemented by derived classes
    virtual bool detect(const Eigen::VectorXf &timeDomainData) const = 0;
    virtual float getLastDetection() const = 0;
};

class PeakAmplitudeDetector : public TimeDomainDetector {
private:
    float detectionThreshold;
    float lastDetection;

public:
    // Constructor to initialize detectionThreshold
    explicit PeakAmplitudeDetector(float threshold);

    // Override the detect method
    bool detect(const Eigen::VectorXf &timeDomainData) const override;
    float getLastDetection() const override;
};