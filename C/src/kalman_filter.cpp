#include "kalman_filter.h"

KalmanFilter::KalmanFilter(const Eigen::Vector3f &initial_state)
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
    x << initial_state(0), initial_state(1), initial_state(2), 0.0f, 0.0f, 0.0f;

    // x_prior = x;

    // Initial state covariance
    // Eigen::MatrixXd P(6, 6);
    P.resize(6, 6);
    P.setIdentity();
    P *= 1000.0f;

    // Process / transition covariance
    // Eigen::MatrixXd Q(6, 6);
    Q.resize(6, 6);
    Q.setIdentity();
    Q *= 0.01f;

    // Observation covariance
    // Eigen::MatrixXd R(3, 3);
    R.resize(3, 3);
    R.setIdentity();
    R *= 10.0f;

    // std::cout << "before Kalman" << std::endl;
    // std::cout << "after Kalman" << std::endl;
    //  return KalmanFilter(transition_matrix, observation_matrix, initial_state_mean, initial_state_covariance, process_covariance, observation_covariance);
}

void KalmanFilter::predict()
{
    x_prior = F * x;
    P_prior = F * P * F.transpose() + Q;
}

void KalmanFilter::update(const Eigen::VectorXf &z)
{
    Eigen::VectorXf y = z - H * x_prior;
    Eigen::MatrixXf S = H * P_prior * H.transpose() + R;
    Eigen::MatrixXf K = P_prior * H.transpose() * S.inverse();
    x = x_prior + K * y;
    P = (Eigen::MatrixXf::Identity(P_prior.rows(), P_prior.cols()) - K * H) * P_prior;
}

// Add getter methods for H and x_prior
const Eigen::MatrixXf &KalmanFilter::getH() const { return H; }
const Eigen::VectorXf &KalmanFilter::getXPrior() const { return x_prior; }
const Eigen::VectorXf &KalmanFilter::getX() const { return x; }

std::pair<Eigen::VectorXf, Eigen::MatrixXf> KalmanFilter::filter_update(const Eigen::VectorXf *filtered_state_mean,
                                                                        const Eigen::MatrixXf *filtered_state_covariance,
                                                                        const Eigen::VectorXf *observation)
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