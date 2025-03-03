#include "../pch.h"

#pragma once

std::vector<double> readFilterTaps(const std::string& fileName);

std::vector<double> readTimeSeries(const std::string& fileName);

void writeTimeSeries(const std::vector<double>& data, const std::string& fileName);

int computeGcd(int a, int b);
std::vector<double> resamplePoly(const std::vector<double>& x, int up, int down, const std::vector<double>& h);