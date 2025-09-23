#pragma once
#include "common.hpp"
#include <vector>

class DiskManager;  // forward declaration

struct Page {
    uint32_t pid = 0;
    std::vector<char> bytes;
};

class Buffer {
public:
    explicit Buffer(DiskManager* dm);
    Page fetch(uint32_t pid);
    void write(const Page& p);
    uint32_t alloc();

private:
    DiskManager* dm_;
};
