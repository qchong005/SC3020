#include "block.hpp"
#include <iostream>

using namespace std;

bool isBlockFull(const Block& block, int max_records) {
    if (block.num_records >= max_records) {
        return true;
    }
    return false;
}


void resetBlockNumRecords(Block& block) {
    block.num_records = 0;
}
