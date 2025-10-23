#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <climits>

using namespace std;

struct CacheLine {
    bool valid = false;
    unsigned int tag = 0;
    unsigned long lastUsed = 0; // used for LRU tracking
};

int main(int argc, char **argv) {
    if (argc != 7) {
        cerr << "Usage: ./csim <num_sets> <blocks_per_set> <block_size> <write-allocate|no-write-allocate> <write-through|write-back> <lru|fifo>\n";
        return 1;
    }

    int numSets = stoi(argv[1]);
    int blocksPerSet = stoi(argv[2]);
    int blockSize = stoi(argv[3]);
    string writeAlloc = argv[4];
    string writePolicy = argv[5];
    string evictPolicy = argv[6];

    // ---- Basic validation ----
    auto isPowerOfTwo = [](int n) { return n > 0 && (n & (n - 1)) == 0; };
    if (!isPowerOfTwo(numSets) || !isPowerOfTwo(blocksPerSet) || !isPowerOfTwo(blockSize)) {
        cerr << "Error: all size parameters must be powers of 2.\n";
        return 1;
    }
    if (blockSize < 4) {
        cerr << "Error: block size must be >= 4 bytes.\n";
        return 1;
    }
    if (writeAlloc == "no-write-allocate" && writePolicy == "write-back") {
        cerr << "Error: no-write-allocate cannot be used with write-back.\n";
        return 1;
    }

    bool isWriteAlloc = (writeAlloc == "write-allocate");
    bool isWriteBack = (writePolicy == "write-back");
    bool useLRU = (evictPolicy == "lru");

    // ---- Initialize cache ----
    vector<vector<CacheLine>> cache(numSets, vector<CacheLine>(blocksPerSet));

    unsigned long totalLoads = 0, totalStores = 0;
    unsigned long loadHits = 0, loadMisses = 0;
    unsigned long storeHits = 0, storeMisses = 0;
    unsigned long cycles = 0, timeCounter = 0;

    string op;
    unsigned int addr, dummy;

    // ---- Read trace ----
    while (cin >> op >> std::hex >> addr >> dummy) {
        unsigned int blockOffsetBits = log2(blockSize);
        unsigned int setBits = log2(numSets);
        unsigned int setIndex = (addr >> blockOffsetBits) & ((1 << setBits) - 1);
        unsigned int tag = addr >> (blockOffsetBits + setBits);

        vector<CacheLine> &set = cache[setIndex];
        bool hit = false;
        int emptyIndex = -1;
        int evictIndex = 0;
        unsigned long oldestTime = ULONG_MAX;

        // search for hit or LRU candidate
        for (int i = 0; i < blocksPerSet; i++) {
            if (set[i].valid && set[i].tag == tag) {
                hit = true;
                if (useLRU) set[i].lastUsed = timeCounter;
                break;
            }
            if (!set[i].valid && emptyIndex == -1) emptyIndex = i;
            if (set[i].lastUsed < oldestTime) {
                oldestTime = set[i].lastUsed;
                evictIndex = i;
            }
        }
        timeCounter++;

        // ---- Simulate load/store ----
        if (op == "l") {
            totalLoads++;
            if (hit) {
                loadHits++;
                cycles += 1;
            } else {
                loadMisses++;
                cycles += 100 * (blockSize / 4); // main memory read
                int target = (emptyIndex != -1) ? emptyIndex : evictIndex;
                set[target].valid = true;
                set[target].tag = tag;
                set[target].lastUsed = timeCounter;
            }
        } 
        else if (op == "s") {
            totalStores++;
            if (hit) {
                storeHits++;
                cycles += 1;
            } else {
                storeMisses++;
                cycles += 100 * (blockSize / 4);
                if (isWriteAlloc) {
                    int target = (emptyIndex != -1) ? emptyIndex : evictIndex;
                    set[target].valid = true;
                    set[target].tag = tag;
                    set[target].lastUsed = timeCounter;
                }
            }
        }
    }

    // ---- Output results ----
    cout << "Total loads: " << totalLoads << "\n";
    cout << "Total stores: " << totalStores << "\n";
    cout << "Load hits: " << loadHits << "\n";
    cout << "Load misses: " << loadMisses << "\n";
    cout << "Store hits: " << storeHits << "\n";
    cout << "Store misses: " << storeMisses << "\n";
    cout << "Total cycles: " << cycles << "\n";

    return 0;
}
