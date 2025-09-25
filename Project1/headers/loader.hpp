#pragma once
#include <string>
#include <cstdlib>
#include "disk.hpp"
#include "block.hpp"

using namespace std;

void loadCSVData(const string& data_file, Disk& disk);
float parseFloatOrZero(const string& s);

// Template function to parse integer types according to their type with error handling
// If parsing fails or missing data, return 0 of that type
template<typename T>
T parseIntOrZero(const string& s) {
    try {
        return static_cast<T>(stoi(s));
    }
    catch (const exception& e) {
        return static_cast<T>(0);
    }
}
