#pragma once
#include "common.hpp"
#include <cstring>

struct Slot { uint16_t off; uint16_t len; uint8_t flags; };

struct PageHeader {
    uint16_t slot_count;
    uint16_t free_start;
    uint32_t reserved;
    uint32_t page_id;
    uint32_t type;
    uint32_t reserved2;
};

class SlottedPageView {
public:
    explicit SlottedPageView(char* p) : p_(p) {}

    PageHeader& hdr() { return *reinterpret_cast<PageHeader*>(p_); }

    void init(uint32_t page_id, uint32_t type) {
        std::memset(p_, 0, PAGE_SIZE);
        hdr().slot_count = 0;
        hdr().free_start = PAGE_SIZE;
        hdr().page_id = page_id;
        hdr().type = type;
    }

    Slot* slot_at(uint16_t i) {
        return reinterpret_cast<Slot*>(
            p_ + sizeof(PageHeader) + i * sizeof(Slot));
    }

    int insert_fixed28(const char* rec28) {
        const uint16_t rec_len = 28;
        const uint16_t need = rec_len + sizeof(Slot);
        uint16_t slots_bytes = hdr().slot_count * sizeof(Slot);
        uint16_t free_bytes = hdr().free_start -
                              (sizeof(PageHeader) + slots_bytes);
        if (free_bytes < need) return -1;

        uint16_t new_slot = hdr().slot_count++;
        Slot* s = slot_at(new_slot);
        hdr().free_start -= rec_len;
        s->off = hdr().free_start; s->len = rec_len; s->flags = 0;
        std::memcpy(p_ + s->off, rec28, rec_len);
        return new_slot;
    }

    bool erase(uint16_t slot) {
        if (slot >= hdr().slot_count) return false;
        Slot* s = slot_at(slot);
        s->flags |= 0x1;
        return true;
    }

private:
    char* p_;
};
