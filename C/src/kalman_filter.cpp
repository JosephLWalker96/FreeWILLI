#include "kalman_filter.h"

KalmanFilter::KalmanFilter(const Eigen::Vector3d &initial_state)
{

    // State transition matrix
    // Eigen::MatrixXd F(6, 6);
    F.resize(6, 6);
    F << 1, 0, 0, 1, 0, 0, // X position update
        0, 1, 0, 0, 1, 0,  // Y position update
        0, 0, 1, 0, 0, 1,  // Z position update
        0, 0, 0, 1, 0, 0,  // X velocity remains the same
        0, 0, 0, 0, 1, 0,  // Y velocity remains the same
        0, 0, 0, 0, 0, 1;  // Z velocity remains the same

    // Observation matrix (3x6): We can only observe position, not velocity
    // Eigen::MatrixXd H(3, 6);
    H.resize(3, 6);
    H << 1, 0, 0, 0, 0, 0, // Observing X position
        0, 1, 0, 0, 0, 0,  // Observing Y position
        0, 0, 1, 0, 0, 0;  // Observing Z position

    // Initial state vector (6x1): [X, Y, Z, Vx, Vy, Vz]
    // Eigen::VectorXd x(6);
    x.resize(6);
    x << initial_state(0), initial_state(1), initial_state(2), 0.0, 0.0, 0.0;

    // x_prior = x;

    // Initial state covariance
    // Eigen::MatrixXd P(6, 6);
    P.resize(6, 6);
    P.setIdentity();
    P *= 1000.0;

    // Process / transition covariance
    // Eigen::MatrixXd Q(6, 6);
    Q.resize(6, 6);
    Q.setIdentity();
    Q *= 0.01;

    // Observation covariance
    // Eigen::MatrixXd R(3, 3);
    R.resize(3, 3);
    R.setIdentity();
    R *= 10.0;

    // std::cout << "before Kalman" << std::endl;
    // std::cout << "after Kalman" << std::endl;
    //  return KalmanFilter(transition_matrix, observation_matrix, initial_state_mean, initial_state_covariance, process_covariance, observation_covariance);
}

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