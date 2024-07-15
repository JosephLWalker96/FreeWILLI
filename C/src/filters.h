/*

This file contains all function prototypes for filters.cpp

*/

#pragma once

#include "pch.h"

void FilterWithLiquidFIR(Eigen::VectorXf& ch1, Eigen::VectorXf& ch2, Eigen::VectorXf& ch3, Eigen::VectorXf& ch4, 
                         firfilt_rrrf& firFilterCh1, firfilt_rrrf& firFilterCh2, firfilt_rrrf& firFilterCh3, firfilt_rrrf& firFilterCh4);
void ApplyLiquidFIR(Eigen::VectorXf& input, firfilt_rrrf& filter);
