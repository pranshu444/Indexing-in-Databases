#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <algorithm>

using namespace std;

// Global disk access tracking
int seekCount = 0;
int transferCount = 0;
int currentBlock = -1;
int globalBlockID = 0;

void diskAccess(int blockNum) {
    transferCount++;
    if (currentBlock == -1 || (blockNum != currentBlock && blockNum != currentBlock + 1)) {
        seekCount++;
    }
    currentBlock = blockNum;
}

// Node structure
struct BPlusNode {
    bool isLeaf;
    vector<int> keys;
    vector<BPlusNode*> children;                // for internal nodes
    vector<pair<int, int>> keyValuePairs;       // for leaf nodes
    BPlusNode* next;                            // for leaf chaining
    int blockID;

    BPlusNode(bool leaf) : isLeaf(leaf), next(nullptr) {
        blockID = globalBlockID++;
    }
};

class BPlusTree {
private:
    BPlusNode* root;
    int order;

    void splitChild(BPlusNode* parent, int index, BPlusNode* child) {
        BPlusNode* newChild = new BPlusNode(child->isLeaf);
        diskAccess(child->blockID);

        int mid = order;

        if (child->isLeaf) {
            newChild->keyValuePairs.assign(child->keyValuePairs.begin() + mid, child->keyValuePairs.end());
            child->keyValuePairs.resize(mid);

            newChild->next = child->next;
            child->next = newChild;

            newChild->keys.clear();
            for (auto& kv : newChild->keyValuePairs)
                newChild->keys.push_back(kv.first);

            parent->keys.insert(parent->keys.begin() + index, newChild->keyValuePairs[0].first);
        } else {
            newChild->keys.assign(child->keys.begin() + mid + 1, child->keys.end());
            newChild->children.assign(child->children.begin() + mid + 1, child->children.end());

            int promotedKey = child->keys[mid];

            child->keys.resize(mid);
            child->children.resize(mid + 1);

            parent->keys.insert(parent->keys.begin() + index, promotedKey);
        }

        parent->children.insert(parent->children.begin() + index + 1, newChild);
    }

    void insertNonFull(BPlusNode* node, int key, int value) {
        diskAccess(node->blockID);

        if (node->isLeaf) {
            auto pos = lower_bound(node->keyValuePairs.begin(), node->keyValuePairs.end(), make_pair(key, value));
            node->keyValuePairs.insert(pos, {key, value});

            node->keys.clear();
            for (auto& kv : node->keyValuePairs)
                node->keys.push_back(kv.first);
        } else {
            int i = upper_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin();
            BPlusNode* child = node->children[i];
            if (child->keys.size() == order*2) {
                splitChild(node, i, child);
                if (key > node->keys[i]) i++;
            }
            insertNonFull(node->children[i], key, value);
        }
    }

public:
    BPlusTree(int C, int gamma, int eta) {
        root = new BPlusNode(true);
        order = (C - eta) / (2 * (gamma + eta));
        cout << "Calculated order: " << order << endl;
    }

    void insert(int key, int value) {
        if (root->keys.size() == order*2) {
            BPlusNode* newRoot = new BPlusNode(false);
            newRoot->children.push_back(root);
            splitChild(newRoot, 0, root);
            root = newRoot;
        }
        insertNonFull(root, key, value);
    }

    bool search(int key, int& valueOut) {
        BPlusNode* curr = root;
        while (!curr->isLeaf) {
            diskAccess(curr->blockID);
            int i = upper_bound(curr->keys.begin(), curr->keys.end(), key) - curr->keys.begin();
            curr = curr->children[i];
        }

        diskAccess(curr->blockID);
        for (auto& kv : curr->keyValuePairs) {
            if (kv.first == key) {
                valueOut = kv.second;
                return true;
            }
        }
        return false;
    }

    void searchLessThan(int value, vector<int>& result) {
        BPlusNode* curr = root;
    
        // Go to the leftmost leaf (first block)
        while (!curr->isLeaf) {
            diskAccess(curr->blockID);
            curr = curr->children[0];
        }
    
        // Traverse until value is passed
        while (curr != nullptr) {
            diskAccess(curr->blockID);
            for (auto& kv : curr->keyValuePairs) {
                if (kv.first < value) {
                    result.push_back(kv.second);
                } else {
                    return; // Early exit once we cross the threshold
                }
            }
            curr = curr->next;
        }
    }
    

