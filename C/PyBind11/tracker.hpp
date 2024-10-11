#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <vector>
#include <eigen3/Eigen/Dense>
#include <set>
#include <map>
#include "kalman_filter.hpp"
#include "/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/dbscan.hpp"

// Define the label function
std::vector<size_t> label2(const std::vector<std::vector<size_t>> &clusters, size_t n);

// Helper function to convert Eigen vectors to std::vector<point2>
std::vector<point2> Eigen_to_point2_vector(const std::vector<Eigen::VectorXf> &data);

class Tracker
{
public:
    Tracker(double eps = 3, int min_samples = 15, int missed_update_threshold = 4);

    KalmanFilter initialize_kalman_filter(const Eigen::Vector2d &initial_state);

    std::vector<Eigen::Vector2d> run_dbscan(const std::vector<Eigen::VectorXf> &data);

    std::pair<std::vector<int>, std::vector<int>> find_optimal_association(const Eigen::MatrixXd &distance_matrix, double threshold);

    Eigen::MatrixXd calculate_distance_matrix(const std::vector<Eigen::Vector2d> &cluster_centers);

    void destroy_expired_filters();

    std::vector<Eigen::Vector2d> get_cluster_centroids(const std::vector<point2> &data, const std::vector<size_t> &labels);

    void increment_missed_counter(const std::set<int> &associated_filters);

    void initialize_filters_for_clusters(const std::vector<int> &unassigned_clusters, const std::vector<Eigen::Vector2d> &cluster_centers);

    void update_kalman_filters(const std::vector<Eigen::Vector2d> &cluster_centers);

    void update_kalman_filters_continuous(const Eigen::VectorXd& observation, double time);

    void process_batch(const std::vector<Eigen::VectorXf> &df_current);

    std::vector<std::map<std::string, double>> kalman_log;

private:
    double eps;
    int min_samples;
    int missed_update_threshold;
    int global_counter;
    int next_label;

    std::vector<KalmanFilter> kalman_filters;
    std::vector<int> cluster_assignments;
    std::vector<int> missed_updates;
    template <typename T>
    void filter_by_indices(std::vector<T> &vec, const std::vector<int> &indices_to_keep);
};

#endif // TRACKER_HPP