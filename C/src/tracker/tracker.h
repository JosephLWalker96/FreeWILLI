#pragma once
#include "../pch.h"
#include "../algorithms/kalman_filter.h"
#include "/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/dbscan.hpp"

using namespace std::chrono_literals;
using TimePoint = std::chrono::system_clock::time_point;

#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#endif

struct LogEntry
{
    unsigned long timestamp; ///< Time of the update (in microseconds or desired time units).
    int filterId;            ///< Unique identifier for the filter.
    double updatedX;         ///< Updated x-coordinate of the filter's state.
    double updatedY;         ///< Updated y-coordinate of the filter's state.

    LogEntry(unsigned long t, int id, double x, double y)
        : timestamp(t), filterId(id), updatedX(x), updatedY(y) {}
};

class Tracker
{

public:
    Tracker(double eps = 3, int min_samples = 15,
            int missed_update_threshold = 4, const std::string &outputFile = "",
            std::chrono::seconds clusteringFrequency = 60s, std::chrono::seconds clusteringWindow = 30s);

    int updateKalmanFiltersContinuous(const Eigen::VectorXf &observation, const TimePoint &timepoint);

    void scheduleCluster();

    void updateTrackerBuffer(const Eigen::VectorXf &directionOfArrival);

    bool mIsTrackerInitialized = false;

    std::string mOutputfile;

private:
    auto runDbscan() -> std::vector<Eigen::Vector3f>;

    void destroyExpiredFilters();

    auto getClusterCentroids(const std::vector<point3> &data, const std::vector<size_t> &labels) -> std::vector<Eigen::Vector3f>;

    void incrementMissedCounter(const std::set<int> &associated_filters);

    void initializeFiltersForClusters(const std::vector<int> &unassigned_clusters, const std::vector<Eigen::Vector3f> &cluster_centers);

    void processBuffer();

    void writeLogToFile();

    void updateKalmanFilters(const std::vector<Eigen::Vector3f> &clusterCenters);

    template <typename T>
    void filterByIndices(std::vector<T> &vec, const std::vector<int> &indices_to_keep);

    std::vector<Eigen::VectorXf> mObservationBuffer;
    std::chrono::seconds mClusterFrequency;
    std::chrono::seconds mClusterWindow;
    std::chrono::seconds mNoClusterWindow;
    std::chrono::time_point<std::chrono::steady_clock> mLastClusterTime = std::chrono::steady_clock::now() - mClusterWindow;
    std::chrono::time_point<std::chrono::steady_clock> mLastFlushTime = std::chrono::steady_clock::now();
    std::vector<LogEntry> mKalmanLog;

    double mEps;
    int mMinSamples;
    int mMissedUpdateThreshold;
    int mGlobalCounter;
    int mNextLabel;

    std::chrono::milliseconds mFlushInterval = 5s;
    size_t mBufferSizeThreshold = 1000; // Adjust as needed

    std::vector<KalmanFilter> mKalmanFilters;
    std::vector<int> mClusterAssignments;
    std::vector<int> mMissedUpdates;
};