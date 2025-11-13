#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

// Metrics tracking
int seekCount = 0;
int transferCount = 0;
int currentBlock = 0;

// Simulate disk access operation - tracks sequential vs random access for both read and write
void diskAccess(int blockNum) {
    transferCount++;
    if (blockNum != currentBlock && blockNum != currentBlock + 1) {
        seekCount++;
        // cout << "  [Disk Seek] Moving to block " << blockNum << endl;
    }
    currentBlock = blockNum;
    // cout << (isWrite ? "  Writing" : "  Reading") << " block " << blockNum << endl;
}

// Simulate scanning all data blocks (once at the start)
void loadDataBlocks(int totalRows, int rowsPerBlock, int dataBlockStart = 0) {
    int totalBlocks = (totalRows + rowsPerBlock - 1) / rowsPerBlock;
    for (int i = 0; i < totalBlocks; i++) {
        diskAccess(dataBlockStart + i); // Ensure unique block number for data
    }
}



class BitmapIndex {
private:
    int numRows;             // Total records in table
    int bitsPerBlock;        // How many bits fit in one disk block
    int blocksPerBitmap;     // How many blocks needed per bitmap
    int dataBlockCount;      // Number of data blocks in the table
    
    // Maps column values to their bitmap blocks
    // Each bitmap is stored as vector of blocks, each block contains multiple bits
    unordered_map<string, vector<vector<bool>>> bitmaps;
    
    // Calculate which block contains a given row
    int getBlockForRow(int rowId) {
        return rowId / bitsPerBlock;
    }
    
    // Calculate position within a block for a given row
    int getPositionInBlock(int rowId) {
        return rowId % bitsPerBlock;
    }
    
    // Generate unique block number for each bitmap's blocks
    int getBitmapBlockId(const string& bitmap, int blockIdx) {
        size_t h = hash<string>{}(bitmap);
        return dataBlockCount + ((h % 1000) * blocksPerBitmap + blockIdx); // Shift to avoid overlap
    }
    
public:
    BitmapIndex(int rows, int blockSize, int dataBlocks) 
    : numRows(rows), bitsPerBlock(blockSize), dataBlockCount(dataBlocks) {
        blocksPerBitmap = (numRows + bitsPerBlock - 1) / bitsPerBlock;
    }
    
    // Create a bitmap for a column value
    void createBitmap(const string& columnValue) {
        if (bitmaps.find(columnValue) != bitmaps.end()) {
            cout << "Bitmap for " << columnValue << " already exists" << endl;
            return;
        }
        
        // Initialize bitmap with all blocks set to false
        vector<vector<bool>> bitmap(blocksPerBitmap, vector<bool>(bitsPerBlock, false));
        bitmaps[columnValue] = bitmap;
    }
    
    // Set bits in memory only, to be flushed later
    void setBitBuffered(const string& columnValue, int rowId, bool value) {
        if (bitmaps.find(columnValue) == bitmaps.end()) {
            createBitmap(columnValue);
        }

        int blockIdx = getBlockForRow(rowId);
        int position = getPositionInBlock(rowId);
        bitmaps[columnValue][blockIdx][position] = value;
    }

