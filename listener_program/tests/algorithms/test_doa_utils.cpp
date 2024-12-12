#include "gtest/gtest.h"
#include "../../src/pch.h"
#include "../../src/algorithms/doa_utils.h"

/*
    the following are tests for convertDoaToElAz function
*/

TEST(ConvertDoaToElAz, UnitVectorPositiveZ)
{
    Eigen::VectorXf doa(3);
    doa << 0.0f, 0.0f, 1.0f;

    Eigen::VectorXf elAz = convertDoaToElAz(doa);

    EXPECT_NEAR(elAz(0), 180.0f, 1e-3);
    EXPECT_NEAR(elAz(1), 0.0f, 1e-3);
}

// Test: Standard Case - Unit vector along positive X-axis
TEST(ConvertDoaToElAz, UnitVectorPositiveX)
{
    Eigen::VectorXf doa(3);
    doa << 1.0f, 0.0f, 0.0f;

    Eigen::VectorXf elAz = convertDoaToElAz(doa);

    EXPECT_NEAR(elAz(0), 90.0f, 1e-3);
    EXPECT_NEAR(elAz(1), 0.0f, 1e-3);
}

// Test: Standard Case - Unit vector along positive Y-axis
TEST(ConvertDoaToElAz, UnitVectorPositiveY)
{
    Eigen::VectorXf doa(3);
    doa << 0.0f, 1.0f, 0.0f;

    Eigen::VectorXf elAz = convertDoaToElAz(doa);

    EXPECT_NEAR(elAz(0), 90.0f, 1e-3);
    EXPECT_NEAR(elAz(1), 90.0f, 1e-3);
}

// Test: Edge Case - Unit vector along negative Z-axis
TEST(ConvertDoaToElAz, UnitVectorNegativeZ)
{
    Eigen::VectorXf doa(3);
    doa << 0.0f, 0.0f, -1.0f;

    Eigen::VectorXf elAz = convertDoaToElAz(doa);

    EXPECT_NEAR(elAz(0), 0.0f, 1e-3);
    EXPECT_NEAR(elAz(1), 0.0f, 1e-3);
}

// Test: Edge Case - Unit vector along negative X-axis
TEST(ConvertDoaToElAz, UnitVectorNegativeX)
{
    Eigen::VectorXf doa(3);
    doa << -1.0f, 0.0f, 0.0f;

    Eigen::VectorXf elAz = convertDoaToElAz(doa);

    EXPECT_NEAR(elAz(0), 90.0f, 1e-3);
    EXPECT_NEAR(elAz(1), 180.0f, 1e-3);
}

// Test: General Case - Arbitrary unit vector
TEST(ConvertDoaToElAz, ArbitraryVector)
{
    Eigen::VectorXf doa(3);
    doa << 0.57735027f, 0.57735027f, 0.57735027f; // Normalized vector (1, 1, 1)

    Eigen::VectorXf elAz = convertDoaToElAz(doa);
    std::cout << "el: " << elAz(0) << std::endl;
    std::cout << "az: " << elAz(1) << std::endl;

    EXPECT_NEAR(elAz(0), 125.264, 1e-3f); // Expected elevation
    EXPECT_NEAR(elAz(1), 45.0f, 1e-3f);   // Expected azimuth
}

// Test: Edge Case - Unit vector in XY plane (azimuth wrap-around)
TEST(ConvertDoaToElAz, WrapAroundAzimuth)
{
    Eigen::VectorXf doa(3);
    doa << -0.70710678f, -0.70710678f, 0.0f; // Normalized (-1, -1, 0)

    Eigen::VectorXf elAz = convertDoaToElAz(doa);

    EXPECT_NEAR(elAz(0), 90.0f, 1e-3f);   // Elevation should be 90 degrees
    EXPECT_NEAR(elAz(1), -135.0f, 1e-3f); // Azimuth should be -135 degrees
}

/*
    the following are tests for computeDoaFromTdoa function
*/

// Test suite for computeDoaFromTdoa
TEST(ComputeDoaFromTdoaTest, ValidInputs)
{
    // Define the precomputed matrices and inputs
    Eigen::MatrixXf precomputedP(3, 3);
    precomputedP << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    Eigen::MatrixXf basisMatrixU(3, 3);
    basisMatrixU << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    float speedOfSound = 1500.0f;
    Eigen::VectorXf tdoa(3);
    tdoa << 0.001f, 0.002f, 0.003f;
    int rank = 3;

    // Call the function
    Eigen::VectorXf doaVector = computeDoaFromTdoa(precomputedP, basisMatrixU, speedOfSound, tdoa, rank);

    // Expected result (scaled TDOA normalized to unit vector)
    Eigen::VectorXf expectedDoa(3);
    expectedDoa << 0.267261f, 0.534522f, 0.801784f; // Normalized result

    // Verify the results
    EXPECT_TRUE((doaVector - expectedDoa).norm() < 1e-4f);
}

TEST(ComputeDoaFromTdoaTest, RankOneDoesNotNormalize)
{
    // Define the precomputed matrices and inputs
    Eigen::MatrixXf precomputedP(3, 3);
    precomputedP << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    Eigen::MatrixXf basisMatrixU(3, 3);
    basisMatrixU << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    float speedOfSound = 1500.0f;
    Eigen::VectorXf tdoa(3);
    tdoa << 0.001f, 0.002f, 0.003f;
    int rank = 1;

    // Call the function
    Eigen::VectorXf doaVector = computeDoaFromTdoa(precomputedP, basisMatrixU, speedOfSound, tdoa, rank);

    // Expected result (scaled TDOA without normalization)
    Eigen::VectorXf expectedDoa(3);
    expectedDoa = tdoa * speedOfSound;

    // Verify the results
    EXPECT_TRUE((doaVector - expectedDoa).norm() < 1e-4f);
}

