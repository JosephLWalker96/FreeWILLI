#include "kalman_filter.hpp"

KalmanFilter::KalmanFilter(const Eigen::MatrixXd &transition_matrices,
                           const Eigen::MatrixXd &observation_matrices,
                           const Eigen::VectorXd &initial_state_mean,
                           const Eigen::MatrixXd &initial_state_covariance,
                           const Eigen::MatrixXd &transition_covariance,
                           const Eigen::MatrixXd &observation_covariance)
    : F(transition_matrices),
      H(observation_matrices),
      Q(transition_covariance),
      R(observation_covariance),
      x(initial_state_mean),
      P(initial_state_covariance) {}

void KalmanFilter::predict()
{
    x_prior = F * x;
    P_prior = F * P * F.transpose() + Q;
}

void KalmanFilter::update(const Eigen::VectorXd &z)
{
    Eigen::VectorXd y = z - H * x_prior;
    Eigen::MatrixXd S = H * P_prior * H.transpose() + R;
    Eigen::MatrixXd K = P_prior * H.transpose() * S.inverse();
    x = x_prior + K * y;
    P = (Eigen::MatrixXd::Identity(P_prior.rows(), P_prior.cols()) - K * H) * P_prior;
}

// Add getter methods for H and x_prior
const Eigen::MatrixXd &KalmanFilter::getH() const { return H; }
const Eigen::VectorXd &KalmanFilter::getXPrior() const { return x_prior; }
const Eigen::VectorXd &KalmanFilter::getX() const { return x; }

std::pair<Eigen::VectorXd, Eigen::MatrixXd> KalmanFilter::filter_update(const Eigen::VectorXd *filtered_state_mean,
                                                                        const Eigen::MatrixXd *filtered_state_covariance,
                                                                        const Eigen::VectorXd *observation)
{
    if (filtered_state_mean != nullptr)
    {
        x = *filtered_state_mean;
    }
    if (filtered_state_covariance != nullptr)
    {
        P = *filtered_state_covariance;
    }

    predict();

    if (observation != nullptr)
    {
        update(*observation);
    }
    else
    {
        x = x_prior;
        P = P_prior;
    }

    return {x, P};
}