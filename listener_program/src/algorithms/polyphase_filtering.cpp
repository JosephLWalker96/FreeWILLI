#include "polyphase_filtering.h"

/**
 * @brief Reads filter taps from a file.
 * @param fileName The name of the file containing filter taps.
 * @return A vector of filter tap values.
 */
std::vector<double> readFilterTaps(const std::string& fileName)
{
    std::vector<double> taps;
    std::ifstream file(fileName);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + fileName);
    }

    double value;
    while (file >> value)
    {
        taps.push_back(value);
    }

    file.close();
    return taps;
}

/**
 * @brief Reads a time series from a file.
 * @param fileName The name of the file containing time series data.
 * @return A vector of time series values.
 */
std::vector<double> readTimeSeries(const std::string& fileName)
{
    std::vector<double> data;
    std::ifstream file(fileName);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + fileName);
    }

    double value;
    while (file >> value)
    {
        data.push_back(value);
    }

    file.close();
    return data;
}

/**
 * @brief Writes a time series to a file.
 * @param data The time series data to write.
 * @param fileName The name of the file to write to.
 */
void writeTimeSeries(const std::vector<double>& data, const std::string& fileName)
{
    std::ofstream file(fileName);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + fileName);
    }

    for (const double& value : data)
    {
        file << value << "\n";
    }

    file.close();
}

/**
 * @brief Computes the greatest common divisor (GCD) of two integers.
 * @param a First integer.
 * @param b Second integer.
 * @return The GCD of a and b.
 */
int computeGcd(int a, int b)
{
    while (b != 0)
    {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

/**
 * @brief Performs efficient polyphase resampling.
 * @param x Input signal.
 * @param up Upsampling factor.
 * @param down Downsampling factor.
 * @param h Filter coefficients.
 * @return The resampled signal.
 */
std::vector<double> resamplePoly(const std::vector<double>& x, int up, int down, const std::vector<double>& h)
{
    if (up < 1 || down < 1)
    {
        throw std::invalid_argument("up and down must be >= 1");
    }

    if (up == 1 && down == 1)
    {
        return x;
    }

    std::vector<std::vector<double>> polyphaseFilters(up);

    for (size_t i = 0; i < h.size(); ++i)
    {
        polyphaseFilters[i % up].push_back(h[i]);
    }

    std::cout << "Num filters: " << polyphaseFilters.size() << std::endl;
    std::cout << "Size filters: " << polyphaseFilters[0].size() << std::endl;

    size_t nOut = (x.size() * up + down - 1) / down;
    std::vector<double> y(nOut, 0.0);

    // #pragma omp parallel for // Example for multithreading using OpenMP
    for (size_t i = 0; i < nOut; ++i)
    {
        int phase = i % up;
        int offset = (i * down) / up;

        if (offset < x.size())
        {
            double sum = 0.0;
            for (size_t j = 0; j < polyphaseFilters[phase].size(); ++j)
            {
                if (offset + j < x.size())
                {
                    sum += x[offset + j] * polyphaseFilters[phase][j];
                }
                else
                {
                    break;
                }
            }
            y[i] = sum;
        }
    }
    return y;
}