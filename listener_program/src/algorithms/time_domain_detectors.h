#pragma once
#include "../pch.h"

class ITimeDomainDetector
{
   public:
    virtual ~ITimeDomainDetector() = default;
    virtual bool detect(const Eigen::VectorXf& timeDomainData) = 0;
    virtual float getLastDetection() const = 0;
};

class ITimeDomainDetectorFactory
{
   public:
    static std::unique_ptr<ITimeDomainDetector> create(const PipelineVariables& pipelineVariables)
    {
        if (pipelineVariables.timeDomainDetector == "PeakAmplitude")
        {
            return std::make_unique<PeakAmplitudeDetector>(pipelineVariables.amplitudeDetectionThreshold);
        } else if (pipelineVariables.timeDomainDetector == "None")
        {
            return std::make_unique<NoTimeDomainDetector>();
        } else
        {
            throw std::invalid_argument("Unknown TimeDomainDetector type: " + pipelineVariables.timeDomainDetector);
        }
    }
};

class PeakAmplitudeDetector : public ITimeDomainDetector
{
   private:
    float detectionThreshold;
    float peakAmplitude;

   public:
    // Constructor to initialize detectionThreshold
    explicit PeakAmplitudeDetector(float threshold);

    // Override the detect method
    bool detect(const Eigen::VectorXf& timeDomainData) override;
    float getLastDetection() const override;
};

class NoTimeDomainDetector : public ITimeDomainDetector
{
   private:
    float peakAmplitude;

   public:
    // Default constructor
    NoTimeDomainDetector() = default;

    // Override the detect method: does nothing and returns false
    bool detect(const Eigen::VectorXf& timeDomainData) override
    {
        int peakIndex = 0;

        // Find the peak amplitude and its index in the data
        peakAmplitude = timeDomainData.maxCoeff(&peakIndex);

        // No detection logic. Let all signals pass.
        return true;
    }

    // Override the getLastDetection method: does nothing and returns 0
    float getLastDetection() const override
    {
        // No detection value
        return peakAmplitude;
    }
};