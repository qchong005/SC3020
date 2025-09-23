#include "heap_file.hpp"
#include "serde.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

// safe converters
bool safe_stoi(const std::string& s, int& out) {
    try { if (s.empty()) return false; size_t idx=0; out=std::stoi(s,&idx); return idx==s.size(); }
    catch(...) { return false; }
}
bool safe_stof(const std::string& s, float& out) {
    try { if (s.empty()) return false; size_t idx=0; out=std::stof(s,&idx); return idx==s.size(); }
    catch(...) { return false; }
}

size_t load_tsv_into_heap(const std::string& path, HeapFile& heap) {
    std::ifstream in(path);
    if (!in) die("Cannot open data file");

    std::string line;
    if (!std::getline(in, line)) return 0; // skip header

    size_t rows = 0;
    char buf[28];

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        char delim = (line.find('\t') != std::string::npos) ? '\t' : ',';

        std::stringstream ss(line);
        std::string col;
        std::vector<std::string> cols;
        while (std::getline(ss, col, delim)) cols.push_back(col);
        if (cols.size() < 9) continue;

        Rec r{};
        int tmp; float ftmp;

        r.date_u32 = date_to_days(cols[0]);
        if (!safe_stoi(cols[1], r.team_id)) continue;
        if (!safe_stoi(cols[2], tmp)) continue; r.pts = (int16_t)tmp;
        if (!safe_stof(cols[3], ftmp)) continue; r.fg_pct = ftmp;
        if (!safe_stof(cols[4], ftmp)) continue; r.ft_pct = ftmp;
        if (!safe_stof(cols[5], ftmp)) continue; r.fg3_pct = ftmp;
        if (!safe_stoi(cols[6], tmp)) continue; r.ast = (int16_t)tmp;
        if (!safe_stoi(cols[7], tmp)) continue; r.reb = (int16_t)tmp;
        if (!safe_stoi(cols[8], tmp)) continue; r.win = (uint8_t)tmp;

        rec_to_bytes(r, buf);
        heap.insert(buf);
        rows++;
    }

    return rows;
}
