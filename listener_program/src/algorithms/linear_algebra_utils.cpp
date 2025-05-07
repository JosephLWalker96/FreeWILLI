#include "linear_algebra_utils.h"

/**
 * @brief Computes the Singular Value Decomposition (SVD) of a given matrix.
 *
 * @param matrix Input matrix to decompose.
 * @return The SVD decomposition of the matrix.
 */
auto computeSvd(const Eigen::MatrixXf& matrix) -> Eigen::JacobiSVD<Eigen::MatrixXf>
{
    return Eigen::JacobiSVD<Eigen::MatrixXf>(matrix, Eigen::ComputeThinU | Eigen::ComputeThinV);
}

/**
 * @brief Precomputes the pseudo-inverse of the singular value matrix multiplied by V.
 *
 * @param svd The SVD decomposition of the matrix.
 * @return The precomputed matrix (V * Sigma^+) * U^T * c
 */
std::tuple<Eigen::MatrixXf, int> precomputePseudoInverseAndRank(
    const Eigen::JacobiSVD<Eigen::MatrixXf>& svd, float speedOfSound)
{
    // Extract matrices and singular values from the SVD decomposition
    const Eigen::MatrixXf& rightSingularVectors = svd.matrixV();
    const Eigen::MatrixXf& leftSingularVectors = svd.matrixU();
    const Eigen::VectorXf& singularValues = svd.singularValues();

    // Compute the pseudo-inverse of the singular value matrix (Sigma^+)
    Eigen::MatrixXf sigmaPseudoInverse = Eigen::MatrixXf::Zero(rightSingularVectors.cols(), leftSingularVectors.cols());
    int rank = 0;
    for (int i = 0; i < singularValues.size(); ++i)
    {
        if (singularValues(i) > svd.threshold())
        {
            sigmaPseudoInverse(i, i) = 1.0 / singularValues(i);
            rank++;
        }
    }
    Eigen::MatrixXf precomputedInverse =
        (rightSingularVectors * sigmaPseudoInverse) * leftSingularVectors.transpose() * speedOfSound;
    return std::make_tuple(precomputedInverse, rank);
}