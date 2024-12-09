#include "doa_utils.h"

/**
 * @brief Normalizes the direction of arrival (DOA) vector based on the specified rank.
 *
 * @param doaVector A reference to the DOA vector to be normalized.
 * @param solutionRank The rank of the solution (1, 2, or 3) determining the normalization strategy:
 *                     - 3: Full 3D normalization.
 *                     - 2: Normalize only the x and y components.
 *                     - 1: No normalization applied (not meaningful for rank 1).
 */
void normalizeDoa(Eigen::VectorXf &doaVector, int solutionRank)
{
    if (solutionRank == 3)
    {
        // Full 3D normalization
        float norm = doaVector.norm();
        if (norm > 1e-7)
        { // Avoid division by zero
            doaVector /= norm;
        }
    }
    else if (solutionRank == 2)
    {
        // Normalize only the x and y components
        double xyNorm = std::sqrt(doaVector(0) * doaVector(0) + doaVector(1) * doaVector(1));
        if (xyNorm > 1e-6)
        { // Avoid division by zero
            doaVector(0) /= xyNorm;
            doaVector(1) /= xyNorm;
        }
    }
}

/**
 * @brief Computes the direction of arrival (DOA) from time difference of arrivals (TDOA) using precomputed matrices.
 *
 * @param precomputedP Precomputed matrix P for efficient computation.
 * @param basisMatrixU Precomputed basis matrix U.
 * @param speedOfSound Speed of sound in the medium (e.g., water or air).
 * @param tdoa Time difference of arrival vector.
 * @param rank Rank of the solution, used for normalization.
 * @return Eigen::VectorXf Normalized direction of arrival (DOA) vector.
 */
auto computeDoaFromTdoa(const Eigen::MatrixXf &precomputedP,
                        const Eigen::MatrixXf &basisMatrixU,
                        const float speedOfSound,
                        const Eigen::VectorXf &tdoa,
                        int rank) -> Eigen::VectorXf
{
    // Scale the TDOA vector by the speed of sound
    Eigen::VectorXf scaledTdoa = tdoa * speedOfSound;

    // Compute the product of U^T and the scaled TDOA vector
    Eigen::VectorXf uTransposeTdoa = basisMatrixU.transpose() * scaledTdoa;

    // Compute the DOA vector using the precomputed matrix P
    Eigen::VectorXf doaVector = precomputedP * uTransposeTdoa;

    // Normalize the DOA vector if the rank is greater than 1
    if (rank > 1)
    {
        normalizeDoa(doaVector, rank);
    }

    return doaVector;
}

/**
 * @brief Converts a direction of arrival (DOA) vector to elevation and azimuth angles.
 *
 * @param doa A 3D unit vector representing the direction of arrival.
 * @return Eigen::VectorXf A 2D vector containing elevation (index 0) and azimuth (index 1) angles in degrees.
 */
auto convertDoaToElAz(const Eigen::VectorXf &doa) -> Eigen::VectorXf
{
    // Calculate elevation angle in degrees
    float elevation = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);

    // Calculate azimuth angle in degrees
    float azimuth = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);

    // Create a result vector containing elevation and azimuth
    Eigen::VectorXf elAzVector(2);
    elAzVector << elevation, azimuth;

    return elAzVector;
}