    void searchGreaterThan(int value, vector<int>& result) {
        BPlusNode* curr = root;

        // Traverse to the appropriate leaf node where the value might exist
        while (!curr->isLeaf) {
            diskAccess(curr->blockID);
            int i = upper_bound(curr->keys.begin(), curr->keys.end(), value) - curr->keys.begin();
            curr = curr->children[i];
        }

        // Traverse the leaf nodes starting from the found node
        while (curr != nullptr) {
            diskAccess(curr->blockID);
            for (auto& kv : curr->keyValuePairs) {
                if (kv.first > value) {
                    result.push_back(kv.second);
                }
            }
            curr = curr->next;
        }
    }
    

    void printTree() {
        queue<BPlusNode*> q;
        q.push(root);
        cout << "\nTree Structure:\n";

        while (!q.empty()) {
            int levelSize = q.size();
            while (levelSize--) {
                BPlusNode* node = q.front(); q.pop();
                if (node->isLeaf) {
                    cout << "[";
                    for (auto& kv : node->keyValuePairs) cout << kv.first << ":" << kv.second << " ";
                    cout << "]";
                } else {
                    cout << "<";
                    for (int k : node->keys) cout << k << " ";
                    cout << ">";
                    for (auto child : node->children) q.push(child);
                }
                cout << "  ";
            }
            cout << endl;
        }
    }

    int calculateHeight() {
        int height = 0;
        BPlusNode* curr = root;
        while (curr && !curr->isLeaf) {
            height++;
            curr = curr->children[0];
        }
        return height + 1; // include leaf level
    }

    void resetDiskMetrics() {
        seekCount = transferCount = 0;
        currentBlock = -1;
    }

    pair<int, int> getDiskMetrics() {
        return {seekCount, transferCount};
    }
};

int main() {
    // Block parameters
    int C = 512;      // block size in bytes
    int gamma = 4;    // key size
    int eta = 8;      // pointer size

    BPlusTree tree(C, gamma, eta);

    vector<pair<int, int>> data;
    for (int i = 1; i <= 50; i++) {
        data.emplace_back(i * 10, i * 100);
    }

    for (size_t i = 0; i < data.size(); i++) {
        tree.insert(data[i].first, data[i].second);
    }

    tree.printTree();
    cout << "Height of B+ Tree: " << tree.calculateHeight() << endl;

    auto diskMetrics = tree.getDiskMetrics();
    int insertSeeks = diskMetrics.first;
    int insertTransfers = diskMetrics.second;
    cout << "\nInsertion Metrics:\nSeeks: " << insertSeeks << ", Transfers: " << insertTransfers << endl;

    tree.resetDiskMetrics();

    vector<int> lessThanResult;
    tree.searchLessThan(280, lessThanResult);
    cout << "\nValues less than 2800: ";
    for (int value : lessThanResult) {
        cout << value << " ";
    }
    auto searchMetrics1 = tree.getDiskMetrics();
    int searchSeeks1 = searchMetrics1.first;
    int searchTransfers1 = searchMetrics1.second;
    cout << "\nLess Than Query Metrics:\nSeeks: " << searchSeeks1 << ", Transfers: " << searchTransfers1 << endl;
    tree.resetDiskMetrics();

    // Greater than query
    vector<int> greaterThanResult;
    tree.searchGreaterThan(280, greaterThanResult);
    cout << "\nValues greater than 2800: ";
    for (int value : greaterThanResult) {
        cout << value << " ";
    }
    auto searchMetrics2 = tree.getDiskMetrics();
    int searchSeeks2 = searchMetrics2.first;
    int searchTransfers2 = searchMetrics2.second;
    cout << "\nGreater Than Query Metrics:\nSeeks: " << searchSeeks2 << ", Transfers: " << searchTransfers2 << endl;
    tree.resetDiskMetrics();

    vector<int> queries = {280};
    for (int q : queries) {
        int val;
        bool found = tree.search(q, val);
        cout << "\nSearch key " << q << ": " << (found ? "Found, value = " + to_string(val) : "Not found");
    }

    auto searchMetrics = tree.getDiskMetrics();
    int searchSeeks = searchMetrics.first;
    int searchTransfers = searchMetrics.second;
    cout << "\nEquality Query Metrics:\nSeeks: " << searchSeeks << ", Transfers: " << searchTransfers << endl;

    return 0;
}