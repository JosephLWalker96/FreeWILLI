#include <iostream>
#include "pico/stdlib.h"
//#include "fftw-3.3.10/api/fftw3.h"
#include <fftw3.h>

int main() {
    stdio_init_all();

    // Define the size of the input array
    int N = 8;

    // Create an input array with some sample data
    float input[8] = {0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    // Allocate memory for the output array
    auto* output = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * (N/2 + 1));

    // Create a plan for the FFT
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(N, input, output, FFTW_ESTIMATE);

    // Execute the FFT
    fftwf_execute(plan);

    // Print the results
    int i=0;
    while (true){
        i %= N/2 + 1;
        std::cout << "Real: " << output[i][0] << ", Imaginary: " << output[i][1] << std::endl;
    }

    return 0;
}