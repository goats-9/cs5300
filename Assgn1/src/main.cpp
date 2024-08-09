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

// Classes and structs

/**
 * @brief Information contained by a thread.
 */
struct ThreadInfo {
    int id;     /// Thread id
    int res;    /// Result of thread computation
};

// Constants

char* INFILE = "inp.txt";   /// Input file
char* OUTFILE = "out.txt";  /// Output file

// Global variables

int N, S, K, rowInc;
std::vector<std::vector<int>> A;
std::atomic<int> counter(0);

// Thread runners

/**
 * @brief Runner function for chunk technique.
 * @param thInfo Thread information.
 * @return Populated result in `thInfo`.
 */
void chunkRunner(ThreadInfo& thInfo) {
    // Remainder i.e., N % K rows are to be given to first N % K threads.
    // Find starting row as id * (N / K) + min(id, N % K);
    int l = thInfo.id * (N / K) + std::min(thInfo.id, N % K);
    for (int i = l; i < std::min(N, l + N / K + (thInfo.id < N % K)); i++) {
        for (auto &u : A[i]) thInfo.res += !u;
    }
}

/**
 * @brief Runner function for mixed technique.
 * @param thInfo Thread information.
 * @return Populated result in `thInfo`.
 */
void mixedRunner(ThreadInfo& thInfo) {
    for (int i = thInfo.id; i < N; i += K) {
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
    while (counter * rowInc < N) {
        // Acquire row and increment
        int r = counter++;
        for (int i = r * rowInc; i < std::min((r + 1) * rowInc, N); i++) {
            for (auto &u : A[i]) thInfo.res += !u;
        }
    } 
}

void help(std::string name) {
    std::cerr << "Usage: " << name << " [options]\n\n"
              << "Options:\n"
              << "\t-h,--help\t\tDisplay this information\n"
              << "\t-t,--technique {chunk|mixed|dynamic|block}\tUse the specified technique for computing matrix sparsity"
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
            else if (tech == "block");
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
    freopen(INFILE, "r", stdin);
    freopen(OUTFILE, "w", stdout);
    // Read inputs
    std::cin >> N >> S >> K >> rowInc;
    A.assign(N, std::vector<int> (N));
    for (auto &u : A) for (auto &v : u) std::cin >> v;
    // Set up threads and respective ThreadInfo structs to be passed
    std::vector<std::thread> threads(K);
    std::vector<ThreadInfo> threadInfos(K);
    for (int i = 0; i < K; i++) threadInfos[i].id = i, threadInfos[i].res = 0;
    // Start timer
    auto startTime = std::chrono::high_resolution_clock::now();
    // Run threads and join them
    for (int i = 0; i < K; i++) threads[i] = std::thread(runner, std::ref(threadInfos[i]));
    for (auto& th : threads) th.join();
    // Finish timer
    auto endTime = std::chrono::high_resolution_clock::now();
    auto tm = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    // Collect and output statistics
    std::cout << "Time taken to count the number of zeros: " << tm << "\n";
    int sm = 0;
    for (auto &thInfo : threadInfos) sm += thInfo.res;
    std::cout << "Total number of zero-valued elements in the matrix: " << sm << '\n';
    for (auto &thInfo : threadInfos) 
        std::cout << "Number of zero-valued elements counted by thread" 
                  << thInfo.id << ": " << thInfo.res << '\n';
    return 0;
}