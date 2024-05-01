/*

This file contains all function prototypes for utils.cpp

*/

#ifndef UTILS
#define UTILS

#include <string>
#include <random>
void ProcessFile(const std::string& fileName); 
void ClearQueue(std::queue<std::vector<uint8_t>>& q);
bool withProbability(double probability);
#endif
