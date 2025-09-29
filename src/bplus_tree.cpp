#include "bplus_tree.h"
#include <iostream>
#include <algorithm>
#include <cstring>

BPlusTree::BPlusTree(int order, const std::string& filename)
    : root(nullptr), index_filename(filename), next_node_id(1),
      total_nodes(0), tree_levels(0) {
    if (order <= 0) {
        // Optimal parameter based on data analysis:
        // - 26,651 records with ~350 unique FT_PCT_home values
        // - Average 76 records per key, ~468 bytes per leaf entry
        // - Block size: 4,096 bytes, ~8 keys max per leaf node
        // - n=25 creates 3-level tree with ~50 nodes (good balance)
        n = 25;
    } else {
        n = order;
    }
}

NodePtr BPlusTree::createNode(NodeType type) {
    auto node = std::make_shared<BPlusTreeNode>(type, next_node_id++);
    node->keys.reserve(n);

    if (type == NodeType::INTERNAL) {
        node->children.reserve(n + 1);
    } else {
        node->values.reserve(n);
        node->next_leaf = 0;
    }

    // Store node in our node map
    nodes[node->node_id] = node;

    return node;
}

/* void BPlusTree::insert(float key, const RecordRef& record_ref) {
    if (!root) {
        // Create root as leaf node
        root = createNode(NodeType::LEAF);
        root->is_root = true;
        insertIntoLeaf(root, key, record_ref);
        return;
    }

    NodePtr leaf = findLeafNode(key);
    insertIntoLeaf(leaf, key, record_ref);

    // Check if leaf needs splitting
    if (leaf->num_keys > n) {
        NodePtr new_leaf = splitLeafNode(leaf);

        if (leaf->is_root) {
            // Create new root
            NodePtr new_root = createNode(NodeType::INTERNAL);
            new_root->is_root = true;
            new_root->keys.push_back(new_leaf->keys[0]);
            new_root->children.push_back(leaf->node_id);
            new_root->children.push_back(new_leaf->node_id);
            new_root->num_keys = 1;

            leaf->is_root = false;
            root = new_root;
        } else {
            insertIntoParent(leaf, new_leaf->keys[0], new_leaf);
        }
    }
}*/

void BPlusTree::insert(float key, const RecordRef& record_ref) {
    // Empty tree: make a leaf root
    if (!root) {
        root = createNode(NodeType::LEAF);
        root->is_root = true;
        // first key/bucket
        insertIntoLeaf(root, key, record_ref);
        return;
    }

    // 1) Find target leaf and insert (append to bucket if duplicate key)
    NodePtr leaf = findLeafNode(key);
    insertIntoLeaf(leaf, key, record_ref);

    // 2) Split leaf if it overflowed (use leafMaxKeys, not n)
    if (leaf->num_keys > 8) {
        NodePtr right_leaf = splitLeafNode(leaf);                 // left keeps lower half
        // Promote the separator = first key of right leaf
        insertIntoParent(leaf, right_leaf->keys[0], right_leaf);  // handles root/non-root & parent splits
    }
}

NodePtr BPlusTree::findLeafNode(float key) {
    NodePtr current = root;

    while (current->type == NodeType::INTERNAL) {
        int i = 0;
        while (i < current->num_keys && key >= current->keys[i]) {
            i++;
        }

        // Get the child node
        if (i < current->children.size()) {
            auto child_it = nodes.find(current->children[i]);
            if (child_it != nodes.end()) {
                current = child_it->second;
            } else {
                break;  // Child not found, this shouldn't happen in proper implementation
            }
        } else {
            break;  // No more children
        }
    }

    return current;
}

void BPlusTree::insertIntoLeaf(NodePtr leaf, float key, const RecordRef& record_ref) {
    // Find position to insert
    int pos = 0;
    while (pos < leaf->num_keys && key > leaf->keys[pos]) {
        pos++;
    }

    // Check if key already exists
    if (pos < leaf->num_keys && leaf->keys[pos] == key) {
        // Add to existing key's values
        leaf->values[pos].push_back(record_ref);
    } else {
        // Insert new key
        leaf->keys.insert(leaf->keys.begin() + pos, key);
        leaf->values.insert(leaf->values.begin() + pos, std::vector<RecordRef>{record_ref});
        leaf->num_keys++;
    }
}

