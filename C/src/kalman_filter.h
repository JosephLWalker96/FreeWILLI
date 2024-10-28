#ifndef KALMAN_FILTER_HPP
#define KALMAN_FILTER_HPP

#include <eigen3/Eigen/Dense>

class KalmanFilter
{
public:
    KalmanFilter(const Eigen::Vector3f &initial_state);

    void predict();
    void update(const Eigen::VectorXf &z);
    std::pair<Eigen::VectorXf, Eigen::MatrixXf> filter_update(const Eigen::VectorXf *filtered_state_mean = nullptr,
                                                              const Eigen::MatrixXf *filtered_state_covariance = nullptr,
                                                              const Eigen::VectorXf *observation = nullptr);

    // Add getter methods for H and x_prior
    const Eigen::MatrixXf &getH() const;
    const Eigen::VectorXf &getXPrior() const;
    const Eigen::VectorXf &getX() const;

private:
    Eigen::MatrixXf F;
    Eigen::MatrixXf H;
    Eigen::MatrixXf Q;
    Eigen::MatrixXf R;
    Eigen::VectorXf x;
    Eigen::MatrixXf P;
    Eigen::VectorXf x_prior;
    Eigen::MatrixXf P_prior;
};

#endif