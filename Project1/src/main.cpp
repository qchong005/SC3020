#include <iostream>
#include <chrono>
#include "record.hpp"
#include "block.hpp"
#include "disk.hpp"
#include "loader.hpp"

using namespace std;

int main() {
    Disk disk("data/data.bin");
    Block block;

    auto start = chrono::high_resolution_clock::now();
    loadCSVData("data/games.txt", disk);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "NBA games data loaded to disk." << endl;
    cout << "Size of Record: " << sizeof(Record) << " bytes" << endl;
    cout << "Total No. of Records on Disk: " << disk.getTotalRecords() << endl;
    cout << "Max Records per Block: " << MAX_RECORDS_PER_BLOCK << endl;
    cout << "Total No. of Blocks on Disk: " << disk.getNumBlocks() << endl;
    cout << "Time to Load Data (ms): " << duration << " ms" << endl;

    return 0;
    
}