/* NodePtr BPlusTree::splitLeafNode(NodePtr leaf) {
    NodePtr new_leaf = createNode(NodeType::LEAF);

    int mid = (leaf->num_keys + 1) / 2;

    // Move half the keys to new leaf
    for (int i = mid; i < leaf->num_keys; i++) {
        new_leaf->keys.push_back(leaf->keys[i]);
        new_leaf->values.push_back(leaf->values[i]);
    }

    new_leaf->num_keys = leaf->num_keys - mid;

    // Update original leaf
    leaf->keys.erase(leaf->keys.begin() + mid, leaf->keys.end());
    leaf->values.erase(leaf->values.begin() + mid, leaf->values.end());
    leaf->num_keys = mid;

    // Update leaf links
    new_leaf->next_leaf = leaf->next_leaf;
    leaf->next_leaf = new_leaf->node_id;

    return new_leaf;
}*/

NodePtr BPlusTree::splitLeafNode(NodePtr left) {
    NodePtr right = createNode(NodeType::LEAF);

    const int L = left->num_keys;
    const int mid = L / 2;  // left keeps [0..mid-1], right gets [mid..L-1]

    // Build right leaf by moving the upper half
    right->num_keys = L - mid;
    right->keys.assign(left->keys.begin() + mid, left->keys.begin() + L);
    right->values.assign(
        std::make_move_iterator(left->values.begin() + mid),
        std::make_move_iterator(left->values.begin() + L)
    );

    // Shrink left leaf to its lower half
    left->num_keys = mid;
    left->keys.resize(mid);
    left->values.resize(mid);

    // Fix leaf chain
    right->next_leaf = left->next_leaf;
    left->next_leaf  = right->node_id;

    // If createNode() doesn’t already register the node, ensure it's in the map:
    // nodes[right->node_id] = right; total_nodes = (int)nodes.size();

    return right;  // caller MUST do: insertIntoParent(left, right->keys[0], right);
}

/*NodePtr BPlusTree::splitInternalNode(NodePtr node, int child_index) {
    NodePtr new_node = createNode(NodeType::INTERNAL);

    int mid = node->num_keys / 2;

    // Move keys and children to new node
    for (int i = mid + 1; i < node->num_keys; i++) {
        new_node->keys.push_back(node->keys[i]);
    }

    for (int i = mid + 1; i <= node->num_keys; i++) {
        new_node->children.push_back(node->children[i]);
    }

    new_node->num_keys = node->num_keys - mid - 1;

    // Update original node
    float promote_key = node->keys[mid];
    node->keys.erase(node->keys.begin() + mid, node->keys.end());
    node->children.erase(node->children.begin() + mid + 1, node->children.end());
    node->num_keys = mid;

    return new_node;
}*/

NodePtr BPlusTree::splitInternalNode(NodePtr node, float& promote_key) {
    // node is an INTERNAL that currently overflowed (num_keys > n)
    NodePtr new_node = createNode(NodeType::INTERNAL);

    const int K   = node->num_keys;
    const int mid = K / 2;                 // promote node->keys[mid]

    promote_key = node->keys[mid];

    // new_node gets keys (mid+1 .. K-1)
    new_node->keys.assign(node->keys.begin() + mid + 1, node->keys.begin() + K);

    // new_node gets children (mid+1 .. K)
    new_node->children.assign(node->children.begin() + mid + 1, node->children.end());

    new_node->num_keys = static_cast<std::uint16_t>(new_node->keys.size());

    // node keeps keys (0 .. mid-1) and children (0 .. mid)
    node->keys.resize(mid);
    node->children.resize(mid + 1);
    node->num_keys = static_cast<std::uint16_t>(mid);

    // Note: if you maintain parent maps elsewhere, re-parent moved children here.
    // for (int i = 0; i < (int)new_node->children.size(); ++i) setParent(new_node->node_id, new_node->children[i], i);
    // for (int i = 0; i < (int)node->children.size(); ++i)     setParent(node->node_id,     node->children[i], i);

    return new_node;
}

