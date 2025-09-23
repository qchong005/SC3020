#include "buffer.hpp"
#include "disk.hpp"

Buffer::Buffer(DiskManager* dm) : dm_(dm) {}

Page Buffer::fetch(uint32_t pid) {
    Page p;
    p.pid = pid;
    p.bytes.resize(PAGE_SIZE);
    dm_->read_page(pid, p.bytes.data());
    return p;
}

void Buffer::write(const Page& p) {
    dm_->write_page(p.pid, p.bytes.data());
}

uint32_t Buffer::alloc() {
    return dm_->allocate_page();
}
