#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <string>
#include <sigpack.h>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/

#include "../src/TDOA_estimation.h"
//#include "utils.h" // Assuming utils.h includes the definition of restartListener
#include "../src/custom_types.h"

using TimePoint = std::chrono::system_clock::time_point;

/*
TEST(GCCPHAT, InvalidValues) {
    // Call the function under test

    unsigned int NUM_CHAN = 4;
    const unsigned int SAMPLE_RATE = 100000;
    int fftLength = 100;
    arma::Mat<double> dataMatrixZeros(NUM_CHAN,fftLength, arma::fill::zeros);
    arma::Mat<double> dataMatrixNan(NUM_CHAN,fftLength, arma::fill::randu);
    dataMatrixNan(2,2) = arma::datum::nan;
    arma::Mat<double> dataMatrixInf(NUM_CHAN,fftLength, arma::fill::randu);
    dataMatrixInf(2,2) = arma::datum::inf;
    int interp =1;
    sp::FFTW fftw(fftLength);
    

    EXPECT_THROW(GCC_PHAT(dataMatrixZeros, interp, fftw, fftLength, NUM_CHAN, SAMPLE_RATE), GCC_Value_Error);
    EXPECT_THROW(GCC_PHAT(dataMatrixNan, interp, fftw, fftLength, NUM_CHAN, SAMPLE_RATE), GCC_Value_Error);
    EXPECT_THROW(GCC_PHAT(dataMatrixInf, interp, fftw, fftLength, NUM_CHAN, SAMPLE_RATE), GCC_Value_Error);
  
}
*/

TEST(DOA_EstimateVerticalArray, ValidDataMidRegion) {
    // Call the function under test
    arma::Col<int> chanSpacing = {1,2,3,1,2,1};
    arma::Col<double> TDOAs(chanSpacing.n_elem);
    
    for (int i = 0; i < chanSpacing.n_elem; i++){
        TDOAs(i) = chanSpacing(i) * 0.0001;
    }
    arma::Col<double> DOAs = {81.37309, 81.37309, 81.37309, 81.37309, 81.37309, 81.37309}; // Ground truth
    arma::Col<double> DOAs_neg = {98.6269, 98.6269, 98.6269, 98.6269, 98.6269, 98.6269}; // Ground truth
    

    arma::Col<double> DOAs_est_pos = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
    TDOAs = -1 * TDOAs;
    arma::Col<double> DOAs_est_neg = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
  
    // Assert that the converted data matches the expectation
    for (int i = 0; i < DOAs.n_elem; i++){
        EXPECT_NEAR(DOAs(i), DOAs_est_pos(i), 0.0001); // Check if channels differ after filtering
        EXPECT_NEAR(DOAs_neg(i), DOAs_est_neg(i), 0.0001); // Check if channels differ after filtering
    }
}

TEST(DOA_EstimateVerticalArray, ValidDataBoundaries) {
    // Call the function under test
    arma::Col<int> chanSpacing = {1,2,3,1,2,1};
    arma::Col<double> TDOAs(chanSpacing.n_elem);
    
    for (int i = 0; i < chanSpacing.n_elem; i++){
        TDOAs(i) = chanSpacing(i) * 0.00066666;
    }
    arma::Col<double> DOAs = {0, 0, 0, 0, 0, 0}; // Ground truth
    

    arma::Col<double> DOAs_est_pos = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
    TDOAs = -1*TDOAs;
    arma::Col<double> DOAs_est_neg = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
  
    // Assert that the converted data matches the expectation
    for (int i = 0; i < DOAs.n_elem; i++){
        EXPECT_NEAR(DOAs(i), DOAs_est_pos(i), 1); // Check if channels differ after filtering
        EXPECT_NEAR(180 - DOAs(i), DOAs_est_neg(i), 1); // Check if channels differ after filtering
    }
}

/*
TEST(DOA_EstimateVerticalArray, Invalid) {
    // Call the function under test
    arma::Col<int> chanSpacing = {1,2,3,1,2,1};
    arma::Col<double> TDOAs(chanSpacing.n_elem);
    
    for (int i = 0; i < chanSpacing.n_elem; i++){
        TDOAs(i) = chanSpacing(i) * 0.000667;
    }
    arma::Col<double> DOAs = {90.0, 90.0, 90.0, 90.0, 90.0, 90.0}; // Ground truth
    
    arma::Col<double> DOAs_est_pos = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
    TDOAs = -1 * TDOAs;
    arma::Col<double> DOAs_est_neg = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
  
    // Assert that the converted data matches the expectation
    for (int i = 0; i < DOAs.n_elem; i++){
        EXPECT_NEAR(DOAs(i), DOAs_est_pos(i), 0.0001); // Check if channels differ after filtering
        EXPECT_NEAR(-1 * DOAs(i), DOAs_est_neg(i), 0.0001); // Check if channels differ after filtering
    }

}
*/
