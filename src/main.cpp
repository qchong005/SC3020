#include "constants.h"
#include "disk.h"
#include "utils.h"
#include "bplus_tree.h"
#include <chrono>
#include <iostream>

using namespace std;

void task1(const Disk &disk)
{
    std::cout << "=== Task 1 ===" << '\n';
    std::cout << "Size of Record: " << sizeof(Record) << " bytes" << endl;
    std::cout << "Total No of Records: " << disk.getTtlRecs() << endl;
    std::cout << "Total No of Blocks: " << disk.getTtlBlks() << endl;
    std::cout << endl;
}

void task2(const Disk &disk)
{
    std::cout << "=== Task 2 ===" << '\n';
    std::cout << "Building B+ tree on FT_PCT_home attribute..." << endl;

    auto start = chrono::high_resolution_clock::now();

    // Create B+ tree with order 4 (can be adjusted)
    BPlusTree bplus_tree(4, "ft_pct_home.idx");

    // Get all FT_PCT_home values with their record references
    auto ft_pct_data = disk.getAllFTPctHomeValues();

    // Build the B+ tree using bulk loading
    bplus_tree.bulkLoad(ft_pct_data);

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    std::cout << "B+ tree construction completed in " << duration << " ms" << endl;

    // Save B+ tree to disk
    bplus_tree.saveToDisk();

    // Print statistics
    bplus_tree.printStatistics();
    std::cout << endl;
}

int main()
{
    Disk disk("data/data.db");
    std::cout << "Creating database from " << DATA_FILE << '\n';
    if (!disk.loadData())
    {
        return 1;
    }

    task1(disk);
    task2(disk);

    // Block block;
    //
    // auto start = chrono::high_resolution_clock::now();
    // loadCSVData("data/games.txt", disk);
    // auto end = chrono::high_resolution_clock::now();
    // auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    //
    // cout << "NBA games data loaded to disk." << endl;
    // cout << "Size of Record: " << sizeof(Record) << " bytes" << endl;
    // cout << "Total No. of Records on Disk: " << disk.getTotalRecords() << endl;
    // cout << "Max Records per Block: " << MAX_RECORDS_PER_BLOCK << endl;
    // cout << "Total No. of Blocks on Disk: " << disk.getNumBlocks() << endl;
    // cout << "Time to Load Data (ms): " << duration << " ms" << endl;

    return 0;
}
