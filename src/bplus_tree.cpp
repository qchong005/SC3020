#include "bplus_tree.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <queue>

BPlusTree::BPlusTree(int max_keys, const std::string &filename)
    : root(nullptr), n(max_keys), next_node_id(1), index_filename(filename), total_nodes(0), tree_height(0)
{

    if (n <= 0)
    {
        n = 100; // Default optimal value from our calculation
    }
}

NodePtr BPlusTree::createNode(bool is_leaf)
{
    auto node = std::make_shared<BPlusNode>(is_leaf);
    node->node_id = next_node_id++;
    node->keys.reserve(n);

    if (is_leaf)
    {
        node->values.reserve(n);
        node->next_leaf = 0;
    }
    else
    {
        node->children.reserve(n + 1);
    }

    nodes[node->node_id] = node;
    return node;
}

void BPlusTree::insert(float key, const RecordRef &record_ref)
{
    if (!root)
    {
        // Create root as leaf node
        root = createNode(true);
        root->is_root = true;
        insertIntoLeaf(root, key, record_ref);
        return;
    }

    NodePtr leaf = findLeafNode(key);
    insertIntoLeaf(leaf, key, record_ref);

    // Check if leaf needs splitting
    if ((int)leaf->keys.size() > n)
    {
        auto [new_leaf, promote_key] = splitLeafNode(leaf);
        insertIntoParent(leaf, promote_key, new_leaf);
    }
}

