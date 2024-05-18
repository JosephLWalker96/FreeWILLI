#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <string>
//#include <sigpack.h>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/

#include "../src/TDOA_estimation.h"
//#include "utils.h" // Assuming utils.h includes the definition of restartListener
//#include "my_globals.h"

using TimePoint = std::chrono::system_clock::time_point;

TEST(ConvertDataTest, ValidDataMidRegion) {
    // Call the function under test
    arma::Col<int> chanSpacing = {1,2,3,1,2,1};
    arma::Col<double> TDOAs(chanSpacing.n_elem);
    
    for (int i = 0; i < chanSpacing.n_elem; i++){
        TDOAs(i) = chanSpacing(i) * 0.0001;
    }
    arma::Col<double> DOAs = {8.6269, 8.6269, 8.6269, 8.6269, 8.6269, 8.6269}; // Ground truth
    

    arma::Col<double> DOAs_est_pos = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
    TDOAs = -1 * TDOAs;
    arma::Col<double> DOAs_est_neg = DOA_EstimateVerticalArray(TDOAs, 1500.0, chanSpacing);
  
    // Assert that the converted data matches the expectation
    for (int i = 0; i < DOAs.n_elem; i++){
        EXPECT_NEAR(DOAs(i), DOAs_est_pos(i), 0.0001); // Check if channels differ after filtering
        EXPECT_NEAR(-1 * DOAs(i), DOAs_est_neg(i), 0.0001); // Check if channels differ after filtering
    }
}
/*
TEST(ConvertDataTest, ValidDataBoundaries) {
    // Call the function under test
    arma::Col<int> chanSpacing = {1,2,3,1,2,1};
    arma::Col<double> TDOAs(chanSpacing.n_elem);
    
    for (int i = 0; i < chanSpacing.n_elem; i++){
        TDOAs(i) = chanSpacing(i) * 0.00066666;
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
/*
TEST(ConvertDataTest, Invalid) {
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