    // After all bits are set, flush to disk once per bitmap
    void flushBitmapToDisk(const string& columnValue) {
        for (int blockIdx = 0; blockIdx < blocksPerBitmap; blockIdx++) {
            int physicalBlockId = getBitmapBlockId(columnValue, blockIdx);
            diskAccess(physicalBlockId); // Simulate writing this bitmap block
        }
    }
    


    
    // Execute equality check (query a column for specific value)
    vector<bool> equalityQuery(const string& columnValue) {
        if (bitmaps.find(columnValue) == bitmaps.end()) {
            cerr << "Error: Bitmap for " << columnValue << " not found" << endl;
            return vector<bool>();
        }
    
        cout << "Executing equality query: " << columnValue << endl;
        vector<bool> result(numRows, false);
    
        for (int blockIdx = 0; blockIdx < blocksPerBitmap; blockIdx++) {
            int physicalBlockId = getBitmapBlockId(columnValue, blockIdx);
            diskAccess(physicalBlockId); // Simulate reading bitmap block
    
            for (int pos = 0; pos < bitsPerBlock; pos++) {
                int rowId = blockIdx * bitsPerBlock + pos;
                if (rowId < numRows) {
                    result[rowId] = bitmaps[columnValue][blockIdx][pos];
                }
            }
        }
    
        // Simulate accessing only those data blocks whose bits are 1
        unordered_map<int, bool> accessedBlocks;
        for (int rowId = 0; rowId < numRows; rowId++) {
            if (result[rowId]) {
                int dataBlock = rowId / bitsPerBlock;
                if (!accessedBlocks[dataBlock]) {
                    diskAccess(dataBlock); // Read original table block
                    accessedBlocks[dataBlock] = true;
                }
            }
        }
    
        return result;
    }
    
    
    // Perform AND operation between two bitmaps
    vector<bool> bitmapAND(const string& columnValue1, const string& columnValue2) {
        if (bitmaps.find(columnValue1) == bitmaps.end() || 
            bitmaps.find(columnValue2) == bitmaps.end()) {
            cerr << "Error: One or both bitmaps not found" << endl;
            return vector<bool>();
        }
    
        cout << "Executing AND operation: " << columnValue1 << " AND " << columnValue2 << endl;
        vector<bool> result(numRows, false);
        int physicalBlockId1 = getBitmapBlockId(columnValue1, 0);
        int physicalBlockId2 = getBitmapBlockId(columnValue2, 0);
        for (int i = 0; i < blocksPerBitmap; i++) {
            diskAccess(physicalBlockId1);
        }
        for (int i = 0; i < blocksPerBitmap; i++) {
            diskAccess(physicalBlockId2);
        }

        for (int blockIdx = 0; blockIdx < blocksPerBitmap; blockIdx++) {
            int physicalBlockId1 = getBitmapBlockId(columnValue1, blockIdx);
            int physicalBlockId2 = getBitmapBlockId(columnValue2, blockIdx);
            for (int pos = 0; pos < bitsPerBlock; pos++) {
                int rowId = blockIdx * bitsPerBlock + pos;
                if (rowId < numRows) {
                    result[rowId] = bitmaps[columnValue1][blockIdx][pos] && 
                                    bitmaps[columnValue2][blockIdx][pos];
                }
            }
        }
        // Simulate reading only data blocks where result bit is 1
        for (int rowId = 0; rowId < numRows; rowId++) {
            if (result[rowId]) {
                int dataBlock = rowId / bitsPerBlock;
                diskAccess(dataBlock);
            }
        }
    
        return result;
    }
    
    
    // Perform OR operation between two bitmaps
    vector<bool> bitmapOR(const string& columnValue1, const string& columnValue2) {
        if (bitmaps.find(columnValue1) == bitmaps.end() || 
            bitmaps.find(columnValue2) == bitmaps.end()) {
            cerr << "Error: One or both bitmaps not found" << endl;
            return vector<bool>();
        }
    
        cout << "Executing OR operation: " << columnValue1 << " OR " << columnValue2 << endl;
        vector<bool> result(numRows, false);
    
        int physicalBlockId1 = getBitmapBlockId(columnValue1, 0);
        int physicalBlockId2 = getBitmapBlockId(columnValue2, 0);
        for (int i = 0; i < blocksPerBitmap; i++) {
            diskAccess(physicalBlockId1);
        }
        for (int i = 0; i < blocksPerBitmap; i++) {
            diskAccess(physicalBlockId2);
        }

        for (int blockIdx = 0; blockIdx < blocksPerBitmap; blockIdx++) {
            int physicalBlockId1 = getBitmapBlockId(columnValue1, blockIdx);
            int physicalBlockId2 = getBitmapBlockId(columnValue2, blockIdx);
            for (int pos = 0; pos < bitsPerBlock; pos++) {
                int rowId = blockIdx * bitsPerBlock + pos;
                if (rowId < numRows) {
                    result[rowId] = bitmaps[columnValue1][blockIdx][pos] || 
                                    bitmaps[columnValue2][blockIdx][pos];
                }
            }
        }
        // Simulate reading only data blocks where result bit is 1
        for (int rowId = 0; rowId < numRows; rowId++) {
            if (result[rowId]) {
                int dataBlock = rowId / bitsPerBlock;
                diskAccess(dataBlock);
            }
        }
    
        return result;
    }
    
    
    // Get matching rows from a result bitmap
    vector<int> getMatchingRows(const vector<bool>& bitmap) {
        vector<int> rows;
        for (int i = 0; i < bitmap.size(); i++) {
            if (bitmap[i]) {
                rows.push_back(i);
            }
        }
        return rows;
    }
    
    // Print a bitmap in readable format
    void printBitmap(const vector<bool>& bitmap) {
        cout << "Bitmap: ";
        for (bool bit : bitmap) {
            cout << (bit ? "1" : "0");
        }
        cout << endl;
    }
};