/* void BPlusTree::insertIntoParent(NodePtr left, float key, NodePtr right) {
    // Find parent of left node
    NodePtr parent = nullptr;

    // For simplification, we'll create a new root if left is root
    if (left->is_root) {
        NodePtr new_root = createNode(NodeType::INTERNAL);
        new_root->is_root = true;
        new_root->keys.push_back(key);
        new_root->children.push_back(left->node_id);
        new_root->children.push_back(right->node_id);
        new_root->num_keys = 1;

        left->is_root = false;
        root = new_root;
        return;
    }

    // In a full implementation, we would traverse from root to find the parent
    // For now, this is simplified
} */

void BPlusTree::insertIntoParent(NodePtr left, float sep_key, NodePtr right) {
    // Case A: left is root → make a new root with [left,right]
    if (left->is_root || left.get() == root.get()) {
        NodePtr new_root = createNode(NodeType::INTERNAL);
        new_root->is_root = true;
        new_root->keys.clear();
        new_root->children.clear();

        new_root->keys.push_back(sep_key);
        new_root->children.push_back(left->node_id);
        new_root->children.push_back(right->node_id);
        new_root->num_keys = 1;

        // Old root is no longer root
        if (left->is_root) left->is_root = false;
        if (root && root.get() == left.get()) root->is_root = false;

        root = new_root;
        nodes[new_root->node_id] = new_root;

        // update stats lazily; calculateStatistics() will refresh later
        return;
    }

    // Case B: normal parent — locate it
    NodePtr parent; int li = -1;
    if (!findParent(left->node_id, parent, li) || !parent) {
        // Fallback: if we truly cannot find parent, promote to new root (defensive)
        NodePtr new_root = createNode(NodeType::INTERNAL);
        new_root->is_root = true;
        new_root->keys.push_back(sep_key);
        new_root->children.push_back(left->node_id);
        new_root->children.push_back(right->node_id);
        new_root->num_keys = 1;
        root = new_root;
        nodes[new_root->node_id] = new_root;
        return;
    }

    // Insert separator to the right of 'left'
    parent->keys.insert(parent->keys.begin() + li, sep_key);
    parent->children.insert(parent->children.begin() + li + 1, right->node_id);
    parent->num_keys++;

    // If parent fits, we are done
    if (parent->num_keys <= n) return;

    // Parent overflow: split internal and push promoted key upward
    // We'll implement the split inline (stable and clear).
    NodePtr L = parent;
    NodePtr R = createNode(NodeType::INTERNAL);

    int K = L->num_keys;
    int mid = K / 2;                 // promote L->keys[mid]
    float up_key = L->keys[mid];

    // R gets keys (mid+1..K-1) and children (mid+1..K)
    R->keys.assign(L->keys.begin() + mid + 1, L->keys.begin() + K);
    R->children.assign(L->children.begin() + mid + 1, L->children.end());
    R->num_keys = (int)R->keys.size();

    // L keeps keys (0..mid-1) and children (0..mid)
    L->keys.resize(mid);
    L->children.resize(mid + 1);
    L->num_keys = mid;

    nodes[R->node_id] = R;

    // If L was root, create a new root
    if (L.get() == root.get() || L->is_root) {
        NodePtr new_root = createNode(NodeType::INTERNAL);
        new_root->is_root = true;
        new_root->keys = { up_key };
        new_root->children = { L->node_id, R->node_id };
        new_root->num_keys = 1;
        if (root) root->is_root = false;
        root = new_root;
        nodes[new_root->node_id] = new_root;
        return;
    }

    // Otherwise, insert promoted key between L and R into L's parent (recurse)
    insertIntoParent(L, up_key, R);
}
 
