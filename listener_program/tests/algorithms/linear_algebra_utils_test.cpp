#include "../../src/algorithms/linear_algebra_utils.h"

// #include "../../src/pch.h"
#include "gtest/gtest.h"

// Test: SVD decomposition of a simple 2x2 matrix
TEST(LinearAlgebraUtilsTest, ComputeSvdBasic)
{
    Eigen::MatrixXf matrix(2, 2);
    matrix << 3, 2, 2, 3;

    Eigen::JacobiSVD<Eigen::MatrixXf> svd = computeSvd(matrix);

    EXPECT_EQ(svd.matrixU().rows(), 2);
    EXPECT_EQ(svd.matrixV().cols(), 2);
    EXPECT_EQ(svd.singularValues().size(), 2);
}

// Test: SVD decomposition of an identity matrix
TEST(LinearAlgebraUtilsTest, ComputeSvdIdentityMatrix)
{
    Eigen::MatrixXf identity = Eigen::MatrixXf::Identity(3, 3);
    Eigen::JacobiSVD<Eigen::MatrixXf> svd = computeSvd(identity);

    Eigen::VectorXf singularValues = svd.singularValues();
    for (int i = 0; i < singularValues.size(); ++i)
    {
        EXPECT_NEAR(singularValues(i), 1.0, 1e-6);
    }
}

// Test: Precomputing pseudo-inverse of a simple 2x2 matrix
TEST(LinearAlgebraUtilsTest, PrecomputePseudoInverseBasic)
{
    Eigen::MatrixXf matrix(2, 2);
    matrix << 4, 7, 2, 6;

    Eigen::JacobiSVD<Eigen::MatrixXf> svd = computeSvd(matrix);
    auto tuple = precomputePseudoInverseAndRank(svd, 1.0f);
    auto precomputed = std::get<0>(tuple);
    // Check dimensions
    EXPECT_EQ(precomputed.rows(), 2);
    EXPECT_EQ(precomputed.cols(), 2);

    // Ensure Pseudo-inverse is correct by verifying A * A^+ ≈ I
    Eigen::MatrixXf identityApprox = matrix * precomputed;

    EXPECT_NEAR(identityApprox(0, 0), 1.0, 1e-3);
    EXPECT_NEAR(identityApprox(1, 1), 1.0, 1e-3);
}

// Test: Compute rank of a full-rank matrix

TEST(LinearAlgebraUtilsTest, ComputeRankFullRankMatrix)
{
    Eigen::MatrixXf matrix(3, 3);
    matrix << 2, -1, 0, -1, 2, -1, 0, -1, 2;

    auto tuple = precomputePseudoInverseAndRank(computeSvd(matrix), 1.0f);
    auto rank = std::get<1>(tuple);
    EXPECT_EQ(rank, 3);
}

// Test: Compute rank of a rank-deficient matrix
TEST(LinearAlgebraUtilsTest, ComputeRankDeficientMatrix)
{
    Eigen::MatrixXf matrix(3, 3);
    matrix << 1, 2, 3, 4, 5, 6, 7, 8, 9;  // Linearly dependent rows

    auto tuple = precomputePseudoInverseAndRank(computeSvd(matrix), 1.0f);
    auto rank = std::get<1>(tuple);
    EXPECT_EQ(rank, 2);
}

// Test: Compute rank of a zero matrix
TEST(LinearAlgebraUtilsTest, ComputeRankZeroMatrix)
{
    Eigen::MatrixXf zeroMatrix = Eigen::MatrixXf::Zero(4, 4);
    auto tuple = precomputePseudoInverseAndRank(computeSvd(zeroMatrix), 1.0f);
    auto rank = std::get<1>(tuple);
    EXPECT_EQ(rank, 0);
}

// Test: Compute rank of a non-square matrix

TEST(LinearAlgebraUtilsTest, ComputeRankNonSquareMatrix)
{
    Eigen::MatrixXf matrix(4, 2);
    matrix << 1, 2, 3, 4, 5, 6, 7, 8;

    auto tuple = precomputePseudoInverseAndRank(computeSvd(matrix), 1.0f);
    auto rank = std::get<1>(tuple);
    EXPECT_EQ(rank, 2);
}