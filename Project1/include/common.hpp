#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <chrono>

static constexpr uint32_t PAGE_SIZE = 4096;

inline uint64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

inline void die(const char* msg) {
    throw std::runtime_error(msg);
}
