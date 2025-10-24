#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <climits>

using namespace std;

// cache's single block
struct CacheLine {
    bool valid = false; // does line contain valid data
    bool dirty = false; // dirty block or not
    unsigned int tag = 0; // tag bits so we know which memory block stored
    unsigned long lastUsed = 0; // used for LRU tracking
};

int main(int argc, char **argv) {
    // program should have 6 arguments and the program name
    if (argc != 7) {
        cerr << "Usage: ./csim <num_sets> <blocks_per_set> <block_size> <write-allocate|no-write-allocate> <write-through|write-back> <lru|fifo>\n";
        return 1;
    }

    // var declarations for the command line args above
    int numSets = stoi(argv[1]);
    int blocksPerSet = stoi(argv[2]);
    int blockSize = stoi(argv[3]);
    string writeAlloc = argv[4];
    string writePolicy = argv[5];
    string evictPolicy = argv[6];

    // here is our validation checking
    // using the following bit trick check for powers of two
    auto isPowerOfTwo = [](int n) { return n > 0 && (n & (n - 1)) == 0; };
    if (!isPowerOfTwo(numSets) || !isPowerOfTwo(blocksPerSet) || !isPowerOfTwo(blockSize)) {
        cerr << "Error: all size parameters must be powers of 2.\n";
        return 1;
    }
    
    // since accesses are <= 4 bytes, block size >= 4 bytes check
    if (blockSize < 4) {
        cerr << "Error: block size must be >= 4 bytes.\n";
        return 1;
    }
    
    // when write-back combined with no-write-allocate this is an illegal configuration, check
    if (writeAlloc == "no-write-allocate" && writePolicy == "write-back") {
        cerr << "Error: no-write-allocate cannot be used with write-back.\n";
        return 1;
    }

    // take the configuration strings and turn into boolean flags
    bool isWriteAlloc = (writeAlloc == "write-allocate");
    bool useLRU = (evictPolicy == "lru");

    // Initialize cache
    // make a 2d vector cache here dependent on numSets and blocksPerSet
    vector<vector<CacheLine>> cache(numSets, vector<CacheLine>(blocksPerSet));

    // counters for the to-be-calculated statistics
    unsigned long totalLoads = 0;
    unsigned long totalStores = 0;
    unsigned long loadHits = 0;
    unsigned long loadMisses = 0;
    unsigned long storeHits = 0;
    unsigned long storeMisses = 0;
    unsigned long cycles = 0;
    unsigned long timeCounter = 0; // this increments after an access

    string op;
    unsigned int addr;
    unsigned int dummy;

    // read the memory trace with stdin
    // lines have form <op> <hex address> <ignored field>
    while (cin >> op >> std::hex >> addr >> dummy) {
        // bit manipulation to calc the index and tag
        unsigned int blockOffsetBits = log2(blockSize);
        unsigned int setBits = log2(numSets);
        unsigned int setIndex = (addr >> blockOffsetBits) & ((1 << setBits) - 1);
        unsigned int tag = addr >> (blockOffsetBits + setBits);

        vector<CacheLine> &set = cache[setIndex];
        bool hit = false;
        int hitIndex = -1; // to track which line is hit
        int emptyIndex = -1; // here to track first empty slot
        int evictIndex = 0; // if full then this is index of to-be-evicted block
        unsigned long oldestTime = ULONG_MAX; // use for finding most recently used block

        // iterate and search for hit or even possible victim line for eviction
        for (int i = 0; i < blocksPerSet; i++) {
            // cache hits
            if (set[i].valid && set[i].tag == tag) { 
                hit = true;
                /*if (useLRU) {
                    set[i].lastUsed = timeCounter; // update the recency
                } */
                hitIndex = i;
                break;
            }
            
            // keep track of first empty line
            if (!set[i].valid && emptyIndex == -1) {
                emptyIndex = i;
            }
            
            // for LRU eviction below helps by tracking oldest line 
            if (set[i].lastUsed < oldestTime) {
                oldestTime = set[i].lastUsed;
                evictIndex = i;
            }
        }
        timeCounter++; // timestamp has to increment for next access

        // code to handle the load operation
        if (op == "l") {
            totalLoads++;
            if (hit) {
                loadHits++;
                cycles += 1; // cache hit so you add a cycle
                if (useLRU) {
                    set[hitIndex].lastUsed = timeCounter;
                }
            } else {
                loadMisses++;
                cycles += 100 * (blockSize / 4) + 1; // cache miss so get block from memory
                
                // take this block just retrieved and insert into cache
                int target = (emptyIndex != -1) ? emptyIndex : evictIndex;

                if (emptyIndex == -1 && writePolicy == "write-back" && set[evictIndex].dirty) {
                    cycles += 100 * (blockSize / 4); // write back dirty block
                }

                set[target].valid = true;
                set[target].tag = tag;
                set[target].lastUsed = timeCounter;
                set[target].dirty = false;
            }
        } else if (op == "s") {
            totalStores++;
            if (hit) {
                storeHits++;
                if (writePolicy == "write-through") {
                    cycles += 1 + 100; // write cache and memory
                } else {
                    cycles += 1; // cache hit so you add a cycle
                    set[hitIndex].dirty = true; // mark dirty on write-back hit
                }
                if (useLRU) {
                    set[hitIndex].lastUsed = timeCounter;
                }
            } else {
                storeMisses++;
                if (isWriteAlloc) {
                    cycles += 100 * (blockSize / 4); // cache miss so get block from memory
                    
                    // take this block just retrieved and insert into cache
                    int target = (emptyIndex != -1) ? emptyIndex : evictIndex;

                    // this is write-back eviction penalty
                    if (emptyIndex == -1 && writePolicy == "write-back" && set[evictIndex].dirty) {
                        cycles += 100 * (blockSize / 4);
                    }

                    set[target].valid = true;
                    set[target].tag = tag;
                    set[target].lastUsed = timeCounter;

                    // write depending on policy
                    if (writePolicy == "write-through") {
                        cycles += 1 + 100;
                        set[target].dirty = false;
                    } else {
                        cycles += 1;
                        set[target].dirty = true;
                    }
                } else {
                    // no-write-allocate so write directly to memory
                    cycles += 100;
                }
            }
        }
    }

    // simply output the summary statistics calculated above
    cout << "Total loads: " << totalLoads << "\n";
    cout << "Total stores: " << totalStores << "\n";
    cout << "Load hits: " << loadHits << "\n";
    cout << "Load misses: " << loadMisses << "\n";
    cout << "Store hits: " << storeHits << "\n";
    cout << "Store misses: " << storeMisses << "\n";
    cout << "Total cycles: " << cycles << "\n";

    return 0;
}
