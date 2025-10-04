#pragma once
#include "constants.h"
#include <cstring>
struct Block
{
    char data[BLOCK_SIZE];

    Block()
    {
        std::memset(data, 0, BLOCK_SIZE);
    }
};
