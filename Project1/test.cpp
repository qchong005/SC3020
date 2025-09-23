#include <iostream>
#include "heap_file.hpp"
#include "slotted_page.hpp"
#include "heap_file.hpp"
#include "disk.hpp"
using namespace std;

int main() {
    cout << "Test started\n";
    DiskManager dm("data/disk.bin");
    Buffer buf(&dm);
    HeapFile hf(&buf);

    Page p = buf.fetch(0);
    SlottedPageView sp(p.bytes.data());

    cout << "Page ID: " << sp.hdr().page_id 
              << ", slots: " << sp.hdr().slot_count << endl;

    // Try reading first 5 slots
    for (uint16_t i = 0; i < sp.hdr().slot_count && i < 5; i++) {
        auto rec = sp.read_fixed28(i);
        if (rec) {
            string s(rec->data(), rec->size());
            cout << "Slot " << i << ": " << s << endl;
        } else {
            cout << "Slot " << i << " is erased or invalid." << endl;
        }
    }

    return 0;


}