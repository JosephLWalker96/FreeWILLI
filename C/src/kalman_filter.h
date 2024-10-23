#ifndef KALMAN_FILTER_HPP
#define KALMAN_FILTER_HPP

#include <eigen3/Eigen/Dense>

class KalmanFilter
{
public:
    KalmanFilter(const Eigen::Vector3d &initial_state);

    void predict();
    void update(const Eigen::VectorXd &z);
    std::pair<Eigen::VectorXd, Eigen::MatrixXd> filter_update(const Eigen::VectorXd *filtered_state_mean = nullptr,
                                                              const Eigen::MatrixXd *filtered_state_covariance = nullptr,
                                                              const Eigen::VectorXd *observation = nullptr);

    // Add getter methods for H and x_prior
    const Eigen::MatrixXd &getH() const;
    const Eigen::VectorXd &getXPrior() const;
    const Eigen::VectorXd &getX() const;

private:
    Eigen::MatrixXd F;
    Eigen::MatrixXd H;
    Eigen::MatrixXd Q;
    Eigen::MatrixXd R;
    Eigen::VectorXd x;
    Eigen::MatrixXd P;
    Eigen::VectorXd x_prior;
    Eigen::MatrixXd P_prior;
};

#endif