void BPlusTree::bulkLoad(std::vector<std::pair<float, RecordRef>>& data) {
    // Sort data by key
    std::sort(data.begin(), data.end());

    // Build B+ tree bottom-up using bulk loading
    for (const auto& pair : data) {
        insert(pair.first, pair.second);
    }

    calculateStatistics();
}

std::vector<RecordRef> BPlusTree::search(float key) {
    if (!root) return {};

    NodePtr leaf = findLeafNode(key);

    // Binary search in leaf node
    auto it = std::lower_bound(leaf->keys.begin(),
                              leaf->keys.begin() + leaf->num_keys, key);

    if (it != leaf->keys.begin() + leaf->num_keys && *it == key) {
        int index = it - leaf->keys.begin();
        return leaf->values[index];
    }

    return {};
}

std::vector<RecordRef> BPlusTree::rangeSearch(float threshold, int &index_nodes_accessed, double *avg_key, int *out_count, int *out_unique_keys)
{
    index_nodes_accessed = 0;
    int unique_keys = 0;
    std::vector<RecordRef> out;
    if (!root) return out;

    // Descend to the leaf where "threshold" would land; count internal nodes
    NodePtr cur = root;
    while (cur && cur->type == NodeType::INTERNAL) {
        index_nodes_accessed++;
        int i = 0;
        while (i < cur->num_keys && threshold >= cur->keys[i]) i++;
        if (i < (int)cur->children.size()) {
            auto it = nodes.find(cur->children[i]);
            if (it == nodes.end()) break;
            cur = it->second;
        } else break;
    }
    if (!cur) return out;

    // Count the first leaf
    index_nodes_accessed++;

    // Walk right along the leaf chain, collecting > threshold
    double sum_keys = 0.0;
    size_t cnt_keys = 0;

    auto leaf = cur;
    while (leaf) {
        for (int i = 0; i < leaf->num_keys; ++i) {
            if (leaf->keys[i] > threshold) {
                ++unique_keys;

                // Accumulate average purely from index keys (each key repeats for its refs)
                if (avg_key) {
                    sum_keys += static_cast<double>(leaf->keys[i]) * leaf->values[i].size();
                    cnt_keys += leaf->values[i].size();
                }
                out.insert(out.end(), leaf->values[i].begin(), leaf->values[i].end());
            }
        }
        if (leaf->next_leaf == 0) break;
        auto it = nodes.find(leaf->next_leaf);
        if (it == nodes.end()) break;
        leaf = it->second;
        index_nodes_accessed++; // visited another leaf
    }

    if (avg_key) *avg_key = (cnt_keys ? (sum_keys / static_cast<double>(cnt_keys)) : 0.0);
    if (out_count) *out_count = out.size(); // returns number of records found in range search
    if (out_unique_keys) *out_unique_keys = unique_keys;  // returns number of unique keys found in range search
    return out;
}

void BPlusTree::rebuildTreeFromData(std::vector<std::pair<float, RecordRef>> &data)
{
    // Reset in-memory state
    nodes.clear();
    root = nullptr;
    next_node_id = 1;
    total_nodes = 0;
    tree_levels = 0;

    // Rebuild bottom-up with existing bulk loader (which sorts & inserts)
    bulkLoad(data);
}

std::vector<float> BPlusTree::getRootKeys() const {
    if (!root) return {};

    std::vector<float> root_keys;
    for (int i = 0; i < root->num_keys; i++) {
        root_keys.push_back(root->keys[i]);
    }

    return root_keys;
}

void BPlusTree::calculateStatistics() {
    if (!root) {
        total_nodes = 0;
        tree_levels = 0;
        return;
    }

    total_nodes = 0;
    countNodes(root, total_nodes);
    tree_levels = calculateLevels(root);
}

void BPlusTree::countNodes(NodePtr node, int& count) {
    if (!node) return;

    count++;

    if (node->type == NodeType::INTERNAL) {
        // Traverse all children
        for (auto child_id : node->children) {
            auto child_it = nodes.find(child_id);
            if (child_it != nodes.end()) {
                countNodes(child_it->second, count);
            }
        }
    }
}