NodePtr BPlusTree::findLeafNode(float key)
{
    NodePtr current = root;

    while (!current->is_leaf)
    {
        int i = 0;
        while (i < (int)current->keys.size() && key >= current->keys[i])
        {
            i++;
        }

        if (i < (int)current->children.size())
        {
            auto child_it = nodes.find(current->children[i]);
            if (child_it != nodes.end())
            {
                current = child_it->second;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return current;
}

void BPlusTree::insertIntoLeaf(NodePtr leaf, float key, const RecordRef &record_ref)
{
    // Find position to insert
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    int pos = it - leaf->keys.begin();

    // Check if key already exists
    if (pos < (int)leaf->keys.size() && leaf->keys[pos] == key)
    {
        // Add to existing key's values
        leaf->values[pos].push_back(record_ref);
    }
    else
    {
        // Insert new key
        leaf->keys.insert(leaf->keys.begin() + pos, key);
        leaf->values.insert(leaf->values.begin() + pos, std::vector<RecordRef>{record_ref});
    }
}

void BPlusTree::insertIntoInternal(NodePtr internal, float key, std::uint32_t child_id)
{
    // Find position to insert
    auto it = std::lower_bound(internal->keys.begin(), internal->keys.end(), key);
    int pos = it - internal->keys.begin();

    // Insert key and child pointer
    internal->keys.insert(internal->keys.begin() + pos, key);
    internal->children.insert(internal->children.begin() + pos + 1, child_id);

    // Update parent relationship
    auto child = nodes[child_id];
    child->parent_id = internal->node_id;
}

std::pair<NodePtr, float> BPlusTree::splitLeafNode(NodePtr leaf)
{
    NodePtr new_leaf = createNode(true);

    int mid = (leaf->keys.size()) / 2;

    // Move half the keys to new leaf
    new_leaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    new_leaf->values.assign(leaf->values.begin() + mid, leaf->values.end());

    // Update original leaf
    leaf->keys.erase(leaf->keys.begin() + mid, leaf->keys.end());
    leaf->values.erase(leaf->values.begin() + mid, leaf->values.end());

    // Update leaf links
    new_leaf->next_leaf = leaf->next_leaf;
    leaf->next_leaf = new_leaf->node_id;

    // Set parent
    new_leaf->parent_id = leaf->parent_id;

    return {new_leaf, new_leaf->keys[0]};
}

std::pair<NodePtr, float> BPlusTree::splitInternalNode(NodePtr internal)
{
    NodePtr new_internal = createNode(false);

    int mid = internal->keys.size() / 2;
    float promote_key = internal->keys[mid];

    // Move keys and children to new node
    new_internal->keys.assign(internal->keys.begin() + mid + 1, internal->keys.end());
    new_internal->children.assign(internal->children.begin() + mid + 1, internal->children.end());

    // Update parent relationships for moved children
    for (auto child_id : new_internal->children)
    {
        if (nodes.find(child_id) != nodes.end())
        {
            nodes[child_id]->parent_id = new_internal->node_id;
        }
    }

    // Update original node
    internal->keys.erase(internal->keys.begin() + mid, internal->keys.end());
    internal->children.erase(internal->children.begin() + mid + 1, internal->children.end());

    // Set parent
    new_internal->parent_id = internal->parent_id;

    return {new_internal, promote_key};
}

void BPlusTree::insertIntoParent(NodePtr left, float key, NodePtr right)
{
    if (left->is_root)
    {
        // Create new root
        NodePtr new_root = createNode(false);
        new_root->is_root = true;
        new_root->keys.push_back(key);
        new_root->children.push_back(left->node_id);
        new_root->children.push_back(right->node_id);

        left->is_root = false;
        left->parent_id = new_root->node_id;
        right->parent_id = new_root->node_id;

        root = new_root;
        return;
    }

    // Find parent
    NodePtr parent = findParent(left);
    if (!parent)
    {
        std::cerr << "Error: Could not find parent node" << std::endl;
        return;
    }

    insertIntoInternal(parent, key, right->node_id);

    // Check if parent needs splitting
    if ((int)parent->keys.size() > n)
    {
        auto [new_parent, promote_key] = splitInternalNode(parent);
        insertIntoParent(parent, promote_key, new_parent);
    }
}

NodePtr BPlusTree::findParent(NodePtr child)
{
    if (child->parent_id == 0)
        return nullptr;

    auto it = nodes.find(child->parent_id);
    return (it != nodes.end()) ? it->second : nullptr;
}

void BPlusTree::bulkLoad(std::vector<std::pair<float, RecordRef>> &data)
{
    // Clear existing tree
    nodes.clear();
    root = nullptr;
    next_node_id = 1;

    // Sort data by key
    std::sort(data.begin(), data.end());

    std::cout << "Building B+ tree with " << data.size() << " records..." << std::endl;

    // Insert all records
    for (size_t i = 0; i < data.size(); i++)
    {
        insert(data[i].first, data[i].second);

        // Progress indicator
        if (i % 5000 == 0)
        {
            std::cout << "Inserted " << i << " records..." << std::endl;
        }
    }

    updateStatistics();
    std::cout << "B+ tree construction completed." << std::endl;
}

std::vector<RecordRef> BPlusTree::search(float key)
{
    if (!root)
        return {};

    NodePtr leaf = findLeafNode(key);

    // Binary search in leaf node
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);

    if (it != leaf->keys.end() && *it == key)
    {
        int index = it - leaf->keys.begin();
        return leaf->values[index];
    }

    return {};
}

/*
// OLD, BUGGY IMPLEMENTATION
std::vector<RecordRef> BPlusTree::searchGreaterThan(float key) {
    std::vector<RecordRef> result;
    if (!root) return result;

    // Find the first leaf node that might contain keys > key
    NodePtr leaf = findLeafNode(key);

    // Start from the appropriate position in the first leaf
    bool started = false;

    while (leaf) {
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] > key) {
                started = true;
                // Add all records for this key
                for (const auto& ref : leaf->values[i]) {
                    result.push_back(ref);
                }
            } else if (started) {
                // Since leaf keys are sorted, if we've started collecting
                // and find a key <= target, we can stop
                break;
            }
        }

        // If we haven't started yet, or if we broke early, move to next leaf
        if (!started || leaf->keys.empty()) {
            if (leaf->next_leaf == 0) break;
            auto next_it = nodes.find(leaf->next_leaf);
            leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
        } else {
            // Move to next leaf to continue collecting
            if (leaf->next_leaf == 0) break;
            auto next_it = nodes.find(leaf->next_leaf);
            leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
        }
    }

    return result;
}

std::vector<RecordRef> BPlusTree::searchRange(float min_key, float max_key) {
    std::vector<RecordRef> result;
    if (!root || min_key > max_key) return result;

    // Find the first leaf node that might contain keys >= min_key
    NodePtr leaf = findLeafNode(min_key);

    while (leaf) {
        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] >= min_key && leaf->keys[i] <= max_key) {
                // Add all records for this key
                for (const auto& ref : leaf->values[i]) {
                    result.push_back(ref);
                }
            } else if (leaf->keys[i] > max_key) {
                // Since keys are sorted, we can stop here
                return result;
            }
        }

        // Move to next leaf
        if (leaf->next_leaf == 0) break;
        auto next_it = nodes.find(leaf->next_leaf);
        leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
    }

    return result;
}

std::pair<std::vector<RecordRef>, int> BPlusTree::searchGreaterThanWithStats(float key) {
    std::vector<RecordRef> result;
    int nodes_accessed = 0;

    if (!root) return {result, nodes_accessed};

    // Track nodes accessed during tree traversal to first leaf
    NodePtr current = root;
    while (!current->is_leaf) {
        nodes_accessed++; // Count internal node access

        int i = 0;
        while (i < (int)current->keys.size() && key >= current->keys[i]) {
            i++;
        }

        if (i < (int)current->children.size()) {
            auto child_it = nodes.find(current->children[i]);
            if (child_it != nodes.end()) {
                current = child_it->second;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    // Now we're at the first leaf - start collecting
    NodePtr leaf = current;
    bool started = false;

    while (leaf) {
        nodes_accessed++; // Count leaf node access

        for (size_t i = 0; i < leaf->keys.size(); i++) {
            if (leaf->keys[i] > key) {
                started = true;
                // Add all records for this key
                for (const auto& ref : leaf->values[i]) {
                    result.push_back(ref);
                }
            } else if (started) {
                // Since leaf keys are sorted, if we've started collecting
                // and find a key <= target, we can stop
                break;
            }
        }

        // If we haven't started yet, or if we broke early, move to next leaf
        if (!started || leaf->keys.empty()) {
            if (leaf->next_leaf == 0) break;
            auto next_it = nodes.find(leaf->next_leaf);
            leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
        } else {
            // Move to next leaf to continue collecting
            if (leaf->next_leaf == 0) break;
            auto next_it = nodes.find(leaf->next_leaf);
            leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
        }
    }

    return {result, nodes_accessed};
}
*/

// NEW, CORRECTED IMPLEMENTATION
std::vector<RecordRef> BPlusTree::searchGreaterThan(float key)
{
    auto [result, nodes_accessed] = searchGreaterThanWithStats(key);
    return result;
}

std::vector<RecordRef> BPlusTree::searchRange(float min_key, float max_key)
{
    std::vector<RecordRef> result;
    if (!root || min_key > max_key)
        return result;

    // Find the first leaf node that might contain keys >= min_key
    NodePtr leaf = findLeafNode(min_key);

    while (leaf)
    {
        for (size_t i = 0; i < leaf->keys.size(); i++)
        {
            if (leaf->keys[i] >= min_key && leaf->keys[i] <= max_key)
            {
                // Add all records for this key
                for (const auto &ref : leaf->values[i])
                {
                    result.push_back(ref);
                }
            }
            else if (leaf->keys[i] > max_key)
            {
                // Since keys are sorted, we can stop here
                return result;
            }
        }

        // Move to next leaf
        if (leaf->next_leaf == 0)
            break;
        auto next_it = nodes.find(leaf->next_leaf);
        leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
    }

    return result;
}

std::pair<std::vector<RecordRef>, int> BPlusTree::searchGreaterThanWithStats(float key)
{
    std::vector<RecordRef> result;
    int nodes_accessed = 0;

    if (!root)
        return {result, nodes_accessed};

    // 1. Traverse to the first leaf node that could contain the key.
    NodePtr current = root;
    while (current && !current->is_leaf)
    {
        nodes_accessed++; // Count internal node access
        int i = 0;
        while (i < (int)current->keys.size() && key >= current->keys[i])
        {
            i++;
        }
        current = nodes.count(current->children[i]) ? nodes[current->children[i]] : nullptr;
    }

    NodePtr leaf = current;

    // 2. Iterate from this leaf onwards.
    while (leaf)
    {
        nodes_accessed++; // Count leaf node access
        for (size_t i = 0; i < leaf->keys.size(); i++)
        {
            if (leaf->keys[i] > key)
            {
                // Add all records for this key
                for (const auto &ref : leaf->values[i])
                {
                    result.push_back(ref);
                }
            }
        }

        // 3. Move to the next leaf.
        if (leaf->next_leaf == 0)
        {
            break; // End of list
        }
        leaf = nodes.count(leaf->next_leaf) ? nodes[leaf->next_leaf] : nullptr;
    }

    return {result, nodes_accessed};
}

bool BPlusTree::deleteKey(float key, const RecordRef &record_ref)
{
    if (!root)
        return false;

    NodePtr leaf = findLeafNode(key);

    // Find the key in the leaf
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);

    if (it == leaf->keys.end() || *it != key)
    {
        return false; // Key not found
    }

    int index = it - leaf->keys.begin();

    // Remove the specific record reference from the values
    auto &value_list = leaf->values[index];
    auto ref_it = std::find(value_list.begin(), value_list.end(), record_ref);

    if (ref_it != value_list.end())
    {
        value_list.erase(ref_it);
    }

    // If this was the last reference for this key, remove the key
    if (value_list.empty())
    {
        leaf->keys.erase(leaf->keys.begin() + index);
        leaf->values.erase(leaf->values.begin() + index);
    }

    return true;
}

int BPlusTree::deleteGreaterThan(float key)
{
    if (!root)
        return 0;

    int deleted_count = 0;

    // Find the first leaf node that could contain the key
    NodePtr leaf = findLeafNode(key);

    // Iterate through all leaves from this point
    while (leaf)
    {
        // Collect keys to delete from this leaf
        std::vector<int> indices_to_delete;

        for (size_t i = 0; i < leaf->keys.size(); i++)
        {
            if (leaf->keys[i] > key)
            {
                // Count all records for this key
                deleted_count += leaf->values[i].size();
                indices_to_delete.push_back(i);
            }
        }

        // Delete in reverse order to maintain indices
        for (int i = indices_to_delete.size() - 1; i >= 0; i--)
        {
            int idx = indices_to_delete[i];
            leaf->keys.erase(leaf->keys.begin() + idx);
            leaf->values.erase(leaf->values.begin() + idx);
        }

        // Move to next leaf
        if (leaf->next_leaf == 0)
        {
            break;
        }
        auto next_it = nodes.find(leaf->next_leaf);
        leaf = (next_it != nodes.end()) ? next_it->second : nullptr;
    }

    // Update tree statistics
    updateStatistics();

    return deleted_count;
}

std::vector<float> BPlusTree::getRootKeys() const
{
    if (!root)
        return {};
    return root->keys;
}

void BPlusTree::updateStatistics()
{
    if (!root)
    {
        total_nodes = 0;
        tree_height = 0;
        return;
    }

    total_nodes = 0;
    countNodes(root, total_nodes);
    tree_height = calculateHeight(root);
}

void BPlusTree::countNodes(NodePtr node, int &count)
{
    if (!node)
        return;

    count++;

    if (!node->is_leaf)
    {
        for (auto child_id : node->children)
        {
            auto child_it = nodes.find(child_id);
            if (child_it != nodes.end())
            {
                countNodes(child_it->second, count);
            }
        }
    }
}

int BPlusTree::calculateHeight(NodePtr node)
{
    if (!node)
        return 0;

    if (node->is_leaf)
    {
        return 1;
    }

    // For internal nodes, recurse to first child
    if (!node->children.empty())
    {
        auto child_it = nodes.find(node->children[0]);
        if (child_it != nodes.end())
        {
            return 1 + calculateHeight(child_it->second);
        }
    }

    return 1;
}

void BPlusTree::printStatistics()
{
    updateStatistics();

    std::cout << "=== B+ Tree Statistics ===" << std::endl;
    std::cout << "Parameter n: " << n << std::endl;
    std::cout << "Number of nodes: " << total_nodes << std::endl;
    std::cout << "Number of levels: " << tree_height << std::endl;

    std::cout << "Root node keys: ";
    auto root_keys = getRootKeys();
    if (root_keys.empty())
    {
        std::cout << "(empty tree)";
    }
    else
    {
        for (size_t i = 0; i < root_keys.size(); i++)
        {
            if (i > 0)
                std::cout << ", ";
            std::cout << root_keys[i];
        }
    }
    std::cout << std::endl;
}

void BPlusTree::printTree()
{
    if (!root)
    {
        std::cout << "Empty tree" << std::endl;
        return;
    }

    std::cout << "=== B+ Tree Structure ===" << std::endl;
    std::queue<std::pair<NodePtr, int>> q;
    q.push({root, 0});
    int current_level = -1;

    while (!q.empty())
    {
        auto [node, level] = q.front();
        q.pop();

        if (level != current_level)
        {
            current_level = level;
            std::cout << "\nLevel " << level << ": ";
        }

        std::cout << "[";
        for (size_t i = 0; i < node->keys.size(); i++)
        {
            if (i > 0)
                std::cout << ", ";
            std::cout << node->keys[i];
        }
        std::cout << "] ";

        if (!node->is_leaf)
        {
            for (auto child_id : node->children)
            {
                auto child_it = nodes.find(child_id);
                if (child_it != nodes.end())
                {
                    q.push({child_it->second, level + 1});
                }
            }
        }
    }
    std::cout << std::endl;
}

void BPlusTree::saveToDisk()
{
    updateStatistics();

    std::ofstream file(index_filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open index file for writing: " << index_filename << std::endl;
        return;
    }

    // Write file header
    uint32_t magic = 0x42504c55; // "BPLU" magic number
    file.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char *>(&n), sizeof(n));
    file.write(reinterpret_cast<const char *>(&total_nodes), sizeof(total_nodes));
    file.write(reinterpret_cast<const char *>(&tree_height), sizeof(tree_height));
    file.write(reinterpret_cast<const char *>(&next_node_id), sizeof(next_node_id));

    uint32_t root_id = root ? root->node_id : 0;
    file.write(reinterpret_cast<const char *>(&root_id), sizeof(root_id));

    // Write all nodes
    for (const auto &[id, node] : nodes)
    {
        saveNodeToDisk(file, node);
    }

    file.close();
    std::cout << "B+ tree saved to disk: " << index_filename << std::endl;
}

