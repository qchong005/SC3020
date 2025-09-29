#include "block.h"
#include "constants.h"
#include "disk.h"
#include "record.h"
#include "utils.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set> //Task 3
#include <tuple> //Task 3

Disk::Disk(const std::string &filename) : filename{filename}, ttlBlks{0}, ttlRecs{0}
{
}

bool Disk::loadData()
{
    std::ifstream txtFile{std::string(DATA_FILE)};
    if (!txtFile.is_open())
    {
        std::cerr << "Cannot open file: " << DATA_FILE << '\n';
        return false;
    }

    std::string line;

    // Skip header
    std::getline(txtFile, line);

    while (std::getline(txtFile, line))
        records.emplace_back(parseTxtData(line));

    txtFile.close();

    ttlRecs = records.size();
    ttlBlks = (ttlRecs + MAX_RECORDS_PER_BLOCK - 1) / MAX_RECORDS_PER_BLOCK;

    return writeToDisk(records);
}

Record Disk::parseTxtData(const std::string &data)
{
    Record rec;
    std::stringstream ss(data);
    std::string token;

    std::getline(ss, token, '\t');
    rec.game_date_est = dateToInt_2Byte(token);

    std::getline(ss, token, '\t');
    rec.team_ID_home = token.empty() ? 0 : std::stoul(token);

    std::getline(ss, token, '\t');
    rec.pts_home = static_cast<std::uint8_t>(token.empty() ? 0 : std::stoul(token));

    std::getline(ss, token, '\t');
    rec.fg_pct_home = token.empty() ? 0.0f : std::stof(token);

    std::getline(ss, token, '\t');
    rec.ft_pct_home = token.empty() ? 0.0f : std::stof(token);

    std::getline(ss, token, '\t');
    rec.fg3_pct_home = token.empty() ? 0.0f : std::stof(token);

    std::getline(ss, token, '\t');
    rec.ast_home = static_cast<std::uint8_t>(token.empty() ? 0 : std::stoul(token));

    std::getline(ss, token, '\t');
    rec.reb_home = static_cast<std::uint8_t>(token.empty() ? 0 : std::stoul(token));

    std::getline(ss, token, '\t');
    rec.home_team_wins = static_cast<std::uint8_t>(token.empty() ? 0 : std::stoul(token));

    return rec;
}

bool Disk::writeToDisk(const std::vector<Record> &records)
{
    std::ofstream dbFile(filename, std::ios::binary);
    if (!dbFile.is_open())
    {
        std::cerr << "Cannot Create DB File: " << filename << '\n';
        return false;
    }

    Block block;
    std::size_t recordsInCurrBlock{};

    for (std::size_t i{}; i < records.size(); i++)
    {
        std::memcpy(block.data + recordsInCurrBlock * RECORD_SIZE, &records[i], RECORD_SIZE);
        recordsInCurrBlock++;

        if (recordsInCurrBlock == MAX_RECORDS_PER_BLOCK || i == records.size() - 1)
        {
            dbFile.write(block.data, BLOCK_SIZE);

            std::memset(block.data, 0, BLOCK_SIZE);
            recordsInCurrBlock = 0;
        }
    }

    dbFile.close();
    return true;
}

void Disk::printStats() const
{
    std::cout << "Size of Record: " << sizeof(Record) << " bytes" << std::endl;
    std::cout << "Total No. of Records: " << ttlRecs << '\n';
    std::cout << "Total No. of Blocks: " << ttlBlks << '\n';
}

int Disk::getTtlBlks() const
{
    return (int)ttlBlks;
}

int Disk::getTtlRecs() const
{
    return (int)ttlRecs;
}

std::vector<std::pair<float, RecordRef>> Disk::getAllFTPctHomeValues() const
{
    std::vector<std::pair<float, RecordRef>> ft_pct_values;
    ft_pct_values.reserve(records.size());

    for (std::size_t i = 0; i < records.size(); i++)
    {
        // Calculate block and record position
        std::uint32_t block_id = static_cast<std::uint32_t>(i / MAX_RECORDS_PER_BLOCK);
        std::uint16_t record_offset = static_cast<std::uint16_t>(i % MAX_RECORDS_PER_BLOCK);

        RecordRef ref(block_id, record_offset);
        ft_pct_values.emplace_back(records[i].ft_pct_home, ref);
    }

    return ft_pct_values;
}

