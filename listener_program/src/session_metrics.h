#pragma once

#include <algorithm>
#include <numeric>

#include "pch.h"

struct SessionMetrics
{
    // Metric 2: time-domain detections (counts every cycle where TD stage returns true)
    int timeDomainDetections = 0;

    // Metric 3: valid DOA estimates produced by DirectionEstimationStage
    int validDoaEstimates = 0;

    // Metrics 4 & 5 shared numerator: DOA observations assigned to an existing track
    int trackedDoaCount = 0;

    // Metric 6: total tracks ever created (incremented in initializeFiltersForClusters)
    int tracksCreated = 0;

    // Metric 7: total tracks terminated (incremented in destroyExpiredFilters)
    int tracksTerminated = 0;

    // Metric 8: peak number of simultaneously active tracks
    int maxActiveTracks = 0;

    // Metric 9: duration in seconds of each terminated track
    std::vector<double> terminatedTrackDurations;

    // Metric 10: time-domain detection counts bucketed by minute index since session start
    std::vector<int> detectionsPerMinute;
    TimePoint sessionStartTime;
    TimePoint lastDataTime;

    void recordTimeDomainDetection(const TimePoint& detectionTime)
    {
        timeDomainDetections++;

        auto elapsedSecs =
            std::chrono::duration_cast<std::chrono::seconds>(detectionTime - sessionStartTime).count();
        int minuteIdx = static_cast<int>(elapsedSecs / 60);
        if (minuteIdx < 0)
        {
            minuteIdx = 0;
        }
        if (minuteIdx >= static_cast<int>(detectionsPerMinute.size()))
        {
            detectionsPerMinute.resize(minuteIdx + 1, 0);
        }
        detectionsPerMinute[minuteIdx]++;
    }

    void report() const
    {
        double durationMins =
            std::chrono::duration_cast<std::chrono::seconds>(lastDataTime - sessionStartTime).count() / 60.0;

        std::cout << "\n========== Session Metrics ==========\n";
        std::cout << std::fixed << std::setprecision(2);

        // 1. Encounter duration
        std::cout << "1.  Encounter duration:              " << durationMins << " min\n";

        // 2. Total time-domain detections
        std::cout << "2.  Time-domain detections:          " << timeDomainDetections << "\n";

        // 3. Total valid DOA estimates
        std::cout << "3.  Valid DOA estimates:             " << validDoaEstimates << "\n";

        // 4. Event-to-track utilization
        if (timeDomainDetections > 0)
        {
            double pct = 100.0 * trackedDoaCount / timeDomainDetections;
            std::cout << "4.  Event-to-track utilization:      " << trackedDoaCount << " / "
                      << timeDomainDetections << " (" << std::setprecision(1) << pct << "%)\n";
        }
        else
        {
            std::cout << "4.  Event-to-track utilization:      N/A (no detections)\n";
        }

        // 5. DOA utilization
        if (validDoaEstimates > 0)
        {
            double pct = 100.0 * trackedDoaCount / validDoaEstimates;
            std::cout << "5.  DOA utilization:                 " << trackedDoaCount << " / "
                      << validDoaEstimates << " (" << std::setprecision(1) << pct << "%)\n";
        }
        else
        {
            std::cout << "5.  DOA utilization:                 N/A (no DOA estimates)\n";
        }

        // 6–8. Track counts
        std::cout << "6.  Unique tracks created:           " << tracksCreated << "\n";
        std::cout << "7.  Tracks terminated:               " << tracksTerminated << "\n";
        std::cout << "8.  Max simultaneous active tracks:  " << maxActiveTracks << "\n";

        // 9. Track duration stats
        if (!terminatedTrackDurations.empty())
        {
            double minD =
                *std::min_element(terminatedTrackDurations.begin(), terminatedTrackDurations.end());
            double maxD =
                *std::max_element(terminatedTrackDurations.begin(), terminatedTrackDurations.end());
            double meanD = std::accumulate(terminatedTrackDurations.begin(),
                                           terminatedTrackDurations.end(), 0.0) /
                           static_cast<double>(terminatedTrackDurations.size());
            std::cout << "9.  Track duration min/mean/max:     " << std::setprecision(1)
                      << minD / 60.0 << " / " << meanD / 60.0 << " / " << maxD / 60.0 << " min\n";
        }
        else
        {
            std::cout << "9.  Track duration:                  N/A (no tracks terminated yet)\n";
        }

        // 10. Detections-per-minute stats
        if (!detectionsPerMinute.empty())
        {
            int minDpm =
                *std::min_element(detectionsPerMinute.begin(), detectionsPerMinute.end());
            int maxDpm =
                *std::max_element(detectionsPerMinute.begin(), detectionsPerMinute.end());
            double meanDpm =
                std::accumulate(detectionsPerMinute.begin(), detectionsPerMinute.end(), 0) /
                static_cast<double>(detectionsPerMinute.size());
            std::cout << "10. Detections/min min/mean/max:     " << minDpm << " / "
                      << std::setprecision(1) << meanDpm << " / " << maxDpm << "\n";
        }
        else
        {
            std::cout << "10. Detections/min:                  N/A\n";
        }

        std::cout << "=====================================\n";
    }
};
