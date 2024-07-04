#include <iostream>
#include <vector>
#include <cmath>
#include <fftw3.h>
#include <armadillo>
#include <sigpack.h>
int main() {

    // Create a 3x3 matrix with Armadillo
    arma::Mat<double> A(3, 3, arma::fill::randu);
    std::cout << "Armadillo Matrix A:\n" << A << std::endl;

    // Perform an FFT using SigPack
    arma::Col<double> x = arma::linspace<arma::Col<double>>(0, 2 * arma::datum::pi, 100);
    arma::Col<double> y = arma::sin(x);

    sp::FFTW fftw(100);

    // Number of points
    int N = 100;

    // Create input data (a sine wave)
    std::vector<double> in(N);
    for (int i = 0; i < N; ++i) {
        in[i] = std::sin(2 * M_PI * i / N);
    }

    // Create output array
    std::vector<fftw_complex> out(N);

    // Create FFTW plan
    //fftw_plan plan = fftw_plan_dft_r2c_1d(N, in.data(), out.data(), FFTW_ESTIMATE);

    // Execute the plan
    //fftw_execute(plan);

    // Output the results
    std::cout << "FFT results:\n";
    //for (int i = 0; i < N; ++i) {
    //    std::cout << out[i][0] << " + " << out[i][1] << "i\n";  // Real and imaginary parts
    //}
}
