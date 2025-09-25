#include "constants.h"
#include "utils.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

bool isLeapYear(const int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

std::uint16_t dateToInt_2Byte(const std::string &s)
{
    std::stringstream ss(s);
    std::string day_str, month_str, year_str;

    std::getline(ss, day_str, '/');
    std::getline(ss, month_str, '/');
    std::getline(ss, year_str);

    int day{std::stoi(day_str)};
    int month{std::stoi(month_str)};
    int year{std::stoi(year_str)};

    int ttlDays{};

    for (int y{(int)EPOCH_YEAR}; y < year; y++)
        ttlDays += isLeapYear(y) ? 366 : 365;

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (isLeapYear(year))
        daysInMonth[1] = 29;

    for (int m = 0; m < month - 1; m++)
        ttlDays += daysInMonth[m];

    // Add remaining days
    ttlDays += day;

    return static_cast<std::uint16_t>(ttlDays);
}

std::string intToDate_2Byte(std::uint16_t days_since_epoch)
{
    int year(EPOCH_YEAR);
    int remainingDays(days_since_epoch);

    while (true)
    {
        int daysInYear{isLeapYear(year) ? 366 : 365};

        if (remainingDays <= daysInYear)
            break;

        remainingDays -= daysInYear;
        year++;
    }

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (isLeapYear(year))
        daysInMonth[1] = 29;

    int month{};
    while (remainingDays > daysInMonth[month])
    {
        remainingDays -= daysInMonth[month];
        month++;
    }

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << remainingDays << '/' << std::setfill('0') << std::setw(2) << month + 1
       << '/' << year;

    return ss.str();
}
