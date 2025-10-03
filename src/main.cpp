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

    // Try to load existing index first
    // bplus_tree.loadFromDisk();

    // If no existing index, build from scratch
    // if (bplus_tree.getTotalNodes() == 0)

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

    // Print statistics
    bplus_tree.printStatistics();

    // // Verify RecordRef calculations
    // std::cout << "\n=== Testing RecordRef Accuracy ===" << std::endl;
    // std::cout << "Block size: " << BLOCK_SIZE << " bytes" << std::endl;
    // std::cout << "Record size: " << RECORD_SIZE << " bytes" << std::endl;
    // std::cout << "Records per block: " << MAX_RECORDS_PER_BLOCK << std::endl;
    //
    // // Test 1: Check first few records directly
    // std::cout << "\nTest 1: First 3 records from memory vs RecordRef:" << std::endl;
    // auto ft_pct_data = disk.getAllFTPctHomeValues();
    // for (int i = 0; i < 3 && i < ft_pct_data.size(); i++)
    // {
    //     float expected_ft_pct = ft_pct_data[i].first;
    //     RecordRef ref = ft_pct_data[i].second;
    //     Record retrieved = disk.getRecord(ref);
    //
    //     std::cout << "  Record " << i << ": "
    //               << "Expected FT_PCT=" << expected_ft_pct << ", Retrieved FT_PCT=" << retrieved.ft_pct_home
    //               << ", Block=" << ref.block_id << ", Offset=" << ref.record_offset
    //               << (expected_ft_pct == retrieved.ft_pct_home ? " ✓" : " ✗") << std::endl;
    // }
    //
    // // Test 2: Test specific record in different blocks
    // std::cout << "\nTest 2: Records in different blocks:" << std::endl;
    // std::vector<size_t> test_indices = {0, 185, 370, 1000}; // Records in blocks 0, 1, 2, and 5
    // for (size_t idx : test_indices)
    // {
    //     if (idx < ft_pct_data.size())
    //     {
    //         float expected_ft_pct = ft_pct_data[idx].first;
    //         RecordRef ref = ft_pct_data[idx].second;
    //         Record retrieved = disk.getRecord(ref);
    //
    //         std::cout << "  Index " << idx << ": "
    //                   << "Expected FT_PCT=" << expected_ft_pct << ", Retrieved FT_PCT=" << retrieved.ft_pct_home
    //                   << ", Block=" << ref.block_id << ", Offset=" << ref.record_offset
    //                   << (expected_ft_pct == retrieved.ft_pct_home ? " ✓" : " ✗") << std::endl;
    //     }
    // }
    //
    // // Test 3: B+ tree search and retrieval
    // std::cout << "\nTest 3: B+ tree search and RecordRef retrieval:" << std::endl;
    // auto root_keys = bplus_tree.getRootKeys();
    // if (!root_keys.empty())
    // {
    //     float test_key = root_keys[0];
    //     auto search_results = bplus_tree.search(test_key);
    //     std::cout << "Found " << search_results.size() << " records with FT_PCT_home = " << test_key << std::endl;
    //
    //     if (!search_results.empty())
    //     {
    //         auto retrieved_records = disk.getRecords(search_results);
    //         bool all_correct = true;
    //         for (size_t i = 0; i < retrieved_records.size(); i++)
    //         {
    //             if (retrieved_records[i].ft_pct_home != test_key)
    //             {
    //                 all_correct = false;
    //                 break;
    //             }
    //         }
    //         std::cout << "All retrieved records have correct FT_PCT_home: " << (all_correct ? "✓" : "✗") <<
    //         std::endl;
    //     }
    // }
    std::cout << std::endl;
}

