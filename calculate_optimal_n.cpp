#include <iostream>
#include <cmath>
#include <cstdint>

int main() {
    const int BLOCK_SIZE = 4096;

    std::cout << "=== B+ Tree Parameter Calculation ===" << std::endl;
    std::cout << "Block size: " << BLOCK_SIZE << " bytes" << std::endl;

    // Node overhead (metadata)
    int node_overhead = sizeof(int) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(bool); // ~11 bytes
    node_overhead += 16; // Additional overhead for vectors, padding, etc.

    std::cout << "Estimated node overhead: " << node_overhead << " bytes" << std::endl;

    // For internal nodes: n keys + (n+1) pointers
    // Each key: 4 bytes (float)
    // Each pointer: 4 bytes (uint32_t)
    int internal_node_calc = node_overhead;
    std::cout << "\n--- Internal Node Calculation ---" << std::endl;

    int max_n_internal = 0;
    for (int n = 1; n <= 1000; n++) {
        int size = node_overhead + (n * 4) + ((n + 1) * 4); // keys + pointers
        if (size <= BLOCK_SIZE) {
            max_n_internal = n;
        } else {
            break;
        }
    }

    std::cout << "Max n for internal nodes: " << max_n_internal << std::endl;
    std::cout << "Internal node size with max n: " << (node_overhead + (max_n_internal * 4) + ((max_n_internal + 1) * 4)) << " bytes" << std::endl;

    // For leaf nodes: n keys + n value_vectors + next_pointer
    // Each key: 4 bytes (float)
    // Each RecordRef: 6 bytes (uint32_t + uint16_t)
    // Average records per key: 26651/26266 ≈ 1.01
    // But we need to account for worst case where some keys have many duplicates
    std::cout << "\n--- Leaf Node Calculation ---" << std::endl;

    double avg_duplicates = 26651.0 / 26266.0;
    std::cout << "Average duplicates per key: " << avg_duplicates << std::endl;

    // Conservative estimate: assume average 2 duplicates per key for safety
    double conservative_duplicates = 2.0;
    std::cout << "Conservative duplicates per key: " << conservative_duplicates << std::endl;

    int max_n_leaf = 0;
    for (int n = 1; n <= 1000; n++) {
        int size = node_overhead + (n * 4) + (n * 6 * conservative_duplicates) + 4; // keys + values + next_ptr
        if (size <= BLOCK_SIZE) {
            max_n_leaf = n;
        } else {
            break;
        }
    }

    std::cout << "Max n for leaf nodes (conservative): " << max_n_leaf << std::endl;
    std::cout << "Leaf node size with max n: " << (int)(node_overhead + (max_n_leaf * 4) + (max_n_leaf * 6 * conservative_duplicates) + 4) << " bytes" << std::endl;

    // Choose the smaller of the two for uniform n
    int optimal_n = std::min(max_n_internal, max_n_leaf);
    std::cout << "\n=== RECOMMENDATION ===" << std::endl;
    std::cout << "Optimal n (uniform): " << optimal_n << std::endl;

    // Calculate expected tree height
    int unique_keys = 26266;
    int tree_height = (int)std::ceil(std::log(unique_keys) / std::log(optimal_n));
    std::cout << "Expected tree height with n=" << optimal_n << ": " << tree_height << " levels" << std::endl;

    // Calculate expected number of leaf nodes
    int leaf_nodes = (int)std::ceil((double)unique_keys / optimal_n);
    std::cout << "Expected leaf nodes: " << leaf_nodes << std::endl;

    // More practical values
    std::cout << "\n=== PRACTICAL RECOMMENDATIONS ===" << std::endl;
    std::cout << "For better balance, consider these values:" << std::endl;

    for (int test_n : {50, 100, 200, 300}) {
        if (test_n <= optimal_n) {
            int height = (int)std::ceil(std::log(unique_keys) / std::log(test_n));
            int leaves = (int)std::ceil((double)unique_keys / test_n);
            std::cout << "n=" << test_n << ": height=" << height << ", leaf_nodes=" << leaves << std::endl;
        }
    }

    return 0;
}