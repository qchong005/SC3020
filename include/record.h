#pragma once

#include <cstdint>

#pragma pack(push, 1)
// 22 bytes in total
struct Record
{
    float fg_pct_home;
    float ft_pct_home;
    float fg3_pct_home;
    std::uint32_t team_ID_home;
    std::uint16_t game_date_est;
    std::uint8_t pts_home;
    std::uint8_t ast_home;
    std::uint8_t reb_home;
    std::uint8_t home_team_wins;
};
#pragma pack(pop)