void BPlusTree::loadFromDisk()
{
    std::ifstream file(index_filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Index file not found, will create new index: " << index_filename << std::endl;
        return;
    }

    // Read and verify header
    uint32_t magic;
    file.read(reinterpret_cast<char *>(&magic), sizeof(magic));
    if (magic != 0x42504c55)
    {
        std::cerr << "Invalid index file format" << std::endl;
        file.close();
        return;
    }

    file.read(reinterpret_cast<char *>(&n), sizeof(n));
    file.read(reinterpret_cast<char *>(&total_nodes), sizeof(total_nodes));
    file.read(reinterpret_cast<char *>(&tree_height), sizeof(tree_height));
    file.read(reinterpret_cast<char *>(&next_node_id), sizeof(next_node_id));

    uint32_t root_id;
    file.read(reinterpret_cast<char *>(&root_id), sizeof(root_id));

    // Clear existing nodes
    nodes.clear();
    root = nullptr;

    // Load all nodes
    for (int i = 0; i < total_nodes; i++)
    {
        auto node = loadNodeFromDisk(file);
        if (node)
        {
            nodes[node->node_id] = node;
            if (node->node_id == root_id)
            {
                root = node;
            }
        }
    }

    file.close();
    std::cout << "B+ tree loaded from disk: " << index_filename << std::endl;
    printStatistics();
}

