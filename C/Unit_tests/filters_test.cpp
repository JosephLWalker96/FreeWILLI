#include "gtest/gtest.h"
#include <sigpack.h>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include "filters.h"
TEST(FilterTest, BasicFiltering) {

    arma::Col<double> h = { 8.5304705e-18, -1.2040846e-03, -2.7904883e-03, -4.2366693e-03,
     -3.9514871e-03, -9.6724173e-18,  8.2750460e-03,  1.8624326e-02,
      2.5445996e-02,  2.1282293e-02,  2.4025036e-17, -3.9675705e-02,
     -9.2092186e-02, -1.4542012e-01, -1.8531199e-01,  8.0040973e-01,
     -1.8531199e-01, -1.4542012e-01, -9.2092186e-02, -3.9675705e-02,
      2.4025036e-17,  2.1282293e-02,  2.5445996e-02,  1.8624326e-02,
      8.2750460e-03, -9.6724173e-18, -3.9514871e-03, -4.2366693e-03,
     -2.7904883e-03, -1.2040846e-03,  8.5304705e-18};         // filter coefficients
    
    sp::FIR_filt<double, double, double> fir_filt;
    fir_filt.set_coeffs(h);  // Assuming h is defined elsewhere
    
    // Create some sample data
    arma::Col<double> ch1(10, arma::fill::value(3.0));
    arma::Col<double> ch2(10, arma::fill::randn);
    arma::Col<double> ch3(10, arma::fill::randn);
    arma::Col<double> ch4(10, arma::fill::randn);
   
    
    arma::Col<double> ch1_orig = ch1;
    arma::Col<double> ch1_fil = {2.5591e-17, -3.6123e-03, -1.1984e-02, -2.4694e-02, -3.6548e-02, -3.6548e-02, -1.1723e-02, 4.4150e-02, 1.2049e-01, 1.8433e-01};

    // Mock FIR filter (replace with actual filter creation if needed)
    std::cout << "Original ch1:\n" << ch1 << std::endl;  // Print using standard output
    // Apply filter (function under test)
    FilterWithFIR(ch1, ch2, ch3, ch4, fir_filt);
    std::cout << "Filtered ch1:\n" << ch1 << std::endl;  // Print using standard output
    // Verify filtered data (simple example, adjust based on your filter behavior)
    EXPECT_TRUE(arma::all(ch1 != ch2)); // Check if channels differ after filtering
    //EXPECT_TRUE(arma::all(ch1 == ch1_fil)); // Check if channels differ after filtering
    
    for (int i = 0; i < 10; i++){
        EXPECT_NEAR(ch1(i), ch1_fil(i), 0.00001); // Check if channels differ after filtering
    }
}