Record Disk::getRecord(const RecordRef& ref) const
{
    // Method 1: Read from memory (if records are loaded)
    if (!records.empty()) {
        std::size_t record_index = ref.block_id * MAX_RECORDS_PER_BLOCK + ref.record_offset;
        if (record_index < records.size()) {
            return records[record_index];
        }
    }

    // Method 2: Read from disk file (if records not in memory)
    std::ifstream dbFile(filename, std::ios::binary);
    if (!dbFile.is_open()) {
        std::cerr << "Cannot open database file: " << filename << '\n';
        return Record{}; // Return empty record on error
    }

    // Calculate file position
    std::size_t block_offset = ref.block_id * BLOCK_SIZE;
    std::size_t record_position = block_offset + (ref.record_offset * RECORD_SIZE);

    // Seek to record position and read
    dbFile.seekg(record_position);
    Record record;
    dbFile.read(reinterpret_cast<char*>(&record), RECORD_SIZE);
    dbFile.close();

    return record;
}

std::vector<Record> Disk::getRecords(const std::vector<RecordRef>& refs) const
{
    std::vector<Record> result;
    result.reserve(refs.size());

    for (const auto& ref : refs) {
        result.push_back(getRecord(ref));
    }

    return result;
}

Disk Disk::createDuplicate(const std::string& out_filename) const
{
    Disk duplicate(out_filename);
    // Copy in-memory dataset & stats
    duplicate.records = this->records;
    duplicate.ttlRecs = this->ttlRecs;
    duplicate.ttlBlks = this->ttlBlks;

    // Write to new file to keep structure consistent
    if (!duplicate.writeToDisk(duplicate.records)) {
        std::cerr << "Failed to write duplicated DB file: " << out_filename << '\n';
    }
    return duplicate;
}

std::tuple<int,int,double> Disk::deleteRecordsByRefs(const std::vector<RecordRef> &refs)
{
    if (refs.empty()) return {0, 0, 0.0};

    // Mark-to-delete flags and which blocks are touched
    std::vector<char> mark(records.size(), 0);
    std::unordered_set<std::uint32_t> blocks_touched;

    for (const auto &r : refs) {
        std::size_t idx = static_cast<std::size_t>(r.block_id) * MAX_RECORDS_PER_BLOCK + r.record_offset;
        if (idx < records.size()) {
            mark[idx] = 1;
            blocks_touched.insert(r.block_id);
        }
    }

    // Compute stats & keep survivors
    int deleted = 0;
    double sum_ft = 0.0;
    std::vector<Record> kept;
    kept.reserve(records.size());

    for (std::size_t i = 0; i < records.size(); ++i) {
        if (mark[i]) {
            deleted++;
            sum_ft += records[i].ft_pct_home;
        } else {
            kept.push_back(records[i]);
        }
    }

    // Rewrite compact DB file with survivors (uses your 4KB Block writer)
    if (writeToDisk(kept)) {
        records = std::move(kept);
        ttlRecs = records.size();
        ttlBlks = (ttlRecs + MAX_RECORDS_PER_BLOCK - 1) / MAX_RECORDS_PER_BLOCK;
    } else {
        std::cerr << "Error rewriting DB file during deletion.\n";
    }

    int data_blocks_accessed = static_cast<int>(blocks_touched.size());
    double avg_ft_deleted = deleted ? (sum_ft / deleted) : 0.0;
    return {deleted, data_blocks_accessed, avg_ft_deleted};
}

// Brute-force scan for records with FT_PCT_home above a threshold (for comparison)
std::pair<std::size_t,int> Disk::bruteForceRangeSearch(float threshold) const
{
    std::size_t matches = 0;

    // Walk block-by-block, then record-by-record within the block
    for (std::size_t blk = 0; blk < ttlBlks; ++blk) {
        std::size_t base_idx = blk * MAX_RECORDS_PER_BLOCK;

        for (std::size_t off = 0; off < MAX_RECORDS_PER_BLOCK; ++off) {
            std::size_t idx = base_idx + off;
            // if (idx >= records.size()) break;            // last, partial block
            if (records[idx].ft_pct_home > threshold) {
                ++matches;

                //For debugging:
                //std::cout << "  Match: Block " << blk << ", Offset " << off << ", FT_PCT_home=" << records[idx].ft_pct_home << '\n';
            }
        }
    }

    // Blocks scanned = all blocks, because linear scan touches every block
    return {matches, static_cast<int>(ttlBlks)};
}
