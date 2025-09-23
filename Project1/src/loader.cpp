#include <iostream>
#include <fstream>
#include <sstream>
#include "loader.hpp"
#include "record.hpp"
#include "block.hpp"
#include "disk.hpp"

using namespace std;

void loadCSVData(const string& data_file, Disk& disk) {
    // open data_file as "infile"
    ifstream infile(data_file);     
    if (!infile.is_open()) {
        throw runtime_error("Cannot open CSV file: " + data_file);
    }

    string row;
    Block block;

    // skip header row
    getline(infile, row);
    int line_number = 1; // to track line numbers for error messages

    while (getline(infile, row)) {
        line_number++;
        stringstream ss(row);   // treat each row like an input
        string val;    // to hold each field value temporarily
        Record rec;

        // read each row until tab, then store the value in val based on order of fields
        // not based on order of storage
        getline(ss, val, '\t');    
        rec.date = dateToInt(val);  // parse date

        getline(ss, val, '\t');
        rec.team_id_home = parseIntOrZero<uint32_t>(val);  // convert string to 32-bit int

        getline(ss, val, '\t');
        rec.pts_home = parseIntOrZero<uint8_t>(val);    // convert string to 8-bit int

        getline(ss, val, '\t');
        rec.fg_pct_home = parseFloatOrZero(val);    // convert string to float

        getline(ss, val, '\t');
        rec.ft_pct_home = parseFloatOrZero(val);

        getline(ss, val, '\t');
        rec.fg3_pct_home = parseFloatOrZero(val);

        getline(ss, val, '\t');
        rec.ast_home = parseIntOrZero<uint8_t>(val);

        getline(ss, val, '\t');
        rec.reb_home = parseIntOrZero<uint8_t>(val);

        getline(ss, val, '\t');
        rec.home_team_wins = (val == "1");

        block.records[block.num_records] = rec;   // add record to block
        block.num_records++;

        if (isBlockFull(block, MAX_RECORDS_PER_BLOCK)) {
            disk.writeBlock(block);         // write block to disk
            resetBlockNumRecords(block);    // reset num_records to 0 for next block
        }

    }

    if (block.num_records > 0) {
        disk.writeBlock(block);   // write any remaining records in partially filled block
    }

    infile.close();  // close the file

}


// If parsing fails or missing data, return 0.0f
float parseFloatOrZero(const string& s) {
    try {
        return stof(s);
    }
    catch (const exception& e) {
        return 0.0f;
    }
}
