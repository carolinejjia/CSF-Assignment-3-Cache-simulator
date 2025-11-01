TODO: names of team members and their contributions to the project

Senuka Abeysinghe: implemented main cache logic, load/store handling, and also the LRU mechanism
Caroline Jia: dealed with input parsing, validation checks, and output verification, added FIFO 

TODO (for MS3): best cache report

Experiments and Results
1. Associativity Comparison

Goal: Examine how cache associativity affects hit rates and total cycles.
Configs Tested:

4-way associative: ./csim 256 4 16 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 314798
Load misses: 3399
Store hits: 188250
Store misses: 9236
Total cycles: 9344483


Direct-mapped: ./csim 1024 1 16 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 312238
Load misses: 5959
Store hits: 187502
Store misses: 9984
Total cycles: 11127283


Fully associative: ./csim 1 1024 16 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 314973
Load misses: 3224
Store hits: 188300
Store misses: 9186
Total cycles: 9226883

Findings:
As associativity increased, conflict misses decreased, leading to higher hit rates and lower overall cycle counts. The direct-mapped cache experienced the most misses (5,959 loads and 9,984 stores) and the highest total cycles (11.13 million), demonstrating limited benefit when multiple addresses mapped to the same set. The 4-way associative cache generated balanced results, achieving 314,798 load hits with only 3,399 misses and a moderate efficient total of 9.34 million cycles. This shows strong efficiency with decent replacement overhead. The fully associative cache achieved the fewest misses (3,224 loads, 9,186 stores) and lowest total cycles (9.23M), confirming its advantage in minimizing conflicts, though at the cost of more hardware complexity. While both 4-way and full associative mapping have similarly efficient cycle counts and miss rates, full associative mapping is less efficient hardware wise, as it requires 1024 tag lookups per access, while the 4-way only compares 4 tags per access. Therefore, overall 4-way associative mapping is the best choice.


2. Replacement Policy (LRU vs. FIFO)

Goal: Test how eviction strategies impact hit rate.
Configs Tested:

LRU: ./csim 512 2 16 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 314453
Load misses: 3744
Store hits: 188124
Store misses: 9362
Total cycles: 9588883

FIFO: ./csim 512 2 16 write-allocate write-back fifo < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 313943
Load misses: 4254
Store hits: 187935
Store misses: 9551
Total cycles: 10022883

Findings:
The LRU replacement policy did perform better than FIFO given its reduction in both the load and store misses. Specifically, we can see that LRU got 3744 load misses and 9362 store misses, compared to FIFO which had 4254 load misses and 9551 store misses. For these efficiency reasons, there was a lower total cycle count (9.59M vs. 10.02M). With LRU, these maintained blocks that were more frequently accessed so temporal locality was improved, however, FIFO was occasionally evicting data that was still active in use. Overall, LRU was definitely more effective in retaining useful cache entries and minimizing costly memory accesses.


3. Write Policy (Write-Back vs. Write-Through)

Goal: Measure cycle cost of different write strategies.
Configs Tested:

Write-back: ./csim 256 4 16 write-allocate write-back lru < ../traces/write02.trace
Total loads: 0
Total stores: 10
Load hits: 0
Load misses: 0
Store hits: 9
Store misses: 1
Total cycles: 410

Write-through: ./csim 256 4 16 write-allocate write-through lru < ../traces/write02.trace
Total loads: 0
Total stores: 10
Load hits: 0
Load misses: 0
Store hits: 9
Store misses: 1
Total cycles: 1410

Findings:
Write-back and write-through both generated identical hit and miss counts because they handle cache lookups the same. However, write-through ran significantly more cycles (1410 vs. 410) because it writes directly to main memory during each store operation whereas write-back delays updating main memory until the block is evicted. Write-back is more efficient, because it minimizes traffic and is more efficient for write-heavy traces.


4. Write-Allocate vs. No-Write-Allocate

Goal: Determine whether fetching on store misses improves efficiency.
Configs Tested:

Write-allocate: ./csim 64 8 16 write-allocate write-back lru < ../traces/write02.trace
Total loads: 0
Total stores: 10
Load hits: 0
Load misses: 0
Store hits: 9
Store misses: 1
Total cycles: 410

No-write-allocate: ./csim 64 8 16 no-write-allocate write-through lru < ../traces/write02.trace
Total loads: 0
Total stores: 10
Load hits: 0
Load misses: 0
Store hits: 0
Store misses: 10
Total cycles: 1000

Findings:
The write-allocate policy very much outperformed no-write-allocate because there was a reduction in both misses and total execution cycles. As you can see with 9 store hits compared to 0 for no-write-allocate and the total cycles dropping from 1000 to 410, this reduction really does exist. Therefore, this shows that fetching a block into the cache on store miss allows subsequent writes to be completed more fast. Therefore, write-allocate is more efficient for workloads that have repeated writes to same mem locations and no-write-allocates waste cycles by writing directly to memory each time.


5. Block Size and Cache Size Scaling

Goal: Observe how changing block size affects cycle cost and hit rates while keeping cache capacity constant.
Configs Tested:

8 byte block: ./csim 512 4 8 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 313390
Load misses: 4807
Store hits: 179658
Store misses: 17828
Total cycles: 8534483

32 byte block: ./csim 128 4 32 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 315689
Load misses: 2508
Store hits: 192637
Store misses: 4849
Total cycles: 10616483

64 byte block: ./csim 64 4 64 write-allocate write-back lru < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 316157
Load misses: 2040
Store hits: 194821
Store misses: 2665
Total cycles: 13050083

Findings:
Load and store misses decreased as block size increased, but total cycles increased because of higher miss penalty and longer memory transfer time. The 8-byte block had the most misses (4,807 loads, 17,828 stores) but the lowest total cycles (8.53 million), while the 64-byte block reduced misses to 2,040 loads and 2,665 stores but required over 13 million cycles. The 32-byte block provided balanced results, with a strong hit rate and moderate cycle cost.


6. Extreme Cases

Goal: Test functional limits of smallest and largest caches.
Configs Tested:

Tiny Cache: ./csim 1 1 4 write-allocate write-back fifo < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 13415
Load misses: 304782
Store hits: 13556
Store misses: 183930
Total cycles: 68772983

Huge Cache: ./csim 8192 1 16 write-allocate write-back fifo < ../traces/gcc.trace
Total loads: 318197
Total stores: 197486
Load hits: 315204
Load misses: 2993
Store hits: 188486
Store misses: 9000
Total cycles: 6626883

Findings:
We see that cache size has a pretty big effect on the performance. With the tiny cache of 1 set and 1 block, there was some severe thrashing given with only 13415 load hits and over 68 million cycles due to frequent misses. However, with the huge cache (8192 sets), this achieved 315204 load hits and just 6.6 million cycles, as most data fit easily. Therefore, we know that with larger caches these can greatly reduce the misses and cycles until it reaches the point of diminishing returns, while very small caches perform bad under heavy workloads.


Conclusion
Moderate associativity (4-way), moderate block size (32 byte), LRU replacement, write-back, and write-allocate resulted in best cache performance overall. These selections led to lower miss rates and more efficient use of memory. In comparison, full associative caches and large block sizes demanded more cycle cost. Direct-mapped caches were more efficient per access but incurred more misses

