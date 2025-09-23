#pragma once
#include "buffer.hpp"
#include "slotted_page.hpp"

struct RID { uint32_t page_id; uint16_t slot; };

class HeapFile {
public:
    explicit HeapFile(Buffer* buf) : buf_(buf) {}

    void init_if_empty() {
        if (!initialized_) {
            first_page_ = buf_->alloc();
            Page p = {first_page_};
            p.bytes.resize(PAGE_SIZE);
            SlottedPageView sp(p.bytes.data());
            sp.init(first_page_, 1);
            buf_->write(p);
            tail_page_ = first_page_;
            initialized_ = true;
        }
    }

    RID insert(const char* rec28) {
        init_if_empty();
        Page p = buf_->fetch(tail_page_);
        SlottedPageView sp(p.bytes.data());
        int slot = sp.insert_fixed28(rec28);
        if (slot >= 0) { buf_->write(p); return RID{tail_page_, (uint16_t)slot}; }
        uint32_t np = buf_->alloc();
        Page npg = {np}; npg.bytes.resize(PAGE_SIZE);
        SlottedPageView sp2(npg.bytes.data()); sp2.init(np, 1);
        int s2 = sp2.insert_fixed28(rec28);
        buf_->write(npg);
        tail_page_ = np;
        return RID{np, (uint16_t)s2};
    }

    bool erase(const RID& rid) {
        Page p = buf_->fetch(rid.page_id);
        SlottedPageView sp(p.bytes.data());
        bool ok = sp.erase(rid.slot);
        buf_->write(p);
        return ok;
    }

private:
    Buffer* buf_;
    bool initialized_ = false;
    uint32_t first_page_ = 0, tail_page_ = 0;
};