int BPlusTree::calculateLevels(NodePtr node, int current_level) {
    if (!node) return 0;

    if (node->type == NodeType::LEAF) {
        return current_level;
    }

    // For internal nodes, recurse to first child
    if (!node->children.empty()) {
        auto child_it = nodes.find(node->children[0]);
        if (child_it != nodes.end()) {
            return calculateLevels(child_it->second, current_level + 1);
        }
    }

    return current_level;
}

void BPlusTree::printStatistics() {
    calculateStatistics();

    std::cout << "B+ Tree Statistics:" << std::endl;
    std::cout << "Parameter n: " << n << std::endl;
    std::cout << "Number of nodes: " << total_nodes << std::endl;
    std::cout << "Number of levels: " << tree_levels << std::endl;

    std::cout << "Root node keys: ";
    auto root_keys = getRootKeys();
    if (root_keys.empty()) {
        std::cout << "(empty tree)";
    } else {
        for (size_t i = 0; i < root_keys.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << root_keys[i];
        }
    }
    std::cout << std::endl;
}

void BPlusTree::saveToDisk() {
    calculateStatistics();

    std::ofstream file(index_filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open index file for writing: " << index_filename << std::endl;
        return;
    }

    // Write file header
    uint32_t magic = 0x42504c55; // "BPLU" magic number
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&n), sizeof(n));
    file.write(reinterpret_cast<const char*>(&total_nodes), sizeof(total_nodes));
    file.write(reinterpret_cast<const char*>(&tree_levels), sizeof(tree_levels));
    file.write(reinterpret_cast<const char*>(&next_node_id), sizeof(next_node_id));

    uint32_t root_id = root ? root->node_id : 0;
    file.write(reinterpret_cast<const char*>(&root_id), sizeof(root_id));

    // Write all nodes
    if (root) {
        saveNodeToDisk(file, root);
    }

    file.close();
    std::cout << "B+ tree saved to disk: " << index_filename << std::endl;
}

void BPlusTree::loadFromDisk() {
    std::ifstream file(index_filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Index file not found, will create new index: " << index_filename << std::endl;
        return;
    }

    // Read and verify header
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x42504c55) {
        std::cerr << "Invalid index file format" << std::endl;
        file.close();
        return;
    }

    file.read(reinterpret_cast<char*>(&n), sizeof(n));
    file.read(reinterpret_cast<char*>(&total_nodes), sizeof(total_nodes));
    file.read(reinterpret_cast<char*>(&tree_levels), sizeof(tree_levels));
    file.read(reinterpret_cast<char*>(&next_node_id), sizeof(next_node_id));

    uint32_t root_id;
    file.read(reinterpret_cast<char*>(&root_id), sizeof(root_id));

    // Clear existing nodes
    nodes.clear();
    root = nullptr;

    // Load all nodes
    if (root_id != 0) {
        loadNodeFromDisk(file, root_id);
        root = nodes[root_id];
    }

    file.close();
    std::cout << "B+ tree loaded from disk: " << index_filename << std::endl;
    printStatistics();
}

