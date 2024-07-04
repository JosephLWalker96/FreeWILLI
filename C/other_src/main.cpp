#include <iostream>
//#include <eigen3/Eigen/Dense>
//#include <eigen3/unsupported/Eigen/FFT>
//#include <eigen3/Eigen/Core>

#include <armadillo>
//#include <sigpack.h>
//#include <fftw/fftw.h>
//#include <filter/filter.h>
#include <fftw3.h>
#include <chrono>
#include <liquid/liquid.h>

int main() {
    // Define the size of the FFT
    const int N = 992;

    // Create a vector with Armadillo and fill it with sample data (e.g., a sine wave)
    arma::vec x = arma::linspace<arma::vec>(0, 2 * arma::datum::pi, N);
    arma::vec y = arma::sin(x);

    // Allocate memory for the FFT output
    fftw_complex* in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

    // Copy data from Armadillo vector to FFTW input
    for (int i = 0; i < N; ++i) {
        in[i][0] = y(i); // Real part
        in[i][1] = 0.0;  // Imaginary part
    }



    // Create the FIR filter using liquid-dsp
    arma::Col<float> coeffs = {0.1, 0.15, 0.5, 0.15, 0.1};
    int m = coeffs.n_elem;
    firfilt_rrrf filter = firfilt_rrrf_create(&coeffs(0), m);

    // Output buffer
    arma::Col<double> filtered_signal(N, arma::fill::zeros);

    // Apply the FIR filter
    float output;
    for (int j = 0; j < 30; j++){
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < N; ++i) {
            firfilt_rrrf_push(filter, y(i));
            firfilt_rrrf_execute(filter, &output);
            filtered_signal(i) = output;
        }
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double> (endTime - startTime).count();
        std::cout << "filter duration: " << duration << std::endl;
    }

    // Destroy the filter object
    firfilt_rrrf_destroy(filter);












    // Create a plan and execute the FFT
    fftw_plan p;
    for (int i = 0; i < 30; i++){
        auto startTime = std::chrono::steady_clock::now();
        p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(p);
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double> (endTime - startTime).count();
        std::cout << "FFT duration: " << duration << std::endl;
    }

    // Print the FFT output
    /*
    std::cout << "FFT of y:\n";
    for (int i = 0; i < N; ++i) {
        std::cout << out[i][0] << " + " << out[i][1] << "i" << std::endl;
    }
    */

    // Clean up
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return 0;
}
