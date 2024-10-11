#include <vector>
#include <eigen3/Eigen/Dense>
#include <algorithm>
#include <set>
#include "kalman_filter.hpp"
#include "/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/dbscan.hpp"
#include <iostream>

// Define the label function
std::vector<size_t> label2(const std::vector<std::vector<size_t>> &clusters, size_t n)
{
    std::vector<size_t> flat_clusters(n);

    for (size_t i = 0; i < clusters.size(); i++)
    {
        for (auto p : clusters[i])
        {
            flat_clusters[p] = i + 1; // Assign cluster labels (1-indexed)
        }
    }

    return flat_clusters;
}
// Helper function to convert NumPy array to std::vector<point2>
std::vector<point2> Eigen_to_point2_vector(const std::vector<Eigen::VectorXf> &data)
{

    std::vector<point2> points;

    for (ssize_t i = 0; i < data.size(); ++i)
    {
        points.push_back(point2{data[i][0], data[i][1]});
    }

    return points;
}

class Tracker
{
public:
    Tracker(double eps = 3, int min_samples = 15, int missed_update_threshold = 4)
        : eps(eps), min_samples(min_samples), missed_update_threshold(missed_update_threshold), global_counter(0), next_label(0) {}

    // Initialize Kalman filter given the initial position
    KalmanFilter initialize_kalman_filter(const Eigen::Vector2d &initial_state)
    {
        Eigen::MatrixXd transition_matrix(4, 4);
        transition_matrix << 1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1;

        Eigen::MatrixXd observation_matrix(2, 4);
        observation_matrix << 1, 0, 0, 0,
            0, 1, 0, 0;

        Eigen::VectorXd initial_state_mean(4);
        initial_state_mean << initial_state(0), initial_state(1), 0.0, 0.0;

        Eigen::MatrixXd initial_state_covariance(4, 4);
        initial_state_covariance.setIdentity();
        initial_state_covariance *= 1000.0;

        Eigen::MatrixXd process_covariance(4, 4);
        process_covariance.setIdentity();
        process_covariance *= 0.01;

        Eigen::MatrixXd observation_covariance(2, 2);
        observation_covariance.setIdentity();
        observation_covariance *= 10.0;

        return KalmanFilter(transition_matrix, observation_matrix, initial_state_mean, initial_state_covariance, process_covariance, observation_covariance);
    }

    // Run DBSCAN
    std::vector<Eigen::Vector2d> run_dbscan(const std::vector<Eigen::VectorXf> &data)
    {
        global_counter++;
        // Convert the numpy array to std::vector<point2>
        auto data_points = Eigen_to_point2_vector(data);

        // std::span<const point2> data_span(data_points);
        std::vector<std::vector<size_t>> db_clusters = dbscan(data_points, eps, min_samples);
        std::vector<size_t> labels = label2(db_clusters, data_points.size());
        // Implement a method for plotting DBSCAN output here if necessary
        // plot_dbscan_output(data_points, labels);

        return get_cluster_centroids(data_points, labels);
    }

    // Find optimal association
    std::pair<std::vector<int>, std::vector<int>> find_optimal_association(const Eigen::MatrixXd &distance_matrix, double threshold)
    {
        int num_kalman_filters = distance_matrix.rows();
        int num_new_clusters = distance_matrix.cols();

        std::vector<int> unassociated_new_clusters;
        std::vector<int> associated_new_clusters;

        // Step 1: Ignore clusters where all distances are above threshold
        for (int new_cluster = 0; new_cluster < num_new_clusters; ++new_cluster)
        {
            if ((distance_matrix.col(new_cluster).array() > threshold).all())
            {
                unassociated_new_clusters.push_back(new_cluster);
            }
            else
            {
                associated_new_clusters.push_back(new_cluster);
            }
        }

        // Step 2: Assign new clusters to Kalman filters based on smallest valid distance
        std::vector<int> associations(num_kalman_filters, -1);
        std::set<int> assigned_new_clusters;

        for (int kf_ind = 0; kf_ind < num_kalman_filters; ++kf_ind)
        {
            double min_distance = std::numeric_limits<double>::infinity();
            int best_new_cluster = -1;
            for (int new_cluster : associated_new_clusters)
            {
                if (distance_matrix(kf_ind, new_cluster) < threshold && assigned_new_clusters.find(new_cluster) == assigned_new_clusters.end())
                {
                    if (distance_matrix(kf_ind, new_cluster) < min_distance)
                    {
                        min_distance = distance_matrix(kf_ind, new_cluster);
                        best_new_cluster = new_cluster;
                    }
                }
            }
            if (best_new_cluster != -1)
            {
                associations[kf_ind] = best_new_cluster;
                assigned_new_clusters.insert(best_new_cluster);
            }
        }

        return {associations, unassociated_new_clusters};
    }

