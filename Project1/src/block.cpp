#include "block.hpp"
#include <iostream>

using namespace std;

bool isBlockFull(const Block& block) {
    if (block.num_records >= MAX_RECORDS_PER_BLOCK) {
        return true;
    }
    return false;
}


void resetBlockNumRecords(Block& block) {
    block.num_records = 0;
}


Record readRecord(const Block& block, size_t index) {
    if (index >= static_cast<size_t>(block.num_records)) {
        cerr << "Index out of range" << endl;
        return Record();
    }

    return block.records[index];
}


void printRecord(const Record& rec) {
    cout << "Date: " << intToDate(rec.date) << endl;
    cout << "Team ID Home: " << rec.team_id_home << endl;
    cout << "PTS Home: " << static_cast<int>(rec.pts_home) << endl;
    cout << "FG% Home: " << rec.fg_pct_home << endl;
    cout << "FT% Home: " << rec.ft_pct_home << endl;
    cout << "FG3% Home: " << rec.fg3_pct_home << endl;
    cout << "AST Home: " << static_cast<int>(rec.ast_home) << endl;
    cout << "REB Home: " << static_cast<int>(rec.reb_home) << endl;
    cout << "Home Team Wins: " << rec.home_team_wins << endl;
    cout << endl;
}
