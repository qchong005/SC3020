#pragma once

#include <cstdint>
#include <string>

bool isLeapYear(const int year);
std::uint16_t dateToInt_2Byte(const std::string &s);
std::string intToDate_2Byte(std::uint16_t days_since_epoch);

bool duplicateFile(const std::string& src, const std::string& dst);