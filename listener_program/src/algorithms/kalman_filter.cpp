#include "kalman_filter.h"

/**
 * @brief Constructs a Kalman Filter with an initial state.
 *
 * @param initialState Initial state vector for the filter.
 */
KalmanFilter::KalmanFilter(const Eigen::Vector3f &initialState)
{
    // State transition matrix (6x6): Describes how the state evolves
    mProcessModel.resize(6, 6);
    mProcessModel << 1, 0, 0, 1, 0, 0, // X position update
        0, 1, 0, 0, 1, 0,              // Y position update
        0, 0, 1, 0, 0, 1,              // Z position update
        0, 0, 0, 1, 0, 0,              // X velocity remains the same
        0, 0, 0, 0, 1, 0,              // Y velocity remains the same
        0, 0, 0, 0, 0, 1;              // Z velocity remains the same

    // Observation model matrix (3x6): We can observe position but not velocity
    mObservationModel.resize(3, 6);
    mObservationModel << 1, 0, 0, 0, 0, 0, // Observing X position
        0, 1, 0, 0, 0, 0,                  // Observing Y position
        0, 0, 1, 0, 0, 0;                  // Observing Z position

    // Initial state vector (6x1): [X, Y, Z, Vx, Vy, Vz]
    mCurrentState.resize(6);
    mCurrentState << initialState(0), initialState(1), initialState(2), 0.0f, 0.0f, 0.0f;

    // Initial state covariance matrix (6x6)
    mStateCovariance.resize(6, 6);
    mStateCovariance.setIdentity();
    mStateCovariance *= 1000.0f;

    // Process noise covariance matrix (6x6)
    mProcessNoiseCov.resize(6, 6);
    mProcessNoiseCov.setIdentity();
    mProcessNoiseCov *= 0.01f;

    // Observation noise covariance matrix (3x3)
    mObservationNoiseCov.resize(3, 3);
    mObservationNoiseCov.setIdentity();
    mObservationNoiseCov *= 10.0f;

    // Initialize predicted state and covariance
    mPredictedState = mCurrentState;
    mPredictedCovariance = mStateCovariance;
}

/**
 * @brief Predicts the next state using the process model.
 */
void KalmanFilter::predict()
{
    // Predict state and covariance
    mPredictedState = mProcessModel * mCurrentState;
    mPredictedCovariance = mProcessModel * mStateCovariance * mProcessModel.transpose() + mProcessNoiseCov;
}

/**
 * @brief Updates the state estimate using an observation.
 *
 * @param observation Vector containing the observed measurements.
 */
void KalmanFilter::update(const Eigen::VectorXf &observation)
{
    // Compute innovation (residual)
    Eigen::VectorXf innovation = observation - mObservationModel * mPredictedState;

    // Compute innovation covariance
    Eigen::MatrixXf innovationCovariance = mObservationModel * mPredictedCovariance * mObservationModel.transpose() + mObservationNoiseCov;

    // Compute Kalman gain
    Eigen::MatrixXf kalmanGain = mPredictedCovariance * mObservationModel.transpose() * innovationCovariance.inverse();

    // Update state and covariance
    mCurrentState = mPredictedState + kalmanGain * innovation;
    mStateCovariance = (Eigen::MatrixXf::Identity(mPredictedCovariance.rows(), mPredictedCovariance.cols()) - kalmanGain * mObservationModel) * mPredictedCovariance;
}

/**
 * @brief Retrieves the observation model matrix.
 *
 * @return Constant reference to the observation model matrix.
 */
const Eigen::MatrixXf &KalmanFilter::getObservationMatrix() const
{
    return mObservationModel;
}

/**
 * @brief Retrieves the predicted state vector prior to the update step.
 *
 * @return Constant reference to the predicted state vector.
 */
const Eigen::VectorXf &KalmanFilter::getPredictedState() const
{
    return mPredictedState;
}

/**
 * @brief Retrieves the current state vector after the update step.
 *
 * @return Constant reference to the current state vector.
 */
const Eigen::VectorXf &KalmanFilter::getCurrentState() const
{
    return mCurrentState;
}

/**
 * @brief Performs a full filter update, including prediction and optional observation updates.
 *
 * @param filteredStateMean Optional pointer to a filtered state mean vector.
 * @param filteredStateCovariance Optional pointer to a filtered state covariance matrix.
 * @param observation Optional pointer to an observation vector.
 * @return Pair containing the updated state vector and covariance matrix.
 */
auto KalmanFilter::filterUpdate(const Eigen::VectorXf *filteredStateMean,
                                const Eigen::MatrixXf *filteredStateCovariance,
                                const Eigen::VectorXf *observation) -> std::pair<Eigen::VectorXf, Eigen::MatrixXf>
{
    // Optional update of state and covariance
    if (filteredStateMean != nullptr)
    {
        mCurrentState = *filteredStateMean;
    }
    if (filteredStateCovariance != nullptr)
    {
        mStateCovariance = *filteredStateCovariance;
    }

    // Predict step
    predict();

    // Update step if observation is available
    if (observation != nullptr)
    {
        update(*observation);
    }
    else
    {
        mCurrentState = mPredictedState;
        mStateCovariance = mPredictedCovariance;
    }

    return {mCurrentState, mStateCovariance};
}