    // Calculate distance matrix between Kalman filter predictions and cluster centers
    Eigen::MatrixXd calculate_distance_matrix(const std::vector<Eigen::Vector2d> &cluster_centers)
    {
        Eigen::MatrixXd distance_matrix(kalman_filters.size(), cluster_centers.size());
        if (!kalman_filters.empty() && !cluster_centers.empty())
        {
            Eigen::MatrixXd predicted_state_means(kalman_filters.size(), 2);
            for (size_t i = 0; i < kalman_filters.size(); ++i)
            {
                predicted_state_means.row(i) = kalman_filters[i].getXPrior().head<2>().transpose();
            }
            for (size_t i = 0; i < kalman_filters.size(); ++i)
            {
                for (size_t j = 0; j < cluster_centers.size(); ++j)
                {
                    distance_matrix(i, j) = (predicted_state_means.row(i).transpose() - cluster_centers[j]).norm();
                }
            }
        }
        return distance_matrix;
    }

    // Destroy expired Kalman filters
    void destroy_expired_filters()
    {
        std::vector<int> indices_to_keep;
        for (size_t i = 0; i < missed_updates.size(); ++i)
        {
            if (missed_updates[i] <= missed_update_threshold)
            {
                indices_to_keep.push_back(i);
            }
        }

        // Keep only filters that haven't missed too many updates
        filter_by_indices(kalman_filters, indices_to_keep);
        filter_by_indices(missed_updates, indices_to_keep);
        filter_by_indices(cluster_assignments, indices_to_keep);
    }

    // Get cluster centroids
    std::vector<Eigen::Vector2d> get_cluster_centroids(const std::vector<point2> &data, const std::vector<size_t> &labels)
    {
        std::vector<Eigen::Vector2d> cluster_centers;
        std::set<int> unique_labels(labels.begin(), labels.end());
        for (int label : unique_labels)
        {
            if (label != -1)
            { // Ignore noise points
                std::vector<point2> cluster_points;
                for (size_t i = 0; i < labels.size(); ++i)
                {
                    if (labels[i] == label)
                    {
                        cluster_points.push_back(data[i]);
                    }
                }
                Eigen::Vector2d cluster_center = Eigen::Vector2d::Zero();
                for (size_t i = std::max(0, static_cast<int>(cluster_points.size()) - 3); i < cluster_points.size(); ++i)
                {
                    Eigen::Vector2d data_point(cluster_points[i].x, cluster_points[i].y);
                    cluster_center += data_point;
                }
                cluster_center /= std::min(static_cast<int>(cluster_points.size()), 3);
                cluster_centers.push_back(cluster_center);
            }
        }
        return cluster_centers;
    }

    // Increment missed updates for unassociated filters
    void increment_missed_counter(const std::set<int> &associated_filters)
    {
        for (size_t i = 0; i < kalman_filters.size(); ++i)
        {
            if (associated_filters.find(i) == associated_filters.end())
            {
                missed_updates[i]++;
            }
        }
    }

    // Initialize filters for unassigned clusters
    void initialize_filters_for_clusters(const std::vector<int> &unassigned_clusters, const std::vector<Eigen::Vector2d> &cluster_centers)
    {
        for (int c : unassigned_clusters)
        {
            KalmanFilter new_kf = initialize_kalman_filter(cluster_centers[c]);
            kalman_filters.push_back(new_kf);
            cluster_assignments.push_back(next_label++);
            missed_updates.push_back(0);
        }
    }

    // Update or initialize Kalman filters based on DBSCAN results
    void update_kalman_filters(const std::vector<Eigen::Vector2d> &cluster_centers)
    {
        Eigen::MatrixXd distance_matrix = calculate_distance_matrix(cluster_centers);

        auto [associations, unassigned_clusters] = find_optimal_association(distance_matrix, eps);

        std::set<int> associated_filters;
        for (size_t r = 0; r < associations.size(); ++r)
        {
            if (associations[r] >= 0)
            {
                associated_filters.insert(r);
                missed_updates[r] = 0;
            }
        }

        initialize_filters_for_clusters(unassigned_clusters, cluster_centers);
        increment_missed_counter(associated_filters);
        destroy_expired_filters();
    }

private:
    double eps;
    int min_samples;
    int missed_update_threshold;
    int global_counter;
    int next_label;

    std::vector<KalmanFilter> kalman_filters;
    std::vector<int> cluster_assignments;
    std::vector<int> missed_updates;

    // Utility function to filter vectors by indices
    template <typename T>
    void filter_by_indices(std::vector<T> &vec, const std::vector<int> &indices_to_keep)
    {
        std::vector<T> filtered;
        for (int idx : indices_to_keep)
        {
            filtered.push_back(vec[idx]);
        }
        vec.swap(filtered);
    }
};
