#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>

using namespace std;

// Metrics tracking
int seekCount = 0;
int transferCount = 0;
int currentBlock = -1;

class HashIndex {
private:
    struct Bucket {
        vector<int> keys;
        int overflowBlock = -1;
    };

    vector<Bucket> buckets;
    int numBuckets;
    int blockCapacity;
    
public:
    HashIndex(int bucketsNum, int capacity) 
        : numBuckets(bucketsNum), blockCapacity(capacity) {
        buckets.resize(numBuckets);
    }

    // Simulate disk access operation
    void diskAccess(int blockNum) {
        transferCount++;
        if (blockNum != currentBlock && blockNum != currentBlock + 1) {
            seekCount++;
        }
        currentBlock = blockNum;
    }

    // Hash function using modulo (static hashing)
    int hashFunction(int key) {
        return key % numBuckets;
    }

    // Insert key with overflow handling
    void insert(int key) {
        int bucketIdx = hashFunction(key);
        Bucket& primary = buckets[bucketIdx];
        
        // Check primary bucket
        if (primary.keys.size() < blockCapacity) {
            primary.keys.push_back(key);
            diskAccess(bucketIdx);
            return; 
        }

        // Handle overflow
        int currentBlock = bucketIdx;
        while (buckets[currentBlock].overflowBlock != -1) {
            currentBlock = buckets[currentBlock].overflowBlock;
            diskAccess(currentBlock);
        }

        if (buckets[currentBlock].keys.size() < blockCapacity) {
            buckets[currentBlock].keys.push_back(key);
            diskAccess(currentBlock);
        } else {
            // Create new overflow block
            Bucket newBucket;
            newBucket.keys.push_back(key);
            buckets.push_back(newBucket);
            buckets[currentBlock].overflowBlock = buckets.size() - 1;
            diskAccess(buckets.size() - 1);
        }
    }

    // Search for key with metrics
    pair<bool, int> search(int key) {
        int operations = 0;
        int bucketIdx = hashFunction(key);
        int currentBlock = bucketIdx;
        int overflowChain = 0;

        do {
            operations++;
            diskAccess(currentBlock);
            
            // Check current block
            for (int k : buckets[currentBlock].keys) {
                if (k == key) {
                    return {true, operations};
                }
            }

            // Follow overflow chain
            currentBlock = buckets[currentBlock].overflowBlock;
            if (currentBlock != -1) overflowChain++;
        } while (currentBlock != -1);

        return {false, operations};
    }

    // Print index structure
    void printStructure() {
        for (int i = 0; i < numBuckets; ++i) {
            cout << "Bucket " << i << ": [";
            for (int k : buckets[i].keys) cout << k << " ";
            cout << "]";
            
            int overflow = buckets[i].overflowBlock;
            while (overflow != -1) {
                cout << " -> Overflow " << overflow << ": [";
                for (int k : buckets[overflow].keys) cout << k << " ";
                cout << "]";
                overflow = buckets[overflow].overflowBlock;
            }
            cout << endl;
        }
    }
};

int main() {
    // Initialize hash index with 5 buckets and capacity of 3 keys/block
    HashIndex hi(5, 3);
    
    // Insert sample data
    vector<int> data = {14, 23, 35, 45, 12, 22, 30, 40, 51, 61, 71, 83, 93, 103};
    for (int k : data) {
        hi.insert(k);
    }

    // Perform searches
    auto result1 = hi.search(35);
    bool found1 = result1.first;
    int ops1 = result1.second;

    auto result2 = hi.search(50);
    bool found2 = result2.first;
    int ops2 = result2.second;
        
    // Print results
    cout << "Index Structure:\n";
    hi.printStructure();
    
    cout << "\nMetrics Summary:\n";
    cout << "Total seeks: " << seekCount << endl;
    cout << "Total transfers: " << transferCount << endl;
    cout << "Search operations:\n";
    cout << "35 found: " << boolalpha << found1 
         << " (Blocks checked: " << ops1 << ")\n";
    cout << "50 found: " << found2 
         << " (Blocks checked: " << ops2 << ")\n";

    return 0;
}