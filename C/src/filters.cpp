#include <cmath>
#include <cstdlib>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/

//#include <eigen3/Eigen/Dense>
//#include <eigen3/unsupported/Eigen/FFT>
//#include <eigen3/Eigen/Core>
//#include <sigpack.h>

#include "custom_types.h"
#include <liquid/liquid.h>
#include "filters.h"
//#include "TDOA_estimation.h"

// #include "gtest/gtest.h"
//#include <fftw/fftw.h>
/*
void FilterWithFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& firFilter) {
    ch1 = firFilter.filter(ch1);
    firFilter.clear();
    ch2 = firFilter.filter(ch2);
    firFilter.clear();
    ch3 = firFilter.filter(ch3);
    firFilter.clear();
    ch4 = firFilter.filter(ch4);
    firFilter.clear();
}
*/

void FilterWithLiquidFIR(Eigen::VectorXd& ch1, Eigen::VectorXd& ch2, Eigen::VectorXd& ch3, Eigen::VectorXd& ch4, 
                         firfilt_rrrf& q1, firfilt_rrrf& q2, firfilt_rrrf& q3, firfilt_rrrf& q4) {

    // Helper function to apply filter
    auto applyFilter = [](Eigen::VectorXd& input, firfilt_rrrf& q) {
        Eigen::VectorXd output(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            firfilt_rrrf_push(q, static_cast<float>(input(i)));
            float y;
            firfilt_rrrf_execute(q, &y);
            output(i) = y;
        }
        input = output;
    };

    // Filter each channel
    applyFilter(ch1, q1);
    applyFilter(ch2, q2);
    applyFilter(ch3, q3);
    applyFilter(ch4, q4);
}



/*
void FilterWithIIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::IIR_filt<double, double, double>& iirFilter){
    //
    // THIS FUNCTION IS DEPRICATED
    //
    
    arma::Col<double> ch1Padded = arma::join_cols(arma::reverse(ch1.subvec(0,14)), ch1, arma::reverse(ch1.subvec(ch1.n_elem - 15, ch1.n_elem - 1)));
    arma::Col<double> ch2Padded = arma::join_cols(arma::reverse(ch2.subvec(0,14)), ch2, arma::reverse(ch2.subvec(ch2.n_elem - 15, ch2.n_elem - 1)));
    arma::Col<double> ch3Padded = arma::join_cols(arma::reverse(ch3.subvec(0,14)), ch3, arma::reverse(ch3.subvec(ch3.n_elem - 15, ch3.n_elem - 1)));
    arma::Col<double> ch4Padded = arma::join_cols(arma::reverse(ch4.subvec(0,14)), ch4, arma::reverse(ch4.subvec(ch4.n_elem - 15, ch4.n_elem - 1)));

    arma::Col<double> ch1Filtered = arma::reverse(iirFilter.filter(ch1Padded));
    ch1Filtered = arma::reverse(iirFilter.filter(ch1Filtered));
    arma::Col<double> ch2Filtered = arma::reverse(iirFilter.filter(ch2Padded));
    ch2Filtered = arma::reverse(iirFilter.filter(ch2Filtered));
    arma::Col<double> ch3Filtered = arma::reverse(iirFilter.filter(ch3Padded));
    ch3Filtered = arma::reverse(iirFilter.filter(ch3Filtered));
    arma::Col<double> ch4Filtered = arma::reverse(iirFilter.filter(ch4Padded));
    ch4Filtered = arma::reverse(iirFilter.filter(ch4Filtered));
}
*/
