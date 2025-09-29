#include "bplus_tree.h"
#include "constants.h"
#include "disk.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <tuple> //Task 3
#include <unordered_set> //Task 3


// Helper to group RecordRefs by their key (FT_PCT_home) for batch deletion
static std::vector<std::pair<float, std::vector<RecordRef>>>
groupRefsByKey(Disk& disk, const std::vector<RecordRef>& refs) {
    std::unordered_map<float, std::vector<RecordRef>> by_key;
    by_key.reserve(refs.size());
    for (const auto& r : refs) {
        float k = disk.getRecord(r).ft_pct_home;  // this must exist in Disk
        by_key[k].push_back(r);
    }
    std::vector<std::pair<float, std::vector<RecordRef>>> grouped;
    grouped.reserve(by_key.size());
    for (auto &kv : by_key) grouped.emplace_back(kv.first, std::move(kv.second));
    return grouped;
}

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

    // Create B+ tree with optimal order (n=400) or auto-calculate
    BPlusTree bplus_tree(0, "data/ft_pct_home.idx");  // 0 = auto-calculate optimal order

    // Try to load existing index first
    bplus_tree.loadFromDisk();

    // If no existing index, build from scratch
    if (bplus_tree.getTotalNodes() == 0) {
        std::cout << "Building new B+ tree index..." << std::endl;

        // Get all FT_PCT_home values with their record references
        auto ft_pct_data = disk.getAllFTPctHomeValues();
        std::cout << "Retrieved " << ft_pct_data.size() << " records for indexing" << std::endl;

        // Build the B+ tree using bulk loading
        // bplus_tree.bulkLoad(ft_pct_data);
        bplus_tree.bulkLoadBottomUp(ft_pct_data);

        // Save B+ tree to disk
        bplus_tree.saveToDisk();
    } else {
        std::cout << "Loaded existing B+ tree index from disk" << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "B+ tree operation completed in " << duration << " ms" << std::endl;

    // Print statistics
    bplus_tree.printStatistics();

    // Verify RecordRef calculations
    std::cout << "\n=== Testing RecordRef Accuracy ===" << std::endl;
    std::cout << "Block size: " << BLOCK_SIZE << " bytes" << std::endl;
    std::cout << "Record size: " << RECORD_SIZE << " bytes" << std::endl;
    std::cout << "Records per block: " << MAX_RECORDS_PER_BLOCK << std::endl;

    // Test 1: Check first few records directly
    std::cout << "\nTest 1: First 3 records from memory vs RecordRef:" << std::endl;
    auto ft_pct_data = disk.getAllFTPctHomeValues();
    for (int i = 0; i < 3 && i < ft_pct_data.size(); i++) {
        float expected_ft_pct = ft_pct_data[i].first;
        RecordRef ref = ft_pct_data[i].second;
        Record retrieved = disk.getRecord(ref);

        std::cout << "  Record " << i << ": "
                  << "Expected FT_PCT=" << expected_ft_pct
                  << ", Retrieved FT_PCT=" << retrieved.ft_pct_home
                  << ", Block=" << ref.block_id
                  << ", Offset=" << ref.record_offset
                  << (expected_ft_pct == retrieved.ft_pct_home ? " ✓" : " ✗") << std::endl;
    }

    // Test 2: Test specific record in different blocks
    std::cout << "\nTest 2: Records in different blocks:" << std::endl;
    std::vector<size_t> test_indices = {0, 185, 370, 1000};  // Records in blocks 0, 1, 2, and 5
    for (size_t idx : test_indices) {
        if (idx < ft_pct_data.size()) {
            float expected_ft_pct = ft_pct_data[idx].first;
            RecordRef ref = ft_pct_data[idx].second;
            Record retrieved = disk.getRecord(ref);

            std::cout << "  Index " << idx << ": "
                      << "Expected FT_PCT=" << expected_ft_pct
                      << ", Retrieved FT_PCT=" << retrieved.ft_pct_home
                      << ", Block=" << ref.block_id
                      << ", Offset=" << ref.record_offset
                      << (expected_ft_pct == retrieved.ft_pct_home ? " ✓" : " ✗") << std::endl;
        }
    }

    // Test 3: B+ tree search and retrieval
    std::cout << "\nTest 3: B+ tree search and RecordRef retrieval:" << std::endl;
    auto root_keys = bplus_tree.getRootKeys();
    if (!root_keys.empty()) {
        float test_key = root_keys[0];
        auto search_results = bplus_tree.search(test_key);
        std::cout << "Found " << search_results.size() << " records with FT_PCT_home = " << test_key << std::endl;

        if (!search_results.empty()) {
            auto retrieved_records = disk.getRecords(search_results);
            bool all_correct = true;
            for (size_t i = 0; i < retrieved_records.size(); i++) {
                if (retrieved_records[i].ft_pct_home != test_key) {
                    all_correct = false;
                    break;
                }
            }
            std::cout << "All retrieved records have correct FT_PCT_home: " << (all_correct ? "✓" : "✗") << std::endl;
        }
    }
    std::cout << std::endl;
}