void BPlusTree::saveNodeToDisk(std::ofstream &file, NodePtr node) {
    if (!node) return;

    // Write node header
    file.write(reinterpret_cast<const char*>(&node->type), sizeof(node->type));
    file.write(reinterpret_cast<const char*>(&node->node_id), sizeof(node->node_id));
    file.write(reinterpret_cast<const char*>(&node->num_keys), sizeof(node->num_keys));
    file.write(reinterpret_cast<const char*>(&node->is_root), sizeof(node->is_root));

    // Write keys
    for (int i = 0; i < node->num_keys; i++) {
        file.write(reinterpret_cast<const char*>(&node->keys[i]), sizeof(float));
    }

    if (node->type == NodeType::INTERNAL) {
        // Write children
        uint32_t num_children = node->children.size();
        file.write(reinterpret_cast<const char*>(&num_children), sizeof(num_children));
        for (auto child_id : node->children) {
            file.write(reinterpret_cast<const char*>(&child_id), sizeof(child_id));
        }

        // Recursively save children
        for (auto child_id : node->children) {
            auto child_it = nodes.find(child_id);
            if (child_it != nodes.end()) {
                saveNodeToDisk(file, child_it->second);
            }
        }
    } else {
        // Leaf node - write values and next pointer
        file.write(reinterpret_cast<const char*>(&node->next_leaf), sizeof(node->next_leaf));

        // Write values
        for (int i = 0; i < node->num_keys; i++) {
            uint32_t value_count = node->values[i].size();
            file.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));
            for (const auto& ref : node->values[i]) {
                file.write(reinterpret_cast<const char*>(&ref.block_id), sizeof(ref.block_id));
                file.write(reinterpret_cast<const char*>(&ref.record_offset), sizeof(ref.record_offset));
            }
        }
    }
}

void BPlusTree::loadNodeFromDisk(std::ifstream &file, uint32_t expected_node_id) {
    NodeType type;
    uint32_t node_id;
    uint16_t num_keys;
    bool is_root;

    // Read node header
    file.read(reinterpret_cast<char*>(&type), sizeof(type));
    file.read(reinterpret_cast<char*>(&node_id), sizeof(node_id));
    file.read(reinterpret_cast<char*>(&num_keys), sizeof(num_keys));
    file.read(reinterpret_cast<char*>(&is_root), sizeof(is_root));

    // Create node
    auto node = std::make_shared<BPlusTreeNode>(type, node_id);
    node->num_keys = num_keys;
    node->is_root = is_root;

    // Read keys
    node->keys.resize(num_keys);
    for (int i = 0; i < num_keys; i++) {
        file.read(reinterpret_cast<char*>(&node->keys[i]), sizeof(float));
    }

    if (type == NodeType::INTERNAL) {
        // Read children
        uint32_t num_children;
        file.read(reinterpret_cast<char*>(&num_children), sizeof(num_children));
        node->children.resize(num_children);
        for (int i = 0; i < num_children; i++) {
            file.read(reinterpret_cast<char*>(&node->children[i]), sizeof(uint32_t));
        }

        // Store this node
        nodes[node_id] = node;

        // Recursively load children
        for (auto child_id : node->children) {
            loadNodeFromDisk(file, child_id);
        }
    } else {
        // Leaf node - read values and next pointer
        file.read(reinterpret_cast<char*>(&node->next_leaf), sizeof(node->next_leaf));

        // Read values
        node->values.resize(num_keys);
        for (int i = 0; i < num_keys; i++) {
            uint32_t value_count;
            file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
            node->values[i].resize(value_count);
            for (int j = 0; j < value_count; j++) {
                file.read(reinterpret_cast<char*>(&node->values[i][j].block_id),
                         sizeof(node->values[i][j].block_id));
                file.read(reinterpret_cast<char*>(&node->values[i][j].record_offset),
                         sizeof(node->values[i][j].record_offset));
            }
        }

        // Store this node
        nodes[node_id] = node;
    }
}

bool BPlusTree::findParent(uint32_t child_id, NodePtr& out_parent, int& out_child_idx) {
    out_parent = nullptr;
    out_child_idx = -1;
    if (!root || root->type == NodeType::LEAF) return false;
    // BFS/DFS; tree is small, DFS is fine.
    std::vector<NodePtr> stack{root};
    while (!stack.empty()) {
        NodePtr cur = stack.back(); stack.pop_back();
        if (!cur || cur->type != NodeType::INTERNAL) continue;

        // Scan this internal's children for a direct hit
        for (int i = 0; i < (int)cur->children.size(); ++i) {
            if (cur->children[i] == child_id) {
                out_parent = cur;
                out_child_idx = i;
                return true;
            }
        }
        // Descend
        for (auto cid : cur->children) {
            auto it = nodes.find(cid);
            if (it != nodes.end()) stack.push_back(it->second);
        }
    }
    return false;
}
