#pragma once

#include "record.h"
#include "utils.h"
#include <cstddef>
#include <string_view>

// Assume using modern filesystem which by default use 4k blocks
inline constexpr std::size_t BLOCK_SIZE = 4096;
inline constexpr std::size_t MAX_RECORDS_PER_BLOCK = BLOCK_SIZE / sizeof(Record);
inline constexpr std::size_t RECORD_SIZE = sizeof(Record);
inline constexpr std::size_t EPOCH_YEAR = 2000;
inline constexpr std::string_view DATA_FILE = "data/games.txt";
