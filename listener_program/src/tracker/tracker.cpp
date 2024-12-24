#include "tracker.h"
#include "tracker_utils.h"
#include "../algorithms/kalman_filter.h"

using TimePoint = std::chrono::system_clock::time_point;

// Tracker class implementation
Tracker::Tracker(double eps, int minSamples,
                 int missedUpdateThreshold, const std::string &outputFile,
                 std::chrono::seconds clusteringFrequency, std::chrono::seconds clusteringWindow)
    : mEps(eps), mMinSamples(minSamples), mMissedUpdateThreshold(missedUpdateThreshold),
      mGlobalCounter(0), mNextLabel(0), mOutputfile(outputFile),
      mClusterFrequency(clusteringFrequency), mClusterWindow(clusteringWindow),
      mNoClusterWindow(mClusterFrequency - mClusterWindow)
{
    std::cout << "Initializing tracker: \n";
    std::cout << "    clustering frequency: " << mClusterFrequency.count() << std::endl;
    std::cout << "    clustering window: " << mClusterWindow.count() << std::endl;
    std::cout << "    no clustering window: " << mNoClusterWindow.count() << std::endl;
}

/**
 * @brief Runs the DBSCAN clustering algorithm on the observation buffer.
 *
 * @return A vector of 3D cluster centroids.
 */
auto Tracker::runDbscan() -> std::vector<Eigen::Vector3f>
{
    mGlobalCounter++; // Increment the global counter to track clustering operations

    // Convert the observation buffer into a 3D point vector for clustering
    auto dataPoints3d = convertEigenToPointVector(mObservationBuffer);

    // Perform DBSCAN clustering
    std::vector<std::vector<size_t>> clusterData = dbscan(dataPoints3d, mEps, mMinSamples);

    // Label the clusters based on DBSCAN results
    std::vector<size_t> labels = labelClusters(clusterData, dataPoints3d.size());

    // Calculate and return cluster centroids
    return getClusterCentroids(dataPoints3d, labels);
}

/**
 * @brief Removes expired filters and their associated data based on missed update thresholds.
 */
void Tracker::destroyExpiredFilters()
{
    // Identify filters that have not exceeded the missed update threshold
    std::vector<int> indicesToKeep;
    for (size_t i = 0; i < mMissedUpdates.size(); ++i)
    {
        if (mMissedUpdates[i] <= mMissedUpdateThreshold)
        {
            indicesToKeep.push_back(i);
        }
    }

    // Retain only the filters and data corresponding to valid indices
    filterByIndices(mKalmanFilters, indicesToKeep);
    filterByIndices(mMissedUpdates, indicesToKeep);
    filterByIndices(mClusterAssignments, indicesToKeep);
}

/**
 * @brief Computes the centroids of clusters based on their labels and data points.
 *
 * @param data Vector of 3D points representing the data.
 * @param labels Vector of cluster labels for each data point.
 * @return A vector of 3D cluster centroids.
 */
auto Tracker::getClusterCentroids(const std::vector<point3> &data, const std::vector<size_t> &labels) -> std::vector<Eigen::Vector3f>
{
    std::vector<Eigen::Vector3f> clusterCenters;

    // Extract unique cluster labels
    std::set<int> uniqueLabels(labels.begin(), labels.end());

    // Compute centroids for each cluster
    for (int label : uniqueLabels)
    {
        if (label != 0)
        { // Ignore noise points (label 0)

            // Collect points belonging to the current cluster
            std::vector<point3> clusterPoints;
            for (size_t i = 0; i < labels.size(); ++i)
            {
                if (labels[i] == label)
                {
                    clusterPoints.push_back(data[i]);
                }
            }

            // Compute the centroid using up to the last N points in the cluster
            int N = 3;
            Eigen::Vector3f clusterCenter = Eigen::Vector3f::Zero();
            for (size_t i = std::max(0, static_cast<int>(clusterPoints.size()) - N); i < clusterPoints.size(); ++i)
            {
                Eigen::Vector3f dataPoint(clusterPoints[i].x, clusterPoints[i].y, clusterPoints[i].z);
                clusterCenter += dataPoint;
            }
            clusterCenter /= std::min(static_cast<int>(clusterPoints.size()), N);

            clusterCenters.push_back(clusterCenter);
        }
    }
    return clusterCenters;
}

/**
 * @brief Increments the missed update counter for filters that were not associated with any clusters.
 *
 * @param associatedFilters Set of indices representing filters that were associated with clusters.
 */
void Tracker::incrementMissedCounter(const std::set<int> &associatedFilters)
{
    for (size_t i = 0; i < mKalmanFilters.size(); ++i)
    {
        // Increment the missed update counter for unassociated filters
        if (associatedFilters.find(i) == associatedFilters.end())
        {
            mMissedUpdates[i]++;
        }
    }
}

