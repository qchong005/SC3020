#pragma once

#include "constants.h"
#include <cstdint>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>

// Record reference for indexing (points to location in main database)
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

    // Comparison operators for sorting
    bool operator<(const RecordRef &other) const
    {
        if (block_id != other.block_id)
        {
            return block_id < other.block_id;
        }
        return record_offset < other.record_offset;
    }

    bool operator==(const RecordRef &other) const
    {
        return block_id == other.block_id && record_offset == other.record_offset;
    }
};

// B+ tree node types
enum class NodeType
{
    INTERNAL,
    LEAF
};

// B+ tree node structure
struct BPlusTreeNode
{
    NodeType type;
    std::uint32_t node_id;
    std::uint16_t num_keys;
    bool is_root;

    // Keys array - FT_PCT_home values
    std::vector<float> keys;

    // For internal nodes: pointers to child nodes
    std::vector<std::uint32_t> children;

    // For leaf nodes: record references and values for duplicate keys
    std::vector<std::vector<RecordRef>> values;

    // For leaf nodes: pointer to next leaf (linked list)
    std::uint32_t next_leaf;

    BPlusTreeNode(NodeType t, std::uint32_t id) : type(t), node_id(id), num_keys(0), is_root(false), next_leaf(0)
    {
    }
};

using NodePtr = std::shared_ptr<BPlusTreeNode>;

// B+ tree class
class BPlusTree
{
  private:
    NodePtr root;
    int n; // Parameter n (max keys per node)
    std::string index_filename;
    std::uint32_t next_node_id;

    // Statistics
    int total_nodes;
    int tree_levels;

    // Node storage for in-memory B+ tree
    std::unordered_map<std::uint32_t, NodePtr> nodes;

    // Insertion Node functions
    NodePtr createNode(NodeType type);
    NodePtr findLeafNode(float key);
    void insertIntoLeaf(NodePtr leaf, float key, const RecordRef &record_ref);
    void insertIntoParent(NodePtr left, float key, NodePtr right);
    NodePtr splitLeafNode(NodePtr leaf);
    NodePtr splitInternalNode(NodePtr node, float &promote_key);

    // Deletion Node Functions
    // --- Parent maps (child -> parent, and child's index in parent) ---
    std::unordered_map<std::uint32_t, std::uint32_t> parent_of;
    std::unordered_map<std::uint32_t, int>           child_idx;

    inline void setParent(std::uint32_t parent, std::uint32_t child, int ix) {
        parent_of[child] = parent; child_idx[child] = ix;
    }

    float firstKeyOfSubtree(NodePtr node) const; // walk leftmost to leaf and return first key
    
    NodePtr makeLeafFromRun(const std::vector<float>& keys_run, const std::vector<std::vector<RecordRef>>& buckets_run);

    // --- Capacities (tune leafMaxKeys() to your project’s leaf capacity) ---
    int leafMaxKeys() const { return 8; } // if you compute this, call your calculator instead
    int leafMinKeys() const { return std::max(1, (leafMaxKeys()+1)/2); }
    int internalMinKeys() const { return std::max(1, (n+1)/2); }

    // --- Deletion helpers ---
    void updateParentSeparatorFor(NodePtr leaf);
    void rebalanceAfterDeleteLeaf(NodePtr leaf);
    void rebalanceInternal(NodePtr internal);

    // Tree statistics calculation
    void calculateStatistics();
    int calculateLevels(NodePtr node, int current_level = 1);
    void countNodes(NodePtr node, int &count);

    // Disk I/O helpers
    void saveNodeToDisk(std::ofstream &file, NodePtr node);
    void loadNodeFromDisk(std::ifstream &file, uint32_t node_id);
    
  public:
    BPlusTree(int order = 4, const std::string &filename = "bplus_tree.idx");
    ~BPlusTree() = default;

    // Core operations
    void insert(float key, const RecordRef &record_ref);
    void bulkLoad(std::vector<std::pair<float, RecordRef>> &data);
    void bulkLoadBottomUp(std::vector<std::pair<float, RecordRef>>& data); //make sure that leafnodes are packed
    std::vector<RecordRef> search(float key);
    std::vector<RecordRef> rangeSearch(float threshold,int &index_nodes_accessed, double *avg_key = nullptr, int *out_count = nullptr, int *out_unique_keys = nullptr);
    void rebuildTreeFromData(std::vector<std::pair<float, RecordRef>> &data);
    bool findParent(uint32_t child_id, NodePtr& out_parent, int& out_child_idx);

    // Delete operations
    bool deleteRecord(float key, const RecordRef& ref); // Delete exactly one occurrence of (key, ref). Drops key if bucket becomes empty.
    bool deleteKeyCompletely(float key); // Remove an entire key (all refs for that key). (Optional convenience)
    std::size_t deleteByRefs(const std::vector<std::pair<float, std::vector<RecordRef>>>& refs_by_key); // Batch delete grouped by key; returns count of RecordRef removed.
    void buildParentMaps(); // Parent map rebuild (call after load/build or big structural edits) 

    // Statistics accessors
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
        return tree_levels;
    }
    std::vector<float> getRootKeys() const;

    // Disk operations
    void saveToDisk();
    void loadFromDisk();

    // Utility
    void printStatistics();
};