int main() {
    // Create example student data (from the sample in search results)
    vector<string> names = {"Geeta Raj", "Deep Singh", "Ria Sharma", "Ajit Singh", "Jitu Bagga", "Neha Kapoor"};
    vector<string> genders = {"F", "M", "F", "M", "M", "F"};
    vector<string> results = {"Fail", "Fail", "Pass", "Fail", "Pass", "Pass"};
    int numRows = names.size();
    int bitsPerBlock = 3; // 3 bits per block for simulation

    int rowsPerBlock = 3;
    int dataBlockCount = (numRows + rowsPerBlock - 1) / rowsPerBlock;
    int dataBlockStart = 0;

    
    // Print sample table
    cout << "==== Student Table ====" << endl;
    cout << "ID | Name        | Gender | Result" << endl;
    cout << "---|-------------|--------|-------" << endl;
    for (int i = 0; i < numRows; i++) {
        printf("%2d | %-11s | %-6s | %-5s\n", i+1, names[i].c_str(), genders[i].c_str(), results[i].c_str());
    }
    cout << endl;
    
    // Create bitmap index
    cout << "==== Bitmap Index Creation Phase ====" << endl;
    BitmapIndex bIndex(numRows, bitsPerBlock, dataBlockCount);
    
    // Track metrics for index creation
    seekCount = 0;
    transferCount = 0;
    currentBlock = -10;
    
    // Simulate loading table once (assume 3 rows per block for simplicity)
    loadDataBlocks(numRows, 3);

    // Set and flush bitmap for Gender=F
    for (int i = 0; i < numRows; i++) {
        bIndex.setBitBuffered("Gender=F", i, genders[i] == "F");
    }
    bIndex.flushBitmapToDisk("Gender=F");

    // Set and flush bitmap for Gender=M
    loadDataBlocks(numRows, 3);
    for (int i = 0; i < numRows; i++) {
        bIndex.setBitBuffered("Gender=M", i, genders[i] == "M");
    }
    bIndex.flushBitmapToDisk("Gender=M");

    // Set and flush bitmap for Result=Pass
    loadDataBlocks(numRows, 3);
    for (int i = 0; i < numRows; i++) {
        bIndex.setBitBuffered("Result=Pass", i, results[i] == "Pass");
    }
    bIndex.flushBitmapToDisk("Result=Pass");

    // Set and flush bitmap for Result=Fail
    loadDataBlocks(numRows, 3);
    for (int i = 0; i < numRows; i++) {
        bIndex.setBitBuffered("Result=Fail", i, results[i] == "Fail");
    }
    bIndex.flushBitmapToDisk("Result=Fail");
    
    cout << "\nIndex creation metrics:" << endl;
    cout << "Disk seeks: " << seekCount << endl;
    cout << "Block transfers: " << transferCount << endl;
    
    // Reset metrics for query execution
    seekCount = 0;
    transferCount = 0;
    currentBlock = 0;
    
    // Execute query: Find female students who passed
    cout << "\n==== Query Execution: Female Students who Passed ====" << endl;
    vector<bool> queryResult = bIndex.bitmapAND("Gender=F", "Result=Pass");
    
    cout << "\nQuery result: ";
    bIndex.printBitmap(queryResult);
    
    vector<int> matchingRows = bIndex.getMatchingRows(queryResult);
    cout << "Matching students:" << endl;
    for (int rowId : matchingRows) {
        cout << "  " << names[rowId] << " (Row " << rowId+1 << "), " ;
    }
    
    cout << "\nQuery execution metrics:" << endl;
    cout << "Disk seeks: " << seekCount << endl;
    cout << "Block transfers: " << transferCount << endl;

    // Reset metrics for query execution
    seekCount = 0;
    transferCount = 0;
    currentBlock = 0;
    
    // Execute query: Find female students who passed
    cout << "\n==== Query Execution: Male Students or Failed ====" << endl;
    vector<bool> queryResult2 = bIndex.bitmapOR("Gender=M", "Result=Fail");
    
    cout << "\nQuery result: ";
    bIndex.printBitmap(queryResult2);
    
    vector<int> matchingRows2 = bIndex.getMatchingRows(queryResult2);
    cout << "Matching students:" << endl;
    for (int rowId : matchingRows2) {
        cout << "  " << names[rowId] << " (Row " << rowId+1 << "), " ;
    }
    
    cout << "\nQuery execution metrics:" << endl;
    cout << "Disk seeks: " << seekCount << endl;
    cout << "Block transfers: " << transferCount << endl;
    
    return 0;
}