/**
 * @brief Initializes new Kalman filters for clusters that were not assigned to existing filters.
 *
 * @param unassignedClusters Vector of cluster indices that were not assigned to any filter.
 * @param clusterCenters Vector of cluster centers corresponding to the clusters.
 */
void Tracker::initializeFiltersForClusters(const std::vector<int> &unassignedClusters, const std::vector<Eigen::Vector3f> &clusterCenters)
{
    for (int cluster : unassignedClusters)
    {
        // Create a new Kalman filter for the unassigned cluster center
        mKalmanFilters.emplace_back(KalmanFilter(clusterCenters[cluster]));

        // Assign a unique label to the new filter and initialize its missed update counter
        mClusterAssignments.push_back(mNextLabel++);
        mMissedUpdates.push_back(0);
    }
}

/**
 * @brief Updates Kalman filters based on the current cluster centers, creating new filters if needed and removing expired ones.
 *
 * @param clusterCenters Vector of current cluster centers.
 */
void Tracker::updateKalmanFilters(const std::vector<Eigen::Vector3f> &clusterCenters)
{
    // Calculate the distance matrix between cluster centers and filter states
    Eigen::MatrixXf distanceMatrix = calculateDistanceMatrix(clusterCenters, mKalmanFilters);

    // Find the optimal association of clusters to filters and identify unassigned clusters
    auto [associations, unassignedClusters] = findOptimalAssociation(distanceMatrix, mEps);

    std::set<int> associatedFilters;
    for (size_t r = 0; r < associations.size(); ++r)
    {
        if (associations[r] >= 0)
        {
            // Mark the filter as associated and reset its missed update counter
            associatedFilters.insert(r);
            mMissedUpdates[r] = 0;
        }
    }

    // Initialize new filters for clusters that were not assigned to any existing filter
    initializeFiltersForClusters(unassignedClusters, clusterCenters);

    // Increment missed counters for unassociated filters
    incrementMissedCounter(associatedFilters);

    // Remove filters that have exceeded the missed update threshold
    destroyExpiredFilters();
}

/**
 * @brief Updates Kalman filters continuously based on a single observation and its timestamp.
 *
 * @param observation Current observation vector.
 * @param timepoint Timestamp associated with the observation.
 */
int Tracker::updateKalmanFiltersContinuous(const Eigen::VectorXf &observation, const TimePoint &timepoint)
{
    int bestMatch = -1;
    float bestDist = 0.14f; // Gating threshold for filter association

    // Convert timepoint to a numerical representation (microseconds since epoch)
    auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(timepoint.time_since_epoch());
    unsigned long timenum = static_cast<unsigned long>(time_since_epoch.count());

    // Find the Kalman filter with the closest prediction to the current observation
    for (size_t idx = 0; idx < mKalmanFilters.size(); ++idx)
    {
        KalmanFilter &kf = mKalmanFilters[idx];
        kf.predict();

        // Compute predicted state in observation space
        Eigen::VectorXf predictedStateMean = kf.getObservationMatrix() * kf.getPredictedState();

        // Calculate the distance (norm) between predicted state and observation
        float distance = (predictedStateMean - observation).norm();

        // Update best match if the distance is within the gating threshold
        if (distance < bestDist)
        {
            bestDist = distance;
            bestMatch = idx;
        }
    }

    // If a valid match is found, update the matched Kalman filter
    if (bestMatch != -1)
    {
        KalmanFilter &kf = mKalmanFilters[bestMatch];
        kf.update(observation);

        // Extract current state and convert to elevation and azimuth
        auto currentState = kf.getCurrentState();
        auto [elevation, azimuth] = convertDoaToElAz(currentState[0], currentState[1], currentState[2]);

        // Log the update
        mKalmanLog.emplace_back(timenum, mClusterAssignments[bestMatch], elevation, azimuth);

        // Check if it's time to flush the log to the file
        auto timeSinceLastWrite = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - mLastFlushTime);
        bool isTimeToWrite = mFlushInterval <= timeSinceLastWrite;

        if (mKalmanLog.size() > mBufferSizeThreshold || isTimeToWrite)
        {
            Tracker::writeLogToFile();
            mLastFlushTime = std::chrono::steady_clock::now();
        }
    }
    return bestMatch;
}

/**
 * @brief Processes the observation buffer by clustering and updating Kalman filters.
 */
void Tracker::processBuffer()
{
    // Run DBSCAN to find cluster centers
    std::vector<Eigen::Vector3f> clusterCenters = Tracker::runDbscan();

    // Update Kalman filters based on the identified clusters
    Tracker::updateKalmanFilters(clusterCenters);
}

