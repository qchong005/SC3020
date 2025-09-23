#pragma once
#include "common.hpp"
#include <fstream>
#include <string>

class DiskManager {
public:
    explicit DiskManager(const std::string& path);
    ~DiskManager();

    uint32_t allocate_page();
    void read_page(uint32_t pid, char* out);
    void write_page(uint32_t pid, const char* in);

    uint32_t size_in_pages() const { return pages_; }

private:
    std::string path_;
    std::fstream f_;
    uint32_t pages_ = 0;
};
