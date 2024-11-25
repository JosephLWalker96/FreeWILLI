#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <numeric>
#include <chrono>
#include <omp.h>

std::vector<double> read_filter_taps(const std::string &filename)
{
    std::vector<double> taps;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    double value;
    while (file >> value)
    {
        taps.push_back(value);
    }

    file.close();
    return taps;
}

// Function to read a time series from a file
std::vector<double> read_time_series(const std::string &filename)
{
    std::vector<double> data;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    double value;
    while (file >> value)
    {
        data.push_back(value);
    }

    file.close();
    return data;
}

// Function to write a time series to a file
void write_time_series(const std::vector<double> &data, const std::string &filename)
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    for (const double &value : data)
    {
        file << value << "\n";
    }

    file.close();
}
// Function to calculate the gcd
int gcd(int a, int b)
{
    while (b != 0)
    {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}
// Efficient polyphase resampling
std::vector<double> resample_poly(const std::vector<double> &x, int up, int down, const std::vector<double> &h)
{
    if (up < 1 || down < 1)
    {
        throw std::invalid_argument("up and down must be >= 1");
    }

    if (up == 1 && down == 1)
    {
        return x;
    }

    // int num_phases = down;
    std::vector<std::vector<double>> polyphase_filters(up);

    for (size_t i = 0; i < h.size(); ++i)
    {
        polyphase_filters[i % up].push_back(h[i]);
    }
    std::cout << "Num filters: " << polyphase_filters.size() << std::endl;
    std::cout << "Size filters: " << polyphase_filters[0].size() << std::endl;

    size_t n_out = (x.size() * up + down - 1) / down;
    std::vector<double> y(n_out, 0.0);

#pragma omp parallel for // Example for multithreading using OpenMP
    for (size_t i = 0; i < n_out; ++i)
    {
        int phase = i % up;
        int offset = (i * down) / up;

        if (offset < x.size())
        {
            double sum = 0.0;
            for (size_t j = 0; j < polyphase_filters[phase].size(); ++j)
            {
                if (offset + j < x.size())
                {
                    sum += x[offset + j] * polyphase_filters[phase][j];
                }
                else
                {
                    std::cout << "Breaking" << std::endl;
                    break;
                }
            }
            y[i] = sum;
        }
    }
    return y;
}

int main()
{
    try
    {
        // Read input time series from file
        std::string input_filename = "input_timeseries.txt";
        std::vector<double> input_signal = read_time_series(input_filename);
        std::cout << "length of input signal: " << input_signal.size() << std::endl;

        // Define upsampling and downsampling factors
        int up = 212;   // Example upsampling factor
        int down = 100; // Example downsampling factor

        // Simplify the up and down factors using the greatest common divisor
        int g_ = gcd(up, down);
        up /= g_;
        down /= g_;
        std::cout << "after gcd: " << up << " " << down << std::endl;

        // Define FIR filter coefficients (example filter)
        // std::vector<double> h = {0.1, 0.2, 0.3, 0.2, 0.1}; // Replace with your filter coefficients
        std::vector<double> h = read_filter_taps("filterTaps.txt");
        std::cout << "length of filter: " << h.size() << std::endl;

        // Perform polyphase filtering
        auto startTime = std::chrono::high_resolution_clock::now();
        std::vector<double> output_signal = resample_poly(input_signal, up, down, h);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(endTime - startTime);
        std::cout << "runtime: " << duration.count() << std::endl;

        // Write output time series to file
        std::string output_filename = "output_timeseries3.txt";
        write_time_series(output_signal, output_filename);

        std::cout << "Polyphase filtering completed successfully.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}