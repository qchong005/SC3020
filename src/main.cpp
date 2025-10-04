#include "bplus_tree.h"
#include "constants.h"
#include "disk.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <set>

void task3(Disk &disk);

void task1(const Disk &disk)
{
    std::cout << "=== Task 1 ===" << '\n';
    disk.printStats();
}

void task2(const Disk &disk)
{
    std::cout << "=== Task 2 ===" << '\n';
    std::cout << "Building B+ tree on FT_PCT_home attribute..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    BPlusTree bplus_tree(100, "ft_pct_home.idx");

    std::cout << "Building new B+ tree index..." << std::endl;

    // Get all FT_PCT_home values with their record references
    auto ft_pct_data = disk.getAllFTPctHomeValues();
    std::cout << "Retrieved " << ft_pct_data.size() << " records for indexing" << std::endl;

    // Build the B+ tree using bulk loading
    bplus_tree.bulkLoad(ft_pct_data);

    // Save B+ tree to disk
    bplus_tree.saveToDisk();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "B+ tree operation completed in " << duration << " ms" << std::endl;
    std::cout << std::endl;

    // Print statistics
    bplus_tree.printStatistics();

    std::cout << std::endl;
}

void task3(Disk &disk)
{
    std::cout << "\n=== Task 3: Delete records with FT_PCT_home > 0.9 ===" << std::endl;

    // Load existing B+ tree from disk
    BPlusTree bplus_tree(100, "ft_pct_home.idx");
    bplus_tree.loadFromDisk();

    std::cout << "\n--- B+ Tree Statistics BEFORE Deletion ---" << std::endl;
    bplus_tree.printStatistics();

    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "\nStep 1: Using B+ tree index to find records with FT_PCT_home > 0.9..." << std::endl;

    // Use B+ tree to find records with FT_PCT_home > 0.9 and count nodes accessed
    auto [record_refs, index_nodes_accessed] = bplus_tree.searchGreaterThanWithStats(0.9f);

    auto index_search_end = std::chrono::high_resolution_clock::now();
    auto index_time = std::chrono::duration_cast<std::chrono::microseconds>(index_search_end - start).count();

    std::cout << "Step 2: B+ tree found " << record_refs.size() << " records with FT_PCT_home > 0.9" << std::endl;
    std::cout << "Index nodes accessed: " << index_nodes_accessed << std::endl;
    std::cout << "Index search time: " << index_time << " microseconds" << std::endl;

    if (record_refs.empty())
    {
        std::cout << "No records found with FT_PCT_home > 0.9" << std::endl;
        return;
    }

    std::cout << "\nStep 3: Retrieving actual records from disk using RecordRef pointers..." << std::endl;

    // Count unique blocks accessed for statistics
    std::set<std::uint32_t> unique_blocks;
    for (const auto &ref : record_refs)
    {
        unique_blocks.insert(ref.block_id);
    }

    std::cout << "Number of data blocks to access: " << unique_blocks.size() << std::endl;

    // Retrieve actual records from disk (NOT from memory!)
    auto disk_read_start = std::chrono::high_resolution_clock::now();
    auto records = disk.getRecords(record_refs);
    auto disk_read_end = std::chrono::high_resolution_clock::now();
    auto disk_time = std::chrono::duration_cast<std::chrono::microseconds>(disk_read_end - disk_read_start).count();

    std::cout << "Disk retrieval time: " << disk_time << " microseconds" << std::endl;

    // Calculate statistics BEFORE deletion
    float total_ft_pct = 0.0f;
    int valid_records = 0;

    std::cout << "\nStep 4: Verifying retrieved records and calculating statistics..." << std::endl;
    std::cout << "Sample of records to be deleted:" << std::endl;

    for (size_t i = 0; i < std::min(size_t(5), records.size()); i++)
    {
        const auto &record = records[i];
        const auto &ref = record_refs[i];

        std::cout << "  Record " << (i + 1) << ": FT_PCT=" << record.ft_pct_home << ", PTS=" << (int)record.pts_home
                  << ", Location=[Block " << ref.block_id << ", Offset " << ref.record_offset << "]" << std::endl;
    }

    // Calculate full statistics
    for (const auto &record : records)
    {
        if (record.ft_pct_home > 0.9f)
        {
            total_ft_pct += record.ft_pct_home;
            valid_records++;
        }
    }

    // Step 5: PERFORM ACTUAL DELETION
    std::cout << "\nStep 5: Deleting records from disk and B+ tree index..." << std::endl;

    auto deletion_start = std::chrono::high_resolution_clock::now();

    // Delete from disk
    int disk_deleted = disk.deleteRecords(record_refs);
    std::cout << "Deleted " << disk_deleted << " records from disk" << std::endl;

    // Delete from B+ tree index
    int index_deleted = bplus_tree.deleteGreaterThan(0.9f);
    std::cout << "Deleted " << index_deleted << " entries from B+ tree index" << std::endl;

    auto deletion_end = std::chrono::high_resolution_clock::now();
    auto deletion_time = std::chrono::duration_cast<std::chrono::milliseconds>(deletion_end - deletion_start).count();

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - start).count();

    std::cout << "\n=== Task 3 Results ===" << std::endl;
    std::cout << "Number of index nodes accessed: " << index_nodes_accessed << std::endl;
    std::cout << "Number of data blocks accessed: " << unique_blocks.size() << std::endl;
    std::cout << "Number of games deleted: " << valid_records << std::endl;
    std::cout << "Average FT_PCT_home of deleted records: " << (valid_records > 0 ? total_ft_pct / valid_records : 0.0f)
              << std::endl;
    std::cout << "Running time of retrieval process: " << total_time << " ms" << std::endl;
    std::cout << "Running time of deletion process: " << deletion_time << " ms" << std::endl;

    // Compare with brute-force linear scan
    std::cout << "\n=== Brute-force Comparison ===" << std::endl;
    std::cout << "Linear scan would access: " << disk.getTtlBlks() << " data blocks" << std::endl;
    std::cout << "Linear scan estimated time: ~" << (disk.getTtlBlks() * 5) << " ms (estimated)" << std::endl;
    std::cout << "Assumption: 5 ms per block access and scan" << std::endl;
    std::cout << "Index speedup: ~" << (float)(disk.getTtlBlks() * 5) / std::max(1, (int)total_time) << "x faster"
              << std::endl;

    // Show updated B+ tree statistics
    std::cout << "\n--- B+ Tree Statistics AFTER Deletion ---" << std::endl;
    bplus_tree.printStatistics();

    // Save updated B+ tree to disk
    bplus_tree.saveToDisk();
    std::cout << "\nUpdated B+ tree index saved to disk." << std::endl;
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

    // Demonstrate index-based data retrieval
    BPlusTree demo_tree(100, "ft_pct_home.idx");
    demo_tree.loadFromDisk();

    // Task 3 demonstration
    task3(disk);

    return 0;
}