/**
 * @brief Filters elements of a vector, keeping only the ones with specified indices.
 *
 * @tparam T Type of the elements in the vector.
 * @param vector The vector to be filtered.
 * @param indicesToKeep Vector of indices specifying which elements to retain.
 */
template <typename T>
void Tracker::filterByIndices(std::vector<T> &vector, const std::vector<int> &indicesToKeep)
{
    // Retain elements specified by indicesToKeep
    std::vector<T> filtered;
    filtered.reserve(indicesToKeep.size());
    std::transform(indicesToKeep.begin(),indicesToKeep.end(),std::back_inserter(filtered),
                 [&vector](int ind){return vector[ind];});

    // Replace the original vector with the filtered one
    vector.swap(filtered);
}

/**
 * @brief Writes the contents of kalman_log to a CSV file and clears the log.
 *
 * This function appends each LogEntry in kalman_log to the specified file in CSV format.
 * It writes the header only if the file is empty or doesn't exist.
 * After successfully writing to the file, it clears the contents of kalman_log.
 *
 * @param filename The path to the output file.
 * @param kalman_log The vector containing LogEntry objects.
 * @throws std::ios_base::failure if the file cannot be opened or writing fails.
 */
void Tracker::writeLogToFile()
{
    // Open the output file in append mode
    std::ofstream ofs(mOutputfile, std::ios_base::app);
    if (!ofs.is_open())
    {
        throw std::ios_base::failure("Failed to open file for writing: " + mOutputfile);
    }

    bool writeHeader = false;

    // Check if the file is empty
    // Option 1: Use filesystem (C++17 and above)
#if __cplusplus >= 201703L
    if (fs::file_size(mOutputfile) == 0)
    {
        writeHeader = true;
    }
#else
    // Option 2: Move the file pointer to the end and check if the position is zero
    ofs.seekp(0, std::ios_base::end);
    if (ofs.tellp() == 0)
    {
        writeHeader = true;
    }
#endif

    // Write CSV header if needed
    if (writeHeader)
    {
        ofs << "time,filter_id,updated_x,updated_y\n";
    }

    // Write each log entry to the file
    for (const auto &entry : mKalmanLog)
    {
        ofs << entry.timestamp << ','
            << entry.filterId << ','
            << entry.updatedX << ','
            << entry.updatedY << '\n';
    }

    // Check if writing was successful
    if (!ofs)
    {
        throw std::ios_base::failure("Failed to write to file: " + mOutputfile);
    }

    // Clear the kalman_log vector
    mKalmanLog.clear();
}

/**
 * @brief Schedules and executes clustering operations for a tracker if the required interval has elapsed.
 *
 * @param tracker Unique pointer to the tracker object.
 * @param observationBuffer Buffer of observations to be processed by the tracker.
 */
void Tracker::scheduleCluster()
{
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastCluster = std::chrono::duration_cast<std::chrono::seconds>(currentTime - mLastClusterTime);

    // Check if it's time to perform batch processing (at the top of the minute)
    bool isTimeToCluster = mClusterFrequency <= timeSinceLastCluster;

    if (isTimeToCluster)
    {
        // Update the last cluster time to the current time
        mLastClusterTime = currentTime;

        // Process the last 30 seconds of collected data
        std::cout << "Cluster size: " << mObservationBuffer.size() << std::endl;
        auto startTime = std::chrono::steady_clock::now();
        processBuffer();
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> clusterDuration = endTime - startTime;
        std::cout << "Cluster time: " << clusterDuration.count() << " seconds" << std::endl;

        mIsTrackerInitialized = true;

        // Clear the observation buffer after processing
        mObservationBuffer.clear();
    }
}

/**
 * @brief Adds observations to the tracker buffer during the last 30 seconds of a clustering interval.
 *
 * @param tracker Unique pointer to the tracker object.
 * @param observationBuffer Buffer to store observations for the tracker.
 * @param directionOfArrival Direction of arrival (DOA) vector to be added to the observation buffer.
 */
void Tracker::updateTrackerBuffer(const Eigen::VectorXf &directionOfArrival)
{
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastCluster = std::chrono::duration_cast<std::chrono::seconds>(currentTime - mLastClusterTime);

    // Determine if we are outside the no clustering window
    bool isWithinLast30Seconds = timeSinceLastCluster >= mNoClusterWindow;

    // Start collecting observations only in the last 30 seconds or if the tracker is not initialized
    if (isWithinLast30Seconds || !mIsTrackerInitialized)
    {
        mObservationBuffer.push_back(directionOfArrival); // Collect data for tracking
    }
}