TEST(ComputeDoaFromTdoaTest, ZeroTdoaVector)
{
    // Define the precomputed matrices and inputs
    Eigen::MatrixXf precomputedP(3, 3);
    precomputedP << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    Eigen::MatrixXf basisMatrixU(3, 3);
    basisMatrixU << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    float speedOfSound = 1500.0f;
    Eigen::VectorXf tdoa(3);
    tdoa << 0.0f, 0.0f, 0.0f;
    int rank = 3;

    // Call the function
    Eigen::VectorXf doaVector = computeDoaFromTdoa(precomputedP, basisMatrixU, speedOfSound, tdoa, rank);

    // Expected result (all zeros)
    Eigen::VectorXf expectedDoa(3);
    expectedDoa << 0.0f, 0.0f, 0.0f;

    // Verify the results
    EXPECT_TRUE((doaVector - expectedDoa).norm() < 1e-4f);
}

TEST(ComputeDoaFromTdoaTest, InvalidRank)
{
    // Define the precomputed matrices and inputs
    Eigen::MatrixXf precomputedP(3, 3);
    precomputedP << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    Eigen::MatrixXf basisMatrixU(3, 3);
    basisMatrixU << 1, 0, 0,
        0, 1, 0,
        0, 0, 1;

    float speedOfSound = 1500.0f;
    Eigen::VectorXf tdoa(3);
    tdoa << 0.001f, 0.002f, 0.003f;
    int rank = 0; // Invalid rank

    // Call the function
    Eigen::VectorXf doaVector = computeDoaFromTdoa(precomputedP, basisMatrixU, speedOfSound, tdoa, rank);

    // Expected result (unchanged TDOA scaled)
    Eigen::VectorXf expectedDoa(3);
    expectedDoa = tdoa * speedOfSound;

    // Verify the results
    EXPECT_TRUE((doaVector - expectedDoa).norm() < 1e-4f);
}

/*
    the following are tests for normalizeDoa function
*/

// Test for full 3D normalization with a typical vector
TEST(NormalizeDoaTest, Full3DNormalizationTypical)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 3.0f, 4.0f, 5.0f;
    normalizeDoa(doaVector, 3);

    EXPECT_NEAR(doaVector.norm(), 1.0f, 1e-6);
    EXPECT_NEAR(doaVector(0), 3.0f / std::sqrt(50.0f), 1e-6);
    EXPECT_NEAR(doaVector(1), 4.0f / std::sqrt(50.0f), 1e-6);
    EXPECT_NEAR(doaVector(2), 5.0f / std::sqrt(50.0f), 1e-6);
}

// Test for 3D normalization with a zero vector
TEST(NormalizeDoaTest, Full3DNormalizationZeroVector)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 0.0f, 0.0f, 0.0f;
    normalizeDoa(doaVector, 3);

    EXPECT_FLOAT_EQ(doaVector.norm(), 0.0f); // Vector remains unchanged
}

// Test for normalization of x and y components (rank 2) with a typical vector
TEST(NormalizeDoaTest, NormalizeXYTypical)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 3.0f, 4.0f, 5.0f;
    normalizeDoa(doaVector, 2);

    double xyNorm = std::sqrt(3.0 * 3.0 + 4.0 * 4.0);
    EXPECT_NEAR(doaVector(0), 3.0 / xyNorm, 1e-6);
    EXPECT_NEAR(doaVector(1), 4.0 / xyNorm, 1e-6);
    EXPECT_FLOAT_EQ(doaVector(2), 5.0f); // z-component remains unchanged
}

// Test for rank 2 normalization with zero x and y components
TEST(NormalizeDoaTest, NormalizeXYZeroXYComponents)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 0.0f, 0.0f, 5.0f;
    normalizeDoa(doaVector, 2);

    EXPECT_FLOAT_EQ(doaVector(0), 0.0f);
    EXPECT_FLOAT_EQ(doaVector(1), 0.0f);
    EXPECT_FLOAT_EQ(doaVector(2), 5.0f); // z-component remains unchanged
}

// Test for rank 1 (no normalization)
TEST(NormalizeDoaTest, Rank1NoNormalization)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 3.0f, 4.0f, 5.0f;
    normalizeDoa(doaVector, 1);

    EXPECT_FLOAT_EQ(doaVector(0), 3.0f);
    EXPECT_FLOAT_EQ(doaVector(1), 4.0f);
    EXPECT_FLOAT_EQ(doaVector(2), 5.0f);
}

// Test for invalid rank
TEST(NormalizeDoaTest, InvalidRank)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 3.0f, 4.0f, 5.0f;
    normalizeDoa(doaVector, 0); // Invalid rank should leave the vector unchanged

    EXPECT_FLOAT_EQ(doaVector(0), 3.0f);
    EXPECT_FLOAT_EQ(doaVector(1), 4.0f);
    EXPECT_FLOAT_EQ(doaVector(2), 5.0f);
}

// Test for edge case with very small values
TEST(NormalizeDoaTest, VerySmallValues)
{
    Eigen::VectorXf doaVector(3);
    doaVector << 1e-7f, 1e-7f, 1e-7f;
    normalizeDoa(doaVector, 3);

    Eigen::VectorXf expectedDoa(3);
    expectedDoa << 0.57735, 0.57735, 0.57735;

    EXPECT_TRUE((doaVector - expectedDoa).norm() < 1e-4f);
}