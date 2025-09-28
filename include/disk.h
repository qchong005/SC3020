#pragma once
#include "bplus_tree.h"
#include "record.h"
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

class Disk
{
  private:
    std::string filename;
    std::size_t ttlBlks;
    std::size_t ttlRecs;
    std::vector<Record> records; // Store loaded records for indexing

  public:
    Disk(const std::string &filename = "./data/data.db");
    ~Disk() = default;

    bool loadData();
    Record parseTxtData(const std::string &data_file);

    bool writeToDisk(const std::vector<Record> &records);

    int getTtlBlks() const;
    int getTtlRecs() const;

    // Under Review
    // Method to get all FT_PCT_home values with their record references for indexing
    std::vector<std::pair<float, RecordRef>> getAllFTPctHomeValues() const;

    // Method to retrieve a record using RecordRef (block_id + record_offset)
    Record getRecord(const RecordRef& ref) const;

    // Method to retrieve multiple records using RecordRefs
    std::vector<Record> getRecords(const std::vector<RecordRef>& refs) const;

    void printStats() const;
};
