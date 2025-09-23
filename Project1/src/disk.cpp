#include "disk.hpp"
#include <vector>

DiskManager::DiskManager(const std::string& path) : path_(path) {
    f_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    if (!f_.is_open()) {
        f_.clear();
        f_.open(path_, std::ios::out | std::ios::binary);
        f_.close();
        f_.open(path_, std::ios::in | std::ios::out | std::ios::binary);
    }
    f_.seekg(0, std::ios::end);
    std::streamoff bytes = f_.tellg();
    pages_ = static_cast<uint32_t>(bytes / PAGE_SIZE);
}

DiskManager::~DiskManager() {
    f_.flush();
    f_.close();
}

uint32_t DiskManager::allocate_page() {
    std::vector<char> zero(PAGE_SIZE, 0);
    f_.seekp(0, std::ios::end);
    f_.write(zero.data(), PAGE_SIZE);
    f_.flush();
    return pages_++;
}

void DiskManager::read_page(uint32_t pid, char* out) {
    f_.seekg(static_cast<std::streamoff>(pid) * PAGE_SIZE, std::ios::beg);
    f_.read(out, PAGE_SIZE);
    if (!f_) die("Disk read error");
}

void DiskManager::write_page(uint32_t pid, const char* in) {
    f_.seekp(static_cast<std::streamoff>(pid) * PAGE_SIZE, std::ios::beg);
    f_.write(in, PAGE_SIZE);
    if (!f_) die("Disk write error");
    f_.flush();
}
