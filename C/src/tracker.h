#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <vector>
#include <eigen3/Eigen/Dense>
#include <set>
#include <map>
#include <chrono>
#include <fstream>
#include "kalman_filter.h"
#include "/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/dbscan.hpp"

using namespace std::chrono_literals;

#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#endif

// Define the label function
std::vector<size_t> label2(const std::vector<std::vector<size_t>> &clusters, size_t n);

// Helper function to convert Eigen vectors to std::vector<point2>
std::vector<point3> Eigen_to_point2_vector(const std::vector<Eigen::VectorXf> &data);

struct LogEntry
{
    unsigned long time;
    int filter_id; // Use 'int' if 'filter_id' is an integer
    double updated_x;
    double updated_y;

    // Optional: Constructor for convenience
    LogEntry(unsigned long t, int id, double x, double y)
        : time(t), filter_id(id), updated_x(x), updated_y(y) {}
};

class Tracker
{
public:
    Tracker(double eps = 3, int min_samples = 15, int missed_update_threshold = 4);

    // KalmanFilter initialize_kalman_filter(const Eigen::Vector3d &initial_state);

    std::vector<Eigen::Vector3d> run_dbscan(const std::vector<Eigen::VectorXf> &data);

    std::pair<std::vector<int>, std::vector<int>> find_optimal_association(const Eigen::MatrixXd &distance_matrix, double threshold);

    Eigen::MatrixXd calculate_distance_matrix(const std::vector<Eigen::Vector3d> &cluster_centers);

    void destroy_expired_filters();

    std::vector<Eigen::Vector3d> get_cluster_centroids(const std::vector<point3> &data, const std::vector<size_t> &labels);

    void increment_missed_counter(const std::set<int> &associated_filters);

    void initialize_filters_for_clusters(const std::vector<int> &unassigned_clusters, const std::vector<Eigen::Vector3d> &cluster_centers);

    void update_kalman_filters(const std::vector<Eigen::Vector3d> &cluster_centers);

    void update_kalman_filters_continuous(const Eigen::VectorXd &observation, unsigned long time);

    void process_batch(const std::vector<Eigen::VectorXf> &df_current);

    void write_log_to_file(const std::string &filename, std::vector<LogEntry> &kalman_log);

    std::vector<LogEntry> kalman_log;

private:
    double eps;
    int min_samples;
    int missed_update_threshold;
    int global_counter;
    int next_label;
    std::string outputfile = "tracker_output.csv";

    std::chrono::milliseconds _flushInterval = 5s;
    size_t _bufferSizeThreshold = 1000; // Adjust as needed
    std::chrono::time_point<std::chrono::steady_clock> _lastFlushTime = std::chrono::steady_clock::now();

    std::vector<KalmanFilter> kalman_filters;
    std::vector<int> cluster_assignments;
    std::vector<int> missed_updates;
    template <typename T>
    void filter_by_indices(std::vector<T> &vec, const std::vector<int> &indices_to_keep);
};

#endif