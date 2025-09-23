#include "common.hpp"
#include "disk.hpp"
#include "buffer.hpp"
#include "heap_file.hpp"
#include <iostream>

size_t load_tsv_into_heap(const std::string& path, HeapFile& heap);

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::cerr << "Usage: dbms <games.txt> <disk.bin>\n";
            return 1;
        }

        std::string tsv = argv[1];
        std::string disk = argv[2];

        DiskManager dm(disk);
        Buffer buf(&dm);
        HeapFile heap(&buf);
        heap.init_if_empty();

        uint64_t t0 = now_ms();
        size_t rows = load_tsv_into_heap(tsv, heap);
        uint64_t t1 = now_ms();

        const size_t rec_sz = 28;
        const size_t slot_bytes = 5;
        const size_t page_hdr = 24;
        size_t recs_per_block = (PAGE_SIZE - page_hdr) / (slot_bytes + rec_sz);
        size_t blocks_needed = (rows + recs_per_block - 1) / recs_per_block;

        std::cout << "=== Task 1: Storage Stats ===\n";
        std::cout << "Record size (bytes): " << rec_sz << "\n";
        std::cout << "Number of records: " << rows << "\n";
        std::cout << "Records per block: " << recs_per_block << "\n";
        std::cout << "Number of data blocks: " << blocks_needed << "\n";
        std::cout << "Load time (ms): " << (t1 - t0) << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
