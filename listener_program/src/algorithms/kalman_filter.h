#pragma once
#include <eigen3/Eigen/Dense>

/**
 * @class KalmanFilter
 * @brief Implements a Kalman Filter for linear state estimation. This includes
 * prediction, observation updates, and state retrieval for real-time applications.
 */
class KalmanFilter
{
public:
    explicit KalmanFilter(const Eigen::Vector3f &initialState);
    void predict();
    void update(const Eigen::VectorXf &observation);
    auto filterUpdate(
        const Eigen::VectorXf *filteredStateMean = nullptr,
        const Eigen::MatrixXf *filteredStateCovariance = nullptr,
        const Eigen::VectorXf *observation = nullptr) -> std::pair<Eigen::VectorXf, Eigen::MatrixXf>;
    const Eigen::MatrixXf &getObservationMatrix() const;
    const Eigen::VectorXf &getPredictedState() const;
    const Eigen::VectorXf &getCurrentState() const;

private:
    Eigen::MatrixXf mProcessModel;        ///< Process model matrix (state transition matrix).
    Eigen::MatrixXf mObservationModel;    ///< Observation model matrix.
    Eigen::MatrixXf mProcessNoiseCov;     ///< Process noise covariance matrix.
    Eigen::MatrixXf mObservationNoiseCov; ///< Observation noise covariance matrix.
    Eigen::VectorXf mCurrentState;        ///< Current state vector.
    Eigen::MatrixXf mStateCovariance;     ///< State covariance matrix.
    Eigen::VectorXf mPredictedState;      ///< Predicted state vector (prior to update).
    Eigen::MatrixXf mPredictedCovariance; ///< Predicted state covariance matrix (prior to update).
};