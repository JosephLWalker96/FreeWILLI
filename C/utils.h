/*

This file contains all function prototypes for utils.cpp

*/

#ifndef UTILS
#define UTILS

#include <string>

void ProcessFile(const std::string& fileName); 
void ClearQueue(std::queue<std::vector<uint8_t>>& q);
#endif
