#pragma once
#include "common.hpp"
#include <string>
#include <ctime>

struct Rec {
    uint32_t date_u32;
    int32_t  team_id;
    int16_t  pts;
    float    fg_pct;
    float    ft_pct;
    float    fg3_pct;
    int16_t  ast;
    int16_t  reb;
    uint8_t  win;
    uint8_t  pad;
};

inline void rec_to_bytes(const Rec& r, char* out28) {
    std::memcpy(out28+0,  &r.date_u32, 4);
    std::memcpy(out28+4,  &r.team_id, 4);
    std::memcpy(out28+8,  &r.pts, 2);
    std::memcpy(out28+10, &r.fg_pct, 4);
    std::memcpy(out28+14, &r.ft_pct, 4);
    std::memcpy(out28+18, &r.fg3_pct, 4);
    std::memcpy(out28+22, &r.ast, 2);
    std::memcpy(out28+24, &r.reb, 2);
    std::memcpy(out28+26, &r.win, 1);
    out28[27] = 0;
}

inline uint32_t date_to_days(const std::string& ddmmyyyy) {
    if (ddmmyyyy.size() < 10) return 0;
    int d = std::stoi(ddmmyyyy.substr(0,2));
    int m = std::stoi(ddmmyyyy.substr(3,2));
    int y = std::stoi(ddmmyyyy.substr(6,4));
    std::tm t = {}; t.tm_mday = d; t.tm_mon = m-1; t.tm_year = y-1900;
    std::time_t tt = std::mktime(&t);
    return static_cast<uint32_t>(tt / 86400);
}
