#pragma once
#include <iostream>
#include <cstdint>
#include <string>
#include <sstream>

using namespace std;

struct Record {
    uint32_t date;
    uint32_t team_id_home;
    float fg_pct_home;
    float ft_pct_home;
    float fg3_pct_home;
    uint8_t pts_home;    // changed order of members to save space
    uint8_t ast_home;
    uint8_t reb_home;
    bool home_team_wins;
};


uint32_t dateToInt(const string& s);    // Expect format "DD/MM/YYYY" and convert to YYYYMMDD as int
string intToDate(uint32_t date_int);    // Convert YYYYMMDD as int back to "DD/MM/YYYY"