void BPlusTree::saveNodeToDisk(std::ofstream &file, NodePtr node)
{
    if (!node)
        return;

    // Write node header
    file.write(reinterpret_cast<const char *>(&node->is_leaf), sizeof(node->is_leaf));
    file.write(reinterpret_cast<const char *>(&node->is_root), sizeof(node->is_root));
    file.write(reinterpret_cast<const char *>(&node->node_id), sizeof(node->node_id));
    file.write(reinterpret_cast<const char *>(&node->parent_id), sizeof(node->parent_id));

    uint32_t num_keys = node->keys.size();
    file.write(reinterpret_cast<const char *>(&num_keys), sizeof(num_keys));

    // Write keys
    for (const auto &key : node->keys)
    {
        file.write(reinterpret_cast<const char *>(&key), sizeof(float));
    }

    if (!node->is_leaf)
    {
        // Write children
        uint32_t num_children = node->children.size();
        file.write(reinterpret_cast<const char *>(&num_children), sizeof(num_children));
        for (auto child_id : node->children)
        {
            file.write(reinterpret_cast<const char *>(&child_id), sizeof(child_id));
        }
    }
    else
    {
        // Leaf node - write values and next pointer
        file.write(reinterpret_cast<const char *>(&node->next_leaf), sizeof(node->next_leaf));

        // Write values
        for (const auto &value_list : node->values)
        {
            uint32_t value_count = value_list.size();
            file.write(reinterpret_cast<const char *>(&value_count), sizeof(value_count));
            for (const auto &ref : value_list)
            {
                file.write(reinterpret_cast<const char *>(&ref.block_id), sizeof(ref.block_id));
                file.write(reinterpret_cast<const char *>(&ref.record_offset), sizeof(ref.record_offset));
            }
        }
    }
}