void demonstrateIndexRetrieval(const Disk &disk, BPlusTree &bplus_tree)
{
    std::cout << "\n=== Index-Based Data Retrieval Example ===" << std::endl;

    // Example 1: Single value lookup using B+ tree index
    std::cout << "\n--- Example 1: Find all games with FT_PCT_home = 0.75 ---" << std::endl;
    float target_ft_pct = 0.75f;

    std::cout << "Step 1: Search B+ tree index for FT_PCT_home = " << target_ft_pct << std::endl;
    auto record_refs = bplus_tree.search(target_ft_pct);
    std::cout << "Step 2: Index returned " << record_refs.size() << " RecordRef pointers" << std::endl;

    if (!record_refs.empty())
    {
        std::cout << "Step 3: Use RecordRef pointers to retrieve actual records from disk:" << std::endl;
        auto records = disk.getRecords(record_refs);

        for (size_t i = 0; i < records.size(); i++)
        {
            const auto &record = records[i];
            const auto &ref = record_refs[i];
            std::cout << "  === Complete Record " << i + 1 << " ===" << std::endl;
            std::cout << "    Location: Block " << ref.block_id << ", Offset " << ref.record_offset << std::endl;
            std::cout << "    Game Date: " << record.game_date_est << std::endl;
            std::cout << "    Team ID (Home): " << record.team_ID_home << std::endl;
            std::cout << "    Points (Home): " << (int)record.pts_home << std::endl;
            std::cout << "    FG% (Home): " << record.fg_pct_home << std::endl;
            std::cout << "    FT% (Home): " << record.ft_pct_home << " [INDEXED FIELD]" << std::endl;
            std::cout << "    3P% (Home): " << record.fg3_pct_home << std::endl;
            std::cout << "    Assists (Home): " << (int)record.ast_home << std::endl;
            std::cout << "    Rebounds (Home): " << (int)record.reb_home << std::endl;
            std::cout << "    Home Team Won: " << ((int)record.home_team_wins ? "Yes" : "No") << std::endl;
            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << "Step 3: No records found with FT_PCT_home = " << target_ft_pct << std::endl;
    }

    // Example 2: Try with a value that exists in the data
    std::cout << "\n--- Example 2: Find games with existing FT_PCT_home value ---" << std::endl;
    auto root_keys = bplus_tree.getRootKeys();
    if (!root_keys.empty())
    {
        float existing_value = root_keys[0]; // Use the split key from root
        std::cout << "Step 1: Search B+ tree index for FT_PCT_home = " << existing_value << std::endl;
        auto refs = bplus_tree.search(existing_value);
        std::cout << "Step 2: Index found " << refs.size() << " matching records" << std::endl;

        if (!refs.empty())
        {
            std::cout << "Step 3: Retrieve records from data blocks:" << std::endl;
            auto retrieved_records = disk.getRecords(refs);

            for (size_t i = 0; i < retrieved_records.size(); i++)
            {
                const auto &record = retrieved_records[i];
                const auto &ref = refs[i];
                std::cout << "  === NBA Game Record ===" << std::endl;
                std::cout << "    Storage Location: Block " << ref.block_id << ", Position " << ref.record_offset
                          << std::endl;
                std::cout << "    Game Date: " << record.game_date_est << std::endl;
                std::cout << "    Home Team ID: " << record.team_ID_home << std::endl;
                std::cout << "    Home Team Stats:" << std::endl;
                std::cout << "      Points: " << (int)record.pts_home << std::endl;
                std::cout << "      Field Goal %: " << record.fg_pct_home << std::endl;
                std::cout << "      Free Throw %: " << record.ft_pct_home << " ⭐ [SEARCH KEY]" << std::endl;
                std::cout << "      3-Point %: " << record.fg3_pct_home << std::endl;
                std::cout << "      Assists: " << (int)record.ast_home << std::endl;
                std::cout << "      Rebounds: " << (int)record.reb_home << std::endl;
                std::cout << "      Game Result: " << ((int)record.home_team_wins ? "HOME WIN" : "HOME LOSS")
                          << std::endl;
                std::cout << std::endl;
            }
        }
    }

    // Example 3: Show the efficiency gain
    std::cout << "\n--- Example 3: Efficiency Comparison ---" << std::endl;
    std::cout << "Without Index (Linear Scan):" << std::endl;
    std::cout << "  - Must scan all " << disk.getTtlRecs() << " records" << std::endl;
    std::cout << "  - Must access all " << disk.getTtlBlks() << " data blocks" << std::endl;
    std::cout << "  - Time complexity: O(n)" << std::endl;

    std::cout << "With B+ Tree Index:" << std::endl;
    std::cout << "  - Tree height: " << bplus_tree.getTreeLevels() << " levels" << std::endl;
    std::cout << "  - Index nodes accessed: ~" << bplus_tree.getTreeLevels() << " nodes" << std::endl;
    std::cout << "  - Data blocks accessed: Only blocks containing matching records" << std::endl;
    std::cout << "  - Time complexity: O(log n)" << std::endl;

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
    demonstrateIndexRetrieval(disk, demo_tree);

    // Task 3 demonstration
    task3(disk);

    return 0;
}
