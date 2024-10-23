#include "tracker.h"
#include "kalman_filter.h"
#include <iostream>

void PrintInfo(const std::vector<Eigen::Vector3d> &cluster_centers,
               const Eigen::MatrixXd &distance_matrix,
               const std::vector<int> &associations,
               const std::vector<int> &unassigned_clusters)
{
    // Print cluster centers
    std::cout << "Cluster Centers:" << std::endl;
    for (const auto &center : cluster_centers)
    {
        std::cout << center.transpose() << std::endl; // Eigen vectors can be printed using .transpose() for better readability
    }

    // Print distance matrix
    std::cout << "Distance Matrix:" << std::endl;
    std::cout << distance_matrix << std::endl;

    // Print associations
    std::cout << "Associations: ";
    for (const auto &assoc : associations)
    {
        std::cout << assoc << " ";
    }
    std::cout << std::endl;

    // Print unassigned clusters
    std::cout << "Unassigned Clusters: ";
    for (const auto &cluster : unassigned_clusters)
    {
        std::cout << cluster << " ";
    }
    std::cout << std::endl;

    // Print message for initializing new filters
    for (const auto &cluster : unassigned_clusters)
    {
        std::cout << "Initializing new filter for cluster: " << cluster << std::endl;
    }
}

std::pair<float, float> doa_to_ElAz(float x, float y, float z)
{
    // Elevation
    float el = static_cast<float>(180.0 - (std::acos(z) * 180.0 / M_PI));

    // Azimuth
    float az = static_cast<float>(std::atan2(y, x) * 180.0 / M_PI);

    return {el, az};
}

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

// Helper function to convert Eigen vectors to std::vector<point2>
std::vector<point3> Eigen_to_point_vector(const std::vector<Eigen::VectorXf> &data)
{
    std::vector<point3> points;
    for (ssize_t i = 0; i < data.size(); ++i)
    {
        points.push_back(point3{data[i][0], data[i][1], data[i][2]});
    }
    return points;
}

// Tracker class implementation
Tracker::Tracker(double eps, int min_samples, int missed_update_threshold)
    : eps(eps), min_samples(min_samples), missed_update_threshold(missed_update_threshold), global_counter(0), next_label(0) {}

// KalmanFilter Tracker::initialize_kalman_filter(const Eigen::Vector3d &initial_state)

std::vector<Eigen::Vector3d> Tracker::run_dbscan(const std::vector<Eigen::VectorXf> &data)
{
    global_counter++;
    auto data_points = Eigen_to_point_vector(data);

    std::vector<std::vector<size_t>> db_clusters = dbscan(data_points, eps, min_samples);
    std::vector<size_t> labels = label2(db_clusters, data_points.size());
    // std::cout << std::endl;

    return get_cluster_centroids(data_points, labels);
}

std::pair<std::vector<int>, std::vector<int>> Tracker::find_optimal_association(const Eigen::MatrixXd &distance_matrix, double threshold)
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

Eigen::MatrixXd Tracker::calculate_distance_matrix(const std::vector<Eigen::Vector3d> &cluster_centers)
{
    Eigen::MatrixXd distance_matrix(kalman_filters.size(), cluster_centers.size());
    if (!kalman_filters.empty() && !cluster_centers.empty())
    {
        Eigen::MatrixXd predicted_state_means(kalman_filters.size(), 3);
        for (size_t i = 0; i < kalman_filters.size(); ++i)
        {
            // auto &kal = kalman_filters[i];
            // kal.predict();
            predicted_state_means.row(i) = kalman_filters[i].getXPrior().head<3>().transpose();
            // predicted_state_means.row(i) = kal.getXPrior().head<3>().transpose();
        }
        for (size_t i = 0; i < kalman_filters.size(); ++i)
        {
            for (size_t j = 0; j < cluster_centers.size(); ++j)
            {
                distance_matrix(i, j) = (predicted_state_means.row(i).transpose() - cluster_centers[j]).norm();
                // distance_matrix(i, j) = (predicted_state_means.row(i) - cluster_centers[j].transpose()).norm();
            }
        }
    }
    return distance_matrix;
}

void Tracker::destroy_expired_filters()
{
    std::vector<int> indices_to_keep;
    for (size_t i = 0; i < missed_updates.size(); ++i)
    {
        if (missed_updates[i] <= missed_update_threshold)
        {
            indices_to_keep.push_back(i);
        }
    }

    filter_by_indices(kalman_filters, indices_to_keep);
    filter_by_indices(missed_updates, indices_to_keep);
    filter_by_indices(cluster_assignments, indices_to_keep);
}

