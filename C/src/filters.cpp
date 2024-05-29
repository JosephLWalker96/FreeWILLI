#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include <iomanip> // for output formatting

#include <eigen3/Eigen/Dense>
#include <eigen3/unsupported/Eigen/FFT>
#include <eigen3/Eigen/Core>
#include <sigpack.h>

#include "custom_types.h"
#include "filters.h"
//#include "TDOA_estimation.h"

#include "gtest/gtest.h"
#include <fftw/fftw.h>

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;

void FilterWithFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& fir_filt){
    ch1 = fir_filt.filter(ch1);
    fir_filt.clear();
    ch2 = fir_filt.filter(ch2);
    fir_filt.clear();
    ch3 = fir_filt.filter(ch3);
    fir_filt.clear();
    ch4 = fir_filt.filter(ch4);
    fir_filt.clear();
}

void FilterWithLiquidFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& fir_filt){
    //
    // THIS FUNCTION IS DEPRICATED
    //

    /*
    float ch1_filtered_test[ch1.n_elem];
    float ch2_filtered_test[ch1.n_elem];
    float ch3_filtered_test[ch1.n_elem];
    float ch4_filtered_test[ch1.n_elem];
    

    for (int i=0; i<ch1.n_elem; i++) {
        firfilt_rrrf_push(q, ch1(i));
        firfilt_rrrf_execute(q, &ch1_filtered_test[i]);
    }
    for (int i=0; i<ch2.n_elem; i++) {
        firfilt_rrrf_push(q, ch2(i));
        firfilt_rrrf_execute(q, &ch2_filtered_test[i]);
    }
    for (int i=0; i<ch3.n_elem; i++) {
        firfilt_rrrf_push(q, ch3(i));
        firfilt_rrrf_execute(q, &ch3_filtered_test[i]);
    }
    for (int i=0; i<ch4.n_elem; i++) {
        firfilt_rrrf_push(q, ch4(i));
        firfilt_rrrf_execute(q, &ch4_filtered_test[i]);
    }
    */
}


void FilterWithIIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::IIR_filt<double, double, double>& iir_filt){
    //
    // THIS FUNCTION IS DEPRICATED
    //
    
    arma::Col<double> ch1_padded = arma::join_cols(arma::reverse(ch1.subvec(0,14)), ch1, arma::reverse(ch1.subvec(ch1.n_elem - 15, ch1.n_elem - 1)));
    arma::Col<double> ch2_padded = arma::join_cols(arma::reverse(ch2.subvec(0,14)), ch2, arma::reverse(ch2.subvec(ch2.n_elem - 15, ch2.n_elem - 1)));
    arma::Col<double> ch3_padded = arma::join_cols(arma::reverse(ch3.subvec(0,14)), ch3, arma::reverse(ch3.subvec(ch3.n_elem - 15, ch3.n_elem - 1)));
    arma::Col<double> ch4_padded = arma::join_cols(arma::reverse(ch4.subvec(0,14)), ch4, arma::reverse(ch4.subvec(ch4.n_elem - 15, ch4.n_elem - 1)));
    cout << "ch1 size: " << ch1.n_elem << endl;
    cout << "ch1_padded size: " << ch1_padded.n_elem << endl;

    arma::Col<double> ch1_filtered = arma::reverse(iir_filt.filter(ch1_padded));
    ch1_filtered = arma::reverse(iir_filt.filter(ch1_filtered));
    arma::Col<double> ch2_filtered = arma::reverse(iir_filt.filter(ch2_padded));
    ch2_filtered = arma::reverse(iir_filt.filter(ch2_filtered));
    arma::Col<double> ch3_filtered = arma::reverse(iir_filt.filter(ch3_padded));
    ch3_filtered = arma::reverse(iir_filt.filter(ch3_filtered));
    arma::Col<double> ch4_filtered = arma::reverse(iir_filt.filter(ch4_padded));
    ch4_filtered = arma::reverse(iir_filt.filter(ch4_filtered));
}
