#pragma once

#include "constants.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>

// Record reference for indexing
struct RecordRef
{
    std::uint32_t block_id;
    std::uint16_t record_offset;

    RecordRef() : block_id(0), record_offset(0)
    {
    }
    RecordRef(std::uint32_t bid, std::uint16_t offset) : block_id(bid), record_offset(offset)
    {
    }

    bool operator<(const RecordRef &other) const
    {
        if (block_id != other.block_id)
            return block_id < other.block_id;
        return record_offset < other.record_offset;
    }

    bool operator==(const RecordRef &other) const
    {
        return block_id == other.block_id && record_offset == other.record_offset;
    }
};

// B+ tree node structure
struct BPlusNode
{
    bool is_leaf;
    bool is_root;
    std::uint32_t node_id;
    std::vector<float> keys;

    // For internal nodes: child pointers
    std::vector<std::uint32_t> children;

    // For leaf nodes: record references (handles duplicates)
    std::vector<std::vector<RecordRef>> values;

    // For leaf nodes: next leaf pointer
    std::uint32_t next_leaf;

    // Parent tracking
    std::uint32_t parent_id;

    BPlusNode(bool leaf = false) : is_leaf(leaf), is_root(false), node_id(0), next_leaf(0), parent_id(0)
    {
    }
};

using NodePtr = std::shared_ptr<BPlusNode>;

class BPlusTree
{
  private:
    NodePtr root;
    int n; // Maximum keys per node
    std::uint32_t next_node_id;
    std::string index_filename;

    // Node storage
    std::unordered_map<std::uint32_t, NodePtr> nodes;

    // Statistics
    int total_nodes;
    int tree_height;

    // Helper functions
    NodePtr createNode(bool is_leaf);
    NodePtr findLeafNode(float key);
    NodePtr findParent(NodePtr child);

    void insertIntoLeaf(NodePtr leaf, float key, const RecordRef &record_ref);
    void insertIntoInternal(NodePtr internal, float key, std::uint32_t child_id);

    std::pair<NodePtr, float> splitLeafNode(NodePtr leaf);
    std::pair<NodePtr, float> splitInternalNode(NodePtr internal);

    void insertIntoParent(NodePtr left, float key, NodePtr right);
    void updateStatistics();
    int calculateHeight(NodePtr node);
    void countNodes(NodePtr node, int &count);

    // Disk I/O
    void saveNodeToDisk(std::ofstream &file, NodePtr node);
    NodePtr loadNodeFromDisk(std::ifstream &file);

  public:
    BPlusTree(int max_keys = 100, const std::string &filename = "bplus_tree.idx");
    ~BPlusTree() = default;

    // Core operations
    void insert(float key, const RecordRef &record_ref);
    void bulkLoad(std::vector<std::pair<float, RecordRef>> &data);
    std::vector<RecordRef> search(float key);

    // Range search operations for Task 3
    std::vector<RecordRef> searchRange(float min_key, float max_key);
    std::vector<RecordRef> searchGreaterThan(float key);

    // Search with statistics tracking
    std::pair<std::vector<RecordRef>, int> searchGreaterThanWithStats(float key);

    // Delete operations
    bool deleteKey(float key, const RecordRef &record_ref);
    int deleteGreaterThan(float key); // Returns number of records deleted

    // Statistics
    int getParameterN() const
    {
        return n;
    }
    int getTotalNodes() const
    {
        return total_nodes;
    }
    int getTreeLevels() const
    {
        return tree_height;
    }
    std::vector<float> getRootKeys() const;

    void printStatistics();

    // Disk operations
    void saveToDisk();
    void loadFromDisk();
};
