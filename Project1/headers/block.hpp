#pragma once
#include <cstdint>
#include "record.hpp"

using namespace std;

const size_t BLOCK_SIZE = 4096; // Size of each block in bytes
const size_t MAX_RECORDS_PER_BLOCK = BLOCK_SIZE / sizeof(Record); // Maximum records per block

struct Block {
    Record records[MAX_RECORDS_PER_BLOCK];    // 4096 / 24 = 170 records per block
    int num_records = 0;    // Number of records currently in the block
};

bool isBlockFull(const Block& block, int max_records);
void resetBlockNumRecords(Block& block);

