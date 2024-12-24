#include "../pch.h"
#include "tracker_utils.h"
#include "../algorithms/kalman_filter.h"

/**
 * @brief Prints details about cluster centers, distance matrix, associations, and unassigned clusters.
 *
 * This function outputs the cluster centers, the distance matrix, associations of objects with clusters,
 * and any clusters that remain unassigned. Additionally, it logs initialization messages for unassigned clusters.
 *
 * @param clusterCenters A vector of 3D Eigen vectors representing the cluster centers.
 * @param distanceMatrix A matrix representing the pairwise distances between objects and clusters.
 * @param associations A vector indicating associations of objects to clusters.
 * @param unassignedClusters A vector of indices for clusters that are not assigned to any objects.
 */
auto printInfo(const std::vector<Eigen::Vector3f> &clusterCenters,
               const Eigen::MatrixXf &distanceMatrix,
               const std::vector<int> &associations,
               const std::vector<int> &unassignedClusters) -> void
{
    std::cout << "Cluster Centers:" << std::endl;
    for (const auto &center : clusterCenters)
    {
        std::cout << center.transpose() << std::endl;
    }

    std::cout << "Distance Matrix:" << std::endl;
    std::cout << distanceMatrix << std::endl;

    std::cout << "Associations: ";
    for (const auto &assoc : associations)
    {
        std::cout << assoc << " ";
    }
    std::cout << std::endl;

    std::cout << "Unassigned Clusters: ";
    for (const auto &cluster : unassignedClusters)
    {
        std::cout << cluster << " ";
    }
    std::cout << std::endl;

    for (const auto &cluster : unassignedClusters)
    {
        std::cout << "Initializing new filter for cluster: " << cluster << std::endl;
    }
}

/**
 * @brief Converts a 3D direction vector to elevation and azimuth angles.
 *
 * @param x The x-coordinate of the 3D vector.
 * @param y The y-coordinate of the 3D vector.
 * @param z The z-coordinate of the 3D vector.
 * @return A pair of floats representing the elevation and azimuth angles (in degrees).
 */
auto convertDoaToElAz(float x, float y, float z) -> std::pair<float, float>
{
    float elevation = static_cast<float>(180.0 - (std::acos(z) * 180.0 / M_PI));
    float azimuth = static_cast<float>(std::atan2(y, x) * 180.0 / M_PI);
    return {elevation, azimuth};
}

/**
 * @brief Flattens a 2D vector of cluster indices into a 1D label vector.
 *
 * Assigns each point in the input clusters a label corresponding to its cluster (1-indexed).
 *
 * @param clusters A vector of vectors, where each sub-vector contains indices of points belonging to a cluster.
 * @param numPoints The total number of points to label.
 * @return A vector where each index corresponds to a point's cluster label (1-indexed).
 */
auto labelClusters(const std::vector<std::vector<size_t>> &clusters, size_t numPoints) -> std::vector<size_t>
{
    std::vector<size_t> flatLabels(numPoints);
    for (size_t clusterIdx = 0; clusterIdx < clusters.size(); ++clusterIdx)
    {
        for (auto pointIdx : clusters[clusterIdx])
        {
            flatLabels[pointIdx] = clusterIdx + 1; // Assign cluster labels (1-indexed)
        }
    }
    return flatLabels;
}

/**
 * @brief Converts a vector of Eigen vectors to a vector of point3 structs.
 *
 * This function takes a vector of 3D Eigen vectors and converts them to a `std::vector<point3>`.
 *
 * @param eigenData A vector of Eigen vectors containing 3D points.
 * @return A vector of `point3` structs containing the converted points.
 */
auto convertEigenToPointVector(const std::vector<Eigen::VectorXf> &buffer) -> std::vector<point3>
{
    std::vector<point3> pointVector;
    pointVector.reserve(buffer.size());
    
    for (const auto &point : buffer){
        pointVector.push_back(point3{point[0], point[1], point[2]});
    }

   std::transform(buffer.begin(), buffer.end(), std::back_inserter(pointVector),
                [](const auto& point){return point3{point[0], point[1], point[2]}; });



    return pointVector;
}

std::pair<std::vector<int>, std::vector<int>> findOptimalAssociation(const Eigen::MatrixXf &distanceMatrix, double threshold)
{
    int numKalmanFilters = distanceMatrix.rows();
    int numNewClusters = distanceMatrix.cols();

    std::vector<int> unassociatedNewClusters;
    std::vector<int> associatedNewClusters;

    // Step 1: Ignore clusters where all distances are above threshold
    for (int newCluster = 0; newCluster < numNewClusters; ++newCluster)
    {
        if ((distanceMatrix.col(newCluster).array() > threshold).all())
        {
            unassociatedNewClusters.push_back(newCluster);
        }
        else
        {
            associatedNewClusters.push_back(newCluster);
        }
    }

    // Step 2: Assign new clusters to Kalman filters based on smallest valid distance
    std::vector<int> associations(numKalmanFilters, -1);
    std::set<int> assignedNewClusters;

    for (int kfIndex = 0; kfIndex < numKalmanFilters; ++kfIndex)
    {
        double minDistance = std::numeric_limits<double>::infinity();
        int bestNewCluster = -1;
        for (int newCluster : associatedNewClusters)
        {
            if (distanceMatrix(kfIndex, newCluster) < threshold && assignedNewClusters.find(newCluster) == assignedNewClusters.end())
            {
                if (distanceMatrix(kfIndex, newCluster) < minDistance)
                {
                    minDistance = distanceMatrix(kfIndex, newCluster);
                    bestNewCluster = newCluster;
                }
            }
        }
        if (bestNewCluster != -1)
        {
            associations[kfIndex] = bestNewCluster;
            assignedNewClusters.insert(bestNewCluster);
        }
    }

    return {associations, unassociatedNewClusters};
}

Eigen::MatrixXf calculateDistanceMatrix(const std::vector<Eigen::Vector3f> &clusterCenters, const std::vector<KalmanFilter> &kalmanFilters)
{
    Eigen::MatrixXf distanceMatrix(kalmanFilters.size(), clusterCenters.size());
    if (!kalmanFilters.empty() && !clusterCenters.empty())
    {
        Eigen::MatrixXf predictedStateMeans(kalmanFilters.size(), 3);
        for (size_t i = 0; i < kalmanFilters.size(); ++i)
        {
            predictedStateMeans.row(i) = kalmanFilters[i].getPredictedState().head<3>().transpose();
        }
        for (size_t i = 0; i < kalmanFilters.size(); ++i)
        {
            for (size_t j = 0; j < clusterCenters.size(); ++j)
            {
                distanceMatrix(i, j) = (predictedStateMeans.row(i).transpose() - clusterCenters[j]).norm();
            }
        }
    }
    return distanceMatrix;
}