NodePtr BPlusTree::loadNodeFromDisk(std::ifstream &file)
{
    bool is_leaf, is_root;
    uint32_t node_id, parent_id, num_keys;

    // Read node header
    file.read(reinterpret_cast<char *>(&is_leaf), sizeof(is_leaf));
    file.read(reinterpret_cast<char *>(&is_root), sizeof(is_root));
    file.read(reinterpret_cast<char *>(&node_id), sizeof(node_id));
    file.read(reinterpret_cast<char *>(&parent_id), sizeof(parent_id));
    file.read(reinterpret_cast<char *>(&num_keys), sizeof(num_keys));

    // Create node
    auto node = std::make_shared<BPlusNode>(is_leaf);
    node->is_root = is_root;
    node->node_id = node_id;
    node->parent_id = parent_id;

    // Read keys
    node->keys.resize(num_keys);
    for (uint32_t i = 0; i < num_keys; i++)
    {
        file.read(reinterpret_cast<char *>(&node->keys[i]), sizeof(float));
    }

    if (!is_leaf)
    {
        // Read children
        uint32_t num_children;
        file.read(reinterpret_cast<char *>(&num_children), sizeof(num_children));
        node->children.resize(num_children);
        for (uint32_t i = 0; i < num_children; i++)
        {
            file.read(reinterpret_cast<char *>(&node->children[i]), sizeof(uint32_t));
        }
    }
    else
    {
        // Leaf node - read values and next pointer
        file.read(reinterpret_cast<char *>(&node->next_leaf), sizeof(node->next_leaf));

        // Read values
        node->values.resize(num_keys);
        for (uint32_t i = 0; i < num_keys; i++)
        {
            uint32_t value_count;
            file.read(reinterpret_cast<char *>(&value_count), sizeof(value_count));
            node->values[i].resize(value_count);
            for (uint32_t j = 0; j < value_count; j++)
            {
                file.read(reinterpret_cast<char *>(&node->values[i][j].block_id), sizeof(node->values[i][j].block_id));
                file.read(reinterpret_cast<char *>(&node->values[i][j].record_offset),
                          sizeof(node->values[i][j].record_offset));
            }
        }
    }

    return node;
}