void demonstrateIndexRetrieval(const Disk& disk, BPlusTree& bplus_tree)
{
    std::cout << "\n=== Index-Based Data Retrieval Example ===" << std::endl;

    // Example 1: Single value lookup using B+ tree index
    std::cout << "\n--- Example 1: Find all games with FT_PCT_home = 0.75 ---" << std::endl;
    float target_ft_pct = 0.75f;

    std::cout << "Step 1: Search B+ tree index for FT_PCT_home = " << target_ft_pct << std::endl;
    auto record_refs = bplus_tree.search(target_ft_pct);
    std::cout << "Step 2: Index returned " << record_refs.size() << " RecordRef pointers" << std::endl;

    if (!record_refs.empty()) {
        std::cout << "Step 3: Use RecordRef pointers to retrieve actual records from disk:" << std::endl;
        auto records = disk.getRecords(record_refs);

        for (size_t i = 0; i < records.size(); i++) {
            const auto& record = records[i];
            const auto& ref = record_refs[i];
            std::cout << "  === Complete Record " << i+1 << " ===" << std::endl;
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
    } else {
        std::cout << "Step 3: No records found with FT_PCT_home = " << target_ft_pct << std::endl;
    }

    // Example 2: Try with a value that exists in the data
    std::cout << "\n--- Example 2: Find games with existing FT_PCT_home value ---" << std::endl;
    auto root_keys = bplus_tree.getRootKeys();
    if (!root_keys.empty()) {
        float existing_value = root_keys[0]; // Use the split key from root
        std::cout << "Step 1: Search B+ tree index for FT_PCT_home = " << existing_value << std::endl;
        auto refs = bplus_tree.search(existing_value);
        std::cout << "Step 2: Index found " << refs.size() << " matching records" << std::endl;

        if (!refs.empty()) {
            std::cout << "Step 3: Retrieve records from data blocks:" << std::endl;
            auto retrieved_records = disk.getRecords(refs);

            for (size_t i = 0; i < retrieved_records.size(); i++) {
                const auto& record = retrieved_records[i];
                const auto& ref = refs[i];
                std::cout << "  === NBA Game Record ===" << std::endl;
                std::cout << "    Storage Location: Block " << ref.block_id << ", Position " << ref.record_offset << std::endl;
                std::cout << "    Game Date: " << record.game_date_est << std::endl;
                std::cout << "    Home Team ID: " << record.team_ID_home << std::endl;
                std::cout << "    Home Team Stats:" << std::endl;
                std::cout << "      Points: " << (int)record.pts_home << std::endl;
                std::cout << "      Field Goal %: " << record.fg_pct_home << std::endl;
                std::cout << "      Free Throw %: " << record.ft_pct_home << " ⭐ [SEARCH KEY]" << std::endl;
                std::cout << "      3-Point %: " << record.fg3_pct_home << std::endl;
                std::cout << "      Assists: " << (int)record.ast_home << std::endl;
                std::cout << "      Rebounds: " << (int)record.reb_home << std::endl;
                std::cout << "      Game Result: " << ((int)record.home_team_wins ? "HOME WIN" : "HOME LOSS") << std::endl;
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
    std::cout << "=== Task 3 ===\n";
    const float THRESH = 0.9f;

    // Duplicate files for safe deletion operations (all operations on duplicates)
    const std::string duplicate_path = "data/task3_data.db";
    const std::string src_idx = "data/ft_pct_home.idx";    
    const std::string dup_idx = "data/ft_pct_home_task3.idx";
    Disk duplicate_disk = disk.createDuplicate(duplicate_path);
    duplicateFile(src_idx, dup_idx);

    // Load existing index from disk 
    BPlusTree bplus_tree_duplicate(0, "data/ft_pct_home_task3.idx");
    bplus_tree_duplicate.loadFromDisk();
    bplus_tree_duplicate.buildParentMaps(); // ensure parent maps are built

    // Index-based range search for FT_PCT_home > THRESH
    int idx_nodes = 0;
    double avg_from_index = 0.0;
    int range_count = 0;
    int unique_keys = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto refs = bplus_tree_duplicate.rangeSearch(THRESH, idx_nodes, &avg_from_index, &range_count, &unique_keys);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto idx_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    // Brute-force range search for comparison
    auto bf_start = std::chrono::high_resolution_clock::now();
    auto [bf_matches, bf_blocks] = duplicate_disk.bruteForceRangeSearch(THRESH);
    auto bf_end = std::chrono::high_resolution_clock::now();
    auto bf_ms = std::chrono::duration_cast<std::chrono::milliseconds>(bf_end - bf_start).count();


    // BEFORE touching the DB, group refs by their actual key (for index deletion) 
    auto grouped = groupRefsByKey(duplicate_disk, refs);

    auto t_del_start = std::chrono::high_resolution_clock::now();
    auto [deleted_count, data_blocks_accessed, avg_ft_deleted] = duplicate_disk.deleteRecordsByRefs(refs);
    auto t_del_end = std::chrono::high_resolution_clock::now();
    auto del_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_del_end - t_del_start).count();

    // Delete from the DUPLICATE INDEX in-place (balanced; NO rebuild) 
    std::size_t removed_in_index = bplus_tree_duplicate.deleteByRefs(grouped);
    (void)removed_in_index; // optional to print
    bplus_tree_duplicate.saveToDisk();       // persists to data/ft_pct_home_task3.idx



    // Apply deletions to duplicate DB file using RecordRefs from index search
    //auto t3 = std::chrono::high_resolution_clock::now();
    //auto [deleted_count, data_blocks_accessed, avg_ft_deleted] = duplicate_disk.deleteRecordsByRefs(refs);
    //auto t4 = std::chrono::high_resolution_clock::now();
    //auto del_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    /* Rebuild B+ tree index from scratch on the updated duplicate DB (Wrong implementation)
    const std::string new_index_path = "data/ft_pct_home_task3.idx";
    BPlusTree bplus_tree_new(0, new_index_path);      
    auto rebuilt_pairs = duplicate_disk.getAllFTPctHomeValues(); 
    bplus_tree_duplicate.rebuildTreeFromData(rebuilt_pairs);           
    bplus_tree_duplicate.saveToDisk();  */


    
    // Report results
    std::cout << "\n=== Task 3 Results ===\n";
    std::cout << "Filter: FT_PCT_home > " << THRESH << "\n";

    std::cout << "Index-based Range Search\n";
    std::cout << "Index nodes accessed: " << idx_nodes << "\n";
    std::cout << "Records returned by index range search: " << range_count << "\n";
    std::cout << "Unique keys returned: " << unique_keys << "\n";
    std::cout << "Running time (retrieval): " << idx_ms << " ms\n";
    std::cout << "Average FT_PCT_home of returned records (from index keys): " << avg_from_index << "\n\n";

    std::cout << "Deletion\n";
    std::cout << "Data blocks accessed (during deletions): " << data_blocks_accessed << "\n";
    std::cout << "Games deleted: " << deleted_count << "\n";
    std::cout << "Average FT_PCT_home of deleted records (from data): " << avg_ft_deleted << "\n";
    std::cout << "Deletion/Rewrite running time: " << del_ms << " ms\n\n";

    std::cout << "Brute-force Range Search\n";
    std::cout << "Data blocks that would be scanned: " << bf_blocks << "\n";
    std::cout << "Records found: " << bf_matches << "\n";
    std::cout << "Running time (brute-force scan): " << bf_ms << " ms\n\n";


    // Print updated data.db stats after deletions
    std::cout << "\nPost-Deletion Database Stats:\n";
    duplicate_disk.printStats(); 

    // Print updated B+ tree stats after deletions
    std::cout << "Updated B+ Tree Statistics after deletion:\n";
    bplus_tree_duplicate.printStatistics();
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
    //BPlusTree demo_tree(25, "ft_pct_home.idx");
    //demo_tree.loadFromDisk();
    //demonstrateIndexRetrieval(disk, demo_tree);

    task3(disk);

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
