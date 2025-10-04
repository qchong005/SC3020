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
    std::cout << "Max Records per Block: " << MAX_RECORDS_PER_BLOCK << '\n';
    std::cout << "No. of Records in Last Block: "
              << (ttlRecs % MAX_RECORDS_PER_BLOCK == 0 ? MAX_RECORDS_PER_BLOCK : ttlRecs % MAX_RECORDS_PER_BLOCK)
              << '\n';
    std::cout << "Total No. of Blocks: " << ttlBlks << '\n';
    std::cout << "No. of Records in Last Block: "
              << (ttlRecs % MAX_RECORDS_PER_BLOCK == 0 ? MAX_RECORDS_PER_BLOCK : ttlRecs % MAX_RECORDS_PER_BLOCK)
              << '\n';
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

Record Disk::getRecord(const RecordRef &ref) const
{
    // Always read from disk file to simulate actual disk I/O
    std::ifstream dbFile(filename, std::ios::binary);
    if (!dbFile.is_open())
    {
        std::cerr << "Cannot open database file: " << filename << '\n';
        return Record{}; // Return empty record on error
    }

    // Calculate file position
    std::size_t block_offset = ref.block_id * BLOCK_SIZE;
    std::size_t record_position = block_offset + (ref.record_offset * RECORD_SIZE);

    // Seek to record position and read
    dbFile.seekg(record_position);
    Record record;
    dbFile.read(reinterpret_cast<char *>(&record), RECORD_SIZE);
    dbFile.close();

    return record;
}

std::vector<Record> Disk::getRecords(const std::vector<RecordRef> &refs) const
{
    std::vector<Record> result;
    result.reserve(refs.size());

    for (const auto &ref : refs)
    {
        result.push_back(getRecord(ref));
    }

    return result;
}

bool Disk::deleteRecord(const RecordRef &ref)
{
    // Open database file for writing
    std::fstream dbFile(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!dbFile.is_open())
    {
        std::cerr << "Cannot open database file for deletion: " << filename << '\n';
        return false;
    }

    // Calculate file position
    std::size_t block_offset = ref.block_id * BLOCK_SIZE;
    std::size_t record_position = block_offset + (ref.record_offset * RECORD_SIZE);

    // Write zeros to mark as deleted
    Record empty_record{};
    dbFile.seekp(record_position);
    dbFile.write(reinterpret_cast<const char *>(&empty_record), RECORD_SIZE);
    dbFile.close();

    return true;
}

int Disk::deleteRecords(const std::vector<RecordRef> &refs)
{
    int deleted_count = 0;

    for (const auto &ref : refs)
    {
        if (deleteRecord(ref))
        {
            deleted_count++;
        }
    }

    return deleted_count;
}
