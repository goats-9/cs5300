/**
 * @author Gautam Singh (CS21BTECH11018)
 * @file Assgn1-Src-CS21BTECH11018.cpp
 * @brief C++ source for a multithreaded solution to compute the sparsity of a
 * matrix. The sparsity of a matrix is defined as the number of zero entries of
 * a matrix.
 * Techniques implemented: Chunk, Mixed, Dynamic, Block.
 * @date 2024-08-08
 */

// Headers

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cmath>

// Classes and structs

/**
 * @brief Information contained by a thread.
 */
struct ThreadInfo {
    uint64_t id;    /// Thread id
    int res;        /// Result of thread computation
};

/**
 * @brief Counter class, implemented using std::atomic objects.
 */
template<class T>
class Counter {
    std::atomic<T> ctr;   /// 64-bit atomic unsigned counter.
public:
    /**
     * @brief Constuctor for Counter Class.
     * @param n Starting integer for counter, defaults to 0.
     */
    Counter(T n = 0) : ctr(n) {}

    /**
     * @brief Method to get current value of counter.
     * @return Current value of counter.
     */
    T get() { return ctr; }

    /**
     * @brief Method to get and post-increment counter atomically.
     * @param inc Amount to increase the counter by.
     * @return Counter value before increment.
     */
    T getAndIncrement(T inc = 1) { return ctr.fetch_add(inc); }
};

// Constants

const char* INFILE = "inp.txt";   /// Input file
const char* OUTFILE = "out.txt";  /// Output file

// Global variables

uint64_t N, S, K, rowInc, blockSize;
std::vector<std::vector<int>> A;
Counter<uint64_t> counter(0);    /// For dynamic methods only

// Thread runners

/**
 * @brief Runner function for chunk technique.
 * @param thInfo Thread information.
 * @return Populated result in `thInfo`.
 */
void chunkRunner(ThreadInfo& thInfo) {
    // Remaining i.e., N % K rows are to be given to first N % K threads.
    // Find starting row as id * (N / K) + min(id, N % K);
    uint64_t l = thInfo.id * (N / K) + std::min(thInfo.id, N % K);
    for (uint64_t i = l; i < std::min(N, l + N / K + (thInfo.id < N % K)); i++) {
        for (auto &u : A[i]) thInfo.res += !u;
    }
}

/**
 * @brief Runner function for mixed technique.
 * @param thInfo Thread information.
 * @return Populated result in `thInfo`.
 */
void mixedRunner(ThreadInfo& thInfo) {
    for (uint64_t i = thInfo.id; i < N; i += K) {
        for (auto &u : A[i]) thInfo.res += !u;
    }
}

/**
 * @brief Runner function for dynamic technique.
 * @param thInfo Thread information.
 * @return Populated result in `thInfo`.
 */
void dynamicRunner(ThreadInfo& thInfo) {
    // Attempt to get a new row
    while (counter.get() < N) {
        // Acquire row and increment
        uint64_t r = counter.getAndIncrement(rowInc);
        for (uint64_t i = r; i < std::min(r + rowInc, N); i++) {
            for (auto &u : A[i]) thInfo.res += !u;
        }
    } 
}

/**
 * @brief Runner function for dynamic block technique. We will have blocks of
 * size blockSize. Here, we consider blockSize = sqrt(N) for a total of N blocks
 * to make it comparable to other methods.
 * @param thInfo Thread information.
 * @return Populated result in `thInfo`.
 */
void dynamicBlockRunner(ThreadInfo& thInfo) {
    // Compute total number of blocks, which is ceil(N / blockSize) ^ 2.
    uint64_t numBlocks = (N + blockSize - 1) / blockSize;
    uint64_t totalBlocks = numBlocks * numBlocks;
    // Attempt to get a new block
    while (counter.get() < totalBlocks) {
        // Acquire block and increment
        uint64_t b = counter.getAndIncrement(rowInc);
        for (uint64_t i = b; i < std::min(b + rowInc, totalBlocks); i++) {
            // Compute (row, col) as (b / K, b % K);
            uint64_t row = i / numBlocks, col = i % numBlocks;
            // Now compute limits based on row and col, similar to the chunk case
            uint64_t rowl = row * (N / numBlocks) + std::min(row, N % numBlocks);
            uint64_t coll = col * (N / numBlocks) + std::min(col, N % numBlocks);
            for (uint64_t j = rowl; j < std::min(N, rowl + N / numBlocks + (row < N % numBlocks)); j++)
                for (uint64_t k = coll; k < std::min(N, coll + N / numBlocks + (col < N % numBlocks)); k++)
                    thInfo.res += !A[j][k];
        }
    }
}

void help(std::string name) {
    std::cerr << "Usage: " << name << " [options]\n\n"
              << "Options:\n"
              << "  -h,--help                                  Display this information\n"
              << "  -t,--technique {chunk|mixed|dynamic|block} Use the specified technique for computing matrix sparsity"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse options
    if (argc < 2) {
        help(argv[0]);
        return 1;
    }
    // Runner function to use in threads
    void (*runner) (ThreadInfo&) = NULL;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            help(argv[0]);
            return 0;
        } else if (arg == "-t" || arg == "--technique") {
            // Set up
            i++;
            std::string tech = argv[i];
            if (tech == "chunk") runner = chunkRunner;
            else if (tech == "mixed") runner = mixedRunner;
            else if (tech == "dynamic") runner = dynamicRunner;
            else if (tech == "block") runner = dynamicBlockRunner;
            else {
                help(argv[0]);
                return 1;
            }
        } else {
            help(argv[0]);
            return 1;
        }
    }
    // Setup input and output filestreams
    if (!freopen(INFILE, "r", stdin)) {
        std::cerr << "[ERROR] Opening input file " << INFILE << " failed: " 
                  << std::strerror(errno) << std::endl;
        return 1;
    }
    if (!freopen(OUTFILE, "w", stdout)) {
        std::cerr << "[ERROR] Opening output file " << OUTFILE << " failed: " 
                  << std::strerror(errno) << std::endl;
        return 1;
    }
    // Read inputs
    std::cin >> N >> S >> K >> rowInc;
    // Set up block size
    blockSize = int(sqrtl(N));
    A.assign(N, std::vector<int> (N));
    for (auto &u : A) for (auto &v : u) std::cin >> v;
    // Set up threads and respective ThreadInfo structs to be passed
    std::vector<std::thread> threads(K);
    std::vector<ThreadInfo> threadInfos(K);
    for (uint64_t i = 0; i < K; i++) threadInfos[i].id = i, threadInfos[i].res = 0;
    // Start timer
    auto startTime = std::chrono::high_resolution_clock::now();
    // Run threads and join them
    for (uint64_t i = 0; i < K; i++) threads[i] = std::thread(runner, std::ref(threadInfos[i]));
    for (auto& th : threads) th.join();
    // Finish timer
    auto endTime = std::chrono::high_resolution_clock::now();
    auto tm = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    // Collect and output statistics
    std::cout << "Time taken to count the number of zeros: " << tm.count() << " ms\n";
    int sm = 0;
    for (auto &thInfo : threadInfos) sm += thInfo.res;
    std::cout << "Total number of zero-valued elements in the matrix: " << sm << '\n';
    for (auto &thInfo : threadInfos) 
        std::cout << "Number of zero-valued elements counted by thread" 
                  << thInfo.id << ": " << thInfo.res << '\n';
    return 0;
}