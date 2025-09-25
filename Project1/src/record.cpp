#include "record.hpp"
#include <sstream>
#include <string>

using namespace std;

uint32_t dateToInt(const string& s) {
    int day, month, year;
    char sep1, sep2;
    // Expect format "DD/MM/YYYY" and convert to YYYYMMDD as int
    istringstream(s) >> day >> sep1 >> month >> sep2 >> year;
    return year * 10000 + month * 100 + day;
}


string intToDate(uint32_t date_int) {
    int year = date_int / 10000;
    int month = (date_int / 100) % 100;
    int day = date_int % 100;
    // Expect format YYYYMMDD and convert to "DD/MM/YYYY"
    ostringstream oss;
    oss << (day < 10 ? "0" : "") << day << "/" << (month < 10 ? "0" : "") << month << "/" << year;
    return oss.str();
}
