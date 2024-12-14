#include "linear_algebra_utils.h"

/**
 * @brief Computes the Singular Value Decomposition (SVD) of a given matrix.
 *
 * @param matrix Input matrix to decompose.
 * @return The SVD decomposition of the matrix.
 */
Eigen::JacobiSVD<Eigen::MatrixXf> computeSvd(const Eigen::MatrixXf &matrix)
{
    return Eigen::JacobiSVD<Eigen::MatrixXf>(matrix, Eigen::ComputeThinU | Eigen::ComputeThinV);
}

/**
 * @brief Precomputes the pseudo-inverse of the singular value matrix multiplied by V.
 *
 * @param svd The SVD decomposition of the matrix.
 * @return The precomputed matrix P = V * Sigma^+.
 */
Eigen::MatrixXf precomputePseudoInverse(const Eigen::JacobiSVD<Eigen::MatrixXf> &svd)
{
    // Extract matrices and singular values from the SVD decomposition
    Eigen::MatrixXf rightSingularVectors = svd.matrixV();
    Eigen::MatrixXf leftSingularVectors = svd.matrixU();
    Eigen::VectorXf singularValues = svd.singularValues();

    // Compute the pseudo-inverse of the singular value matrix (Sigma^+)
    Eigen::MatrixXf sigmaPseudoInverse = Eigen::MatrixXf::Zero(rightSingularVectors.cols(), leftSingularVectors.cols());
    for (int i = 0; i < singularValues.size(); ++i)
    {
        if (singularValues(i) > 1e-10) // Threshold to avoid division by zero
        {
            sigmaPseudoInverse(i, i) = 1.0 / singularValues(i);
        }
    }

    Eigen::MatrixXf precomputedMatrix = rightSingularVectors * sigmaPseudoInverse;
    return precomputedMatrix;
}

/**
 * @brief Computes the numerical rank of a matrix based on its SVD and a tolerance.
 *
 * @param matrix Input matrix whose rank is to be determined.
 * @param tolerance Threshold below which singular values are considered zero.
 * @return The numerical rank of the matrix.
 */
int computeRank(const Eigen::MatrixXf &matrix, double tolerance)
{
    // Perform SVD decomposition
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(matrix, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::VectorXf singularValues = svd.singularValues();

    // Count singular values greater than the tolerance
    int rank = 0;
    for (int i = 0; i < singularValues.size(); ++i)
    {
        if (singularValues(i) > tolerance)
        {
            ++rank;
        }
    }
    return rank;
}