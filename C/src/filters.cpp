#include "filters.h"
#include <cmath>
#include <cstdlib>

void FilterWithLiquidFIR(Eigen::VectorXf& ch1, Eigen::VectorXf& ch2, Eigen::VectorXf& ch3, Eigen::VectorXf& ch4, 
                         firfilt_rrrf& firFilterCh1, firfilt_rrrf& firFilterCh2, firfilt_rrrf& firFilterCh3, firfilt_rrrf& firFilterCh4) {

    // Helper function to apply filter
    auto applyFilter = [](Eigen::VectorXf& input, firfilt_rrrf& filter) {
        Eigen::VectorXf output(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            firfilt_rrrf_push(filter, input(i));
            float y;
            firfilt_rrrf_execute(filter, &y);
            output(i) = y;
        }
        input = output;
    };

    // Filter each channel
    applyFilter(ch1, firFilterCh1);
    applyFilter(ch2, firFilterCh2);
    applyFilter(ch3, firFilterCh3);
    applyFilter(ch4, firFilterCh4);
}