std::vector<Eigen::Vector3d> Tracker::get_cluster_centroids(const std::vector<point3> &data, const std::vector<size_t> &labels)
{
    std::vector<Eigen::Vector3d> cluster_centers;
    std::set<int> unique_labels(labels.begin(), labels.end());
    for (int label : unique_labels)
    {
        if (label != 0)
        { // Ignore noise points
            std::vector<point3> cluster_points;
            for (size_t i = 0; i < labels.size(); ++i)
            {
                if (labels[i] == label)
                {
                    cluster_points.push_back(data[i]);
                }
            }
            Eigen::Vector3d cluster_center = Eigen::Vector3d::Zero();
            for (size_t i = std::max(0, static_cast<int>(cluster_points.size()) - 3); i < cluster_points.size(); ++i)
            {
                Eigen::Vector3d data_point(cluster_points[i].x, cluster_points[i].y, cluster_points[i].z);
                cluster_center += data_point;
            }
            cluster_center /= std::min(static_cast<int>(cluster_points.size()), 3);

            // Eigen::Vector2d data_pointNew(cluster_points.back().x, cluster_points.back().y);
            cluster_centers.push_back(cluster_center);
        }
    }
    return cluster_centers;
}

void Tracker::increment_missed_counter(const std::set<int> &associated_filters)
{
    for (size_t i = 0; i < kalman_filters.size(); ++i)
    {
        if (associated_filters.find(i) == associated_filters.end())
        {
            missed_updates[i]++;
        }
    }
}

void Tracker::initialize_filters_for_clusters(const std::vector<int> &unassigned_clusters, const std::vector<Eigen::Vector3d> &cluster_centers)
{
    for (int c : unassigned_clusters)
    {
        kalman_filters.emplace_back(KalmanFilter(cluster_centers[c]));

        cluster_assignments.push_back(next_label++);
        missed_updates.push_back(0);
    }
}

void Tracker::update_kalman_filters(const std::vector<Eigen::Vector3d> &cluster_centers)
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

    // PrintInfo(cluster_centers, distance_matrix, associations, unassigned_clusters);
    initialize_filters_for_clusters(unassigned_clusters, cluster_centers);

    increment_missed_counter(associated_filters);
    destroy_expired_filters();
}

// Function to update the Kalman filters based on continuous observations
void Tracker::update_kalman_filters_continuous(const Eigen::VectorXd &observation, unsigned long time)
{
    int best_match = -1;
    double best_dist = 0.14; // Gating threshold

    // Find the Kalman filter with the closest prediction to the current observation
    for (size_t idx = 0; idx < kalman_filters.size(); ++idx)
    {
        KalmanFilter &kf = kalman_filters[idx];
        kf.predict();

        // Predicted state (H * x_prior in Python)
        Eigen::VectorXd predicted_state_mean = kf.getH() * kf.getXPrior();

        // Calculate the distance (norm) between predicted state and observation
        double dist = (predicted_state_mean - observation).norm();

        if (dist < best_dist)
        {
            best_dist = dist;
            best_match = idx;
        }
    }

    // If a valid match is found, update the corresponding Kalman filter

    if (best_match != -1)
    {
        KalmanFilter &kf = kalman_filters[best_match];
        kf.update(observation);

        auto currentState = kf.getX();
        auto [el, az] = doa_to_ElAz(currentState[0], currentState[1], currentState[2]);

        kalman_log.emplace_back(time, cluster_assignments[best_match], el, az);

        // Writting to buffer
        auto timeSinceLastWrite = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - _lastFlushTime);
        bool timeToWrite = _flushInterval <= timeSinceLastWrite;

        if (kalman_log.size() > _bufferSizeThreshold || timeToWrite)
        {
            Tracker::write_log_to_file(outputfile, kalman_log);
            _lastFlushTime = std::chrono::steady_clock::now();
        }
    }
}

// Translated process_batch function
void Tracker::process_batch(const std::vector<Eigen::VectorXf> &df_current)
{
    // std::cout << "Before run_dbsan" << std::endl;
    std::vector<Eigen::Vector3d> cluster_centers = Tracker::run_dbscan(df_current);

    Tracker::update_kalman_filters(cluster_centers);
}

template <typename T>
void Tracker::filter_by_indices(std::vector<T> &vec, const std::vector<int> &indices_to_keep)
{
    std::vector<T> filtered;
    for (int idx : indices_to_keep)
    {
        filtered.push_back(vec[idx]);
    }
    vec.swap(filtered);
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
void Tracker::write_log_to_file(const std::string &filename, std::vector<LogEntry> &kalman_log)
{
    // Open the output file in append mode
    std::ofstream ofs(filename, std::ios_base::app);
    if (!ofs.is_open())
    {
        throw std::ios_base::failure("Failed to open file for writing: " + filename);
    }

    bool write_header = false;

    // Check if the file is empty
    // Option 1: Use filesystem (C++17 and above)
#if __cplusplus >= 201703L
    if (fs::file_size(filename) == 0)
    {
        write_header = true;
    }
#else
    // Option 2: Move the file pointer to the end and check if the position is zero
    ofs.seekp(0, std::ios_base::end);
    if (ofs.tellp() == 0)
    {
        write_header = true;
    }
#endif

    // Write CSV header if needed
    if (write_header)
    {
        ofs << "time,filter_id,updated_x,updated_y\n";
    }

    // Write each log entry to the file
    for (const auto &entry : kalman_log)
    {
        ofs << entry.time << ','
            << entry.filter_id << ','
            << entry.updated_x << ','
            << entry.updated_y << '\n';
    }

    // Check if writing was successful
    if (!ofs)
    {
        throw std::ios_base::failure("Failed to write to file: " + filename);
    }

    // Clear the kalman_log vector
    kalman_log.clear();
}