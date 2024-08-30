/**
 * @author Gautam Singh (CS21BTECH11018)
 * @file Assgn2-Src-CS21BTECH11018.cpp
 * @brief C++ source for a multithreaded solution to compute the sparsity of a
 * matrix using various techniques and libraries. The sparsity of a matrix is
 * defined as the number of zero entries of a matrix.
 *  
 * Techniques implemented: Chunk, Mixed, Dynamic.
 * Libraries used: pthreads, OpenMP
 * 
 * @date 2024-08-29
 */

// Headers

#include <iostream>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cmath>
#include <map>
#include <functional>
#include <pthread.h>
#include <omp.h>
#include <cassert>

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

// Global variables

uint64_t N, S, K, rowInc;
std::vector<std::vector<int>> A;
/// @brief Array of ThreadInfo structs for collection and output of results
std::vector<ThreadInfo> threadInfos;
/// @brief Counter for dynamic methods only
Counter<uint64_t> counter(0);

// Pthreads runners

/**
 * @brief Pthreads runner function for chunk technique.
 * @param arg Thread information.
 * @return Populated result in `thInfo`.
 */
void *pthreads_chunkRunner(void *arg) {
    ThreadInfo *thInfo = (ThreadInfo *)arg;
    // Remaining i.e., N % K rows are to be given to first N % K threads.
    // Find starting row as id * (N / K) + min(id, N % K);
    uint64_t l = thInfo->id * (N / K) + std::min(thInfo->id, N % K);
    for (uint64_t i = l; i < std::min(N, l + N / K + (thInfo->id < N % K)); i++) {
        for (auto &u : A[i]) thInfo->res += !u;
    }
    return NULL;
}

/**
 * @brief Pthreads runner function for mixed technique.
 * @param arg Thread information.
 * @return Populated result in `thInfo`.
 */
void *pthreads_mixedRunner(void *arg) {
    ThreadInfo *thInfo = (ThreadInfo *)arg;
    for (uint64_t i = thInfo->id; i < N; i += K) {
        for (auto &u : A[i]) thInfo->res += !u;
    }
    return NULL;
}

/**
 * @brief Pthread runner function for dynamic technique.
 * @param arg Thread information.
 * @return Populated result in `thInfo`.
 */
void *pthreads_dynamicRunner(void *arg) {
    ThreadInfo *thInfo = (ThreadInfo *)arg;
    // Attempt to get a new row
    while (counter.get() < N) {
        // Acquire row and increment
        uint64_t r = counter.getAndIncrement(rowInc);
        for (uint64_t i = r; i < std::min(r + rowInc, N); i++) {
            for (auto &u : A[i]) thInfo->res += !u;
        }
    } 
    return NULL;
}

// OpenMP runners

/**
 * @brief OpenMP runner function for chunk technique.
 * @param arg Unused.
 * @return Populated result in `thInfo`.
 */
void *omp_chunkRunner(void *arg) {
    #pragma omp parallel for schedule(static)
    for (uint64_t i = 0; i < N; i++) {
        for (auto &u : A[i]) threadInfos[omp_get_thread_num()].res += !u;
    }
    return NULL;
}

/**
 * @brief OpenMP runner function for mixed technique.
 * @param arg Unused.
 * @return Populated result in `thInfo`.
 */
void *omp_mixedRunner(void *arg) {
    #pragma omp parallel for schedule(static, 1)
    for (uint64_t i = 0; i < N; i++) {
        for (auto &u : A[i]) threadInfos[omp_get_thread_num()].res += !u;
    }
    return NULL;
}

/**
 * @brief OpenMP runner function for dynamic technique.
 * @param arg Unused.
 * @return Populated result in `thInfo`.
 */
void *omp_dynamicRunner(void *arg) {
    #pragma omp parallel for schedule(dynamic, K)
    for (uint64_t i = 0; i < N; i++) {
        assert(threadInfos.size() == K);
        for (auto &u : A[i]) threadInfos[omp_get_thread_num()].res += !u;
    }
    return NULL;
}

// Constants

/// @brief Input file
const char* INFILE = "inp.txt";
/// @brief Output file
const char* OUTFILE = "out.txt";
/// @brief Supported runners
std::unordered_map<std::string, void *(*)(void *)> supportedRunners {
    {"pthreads_chunk", pthreads_chunkRunner},
    {"pthreads_mixed", pthreads_mixedRunner},
    {"pthreads_dynamic", pthreads_dynamicRunner},
    {"omp_chunk", omp_chunkRunner},
    {"omp_mixed", omp_mixedRunner},
    {"omp_dynamic", omp_dynamicRunner},
};

/**
 * @brief Function to print program help.
 * @param name Name of executable, usually argv[0].
 */
void help(std::string name) {
    std::cerr << "Usage: " << name << " [options]\n\n"
              << "Options:\n"
              << "  -h,--help                            Display this information\n"
              << "  -t,--technique {chunk|mixed|dynamic} Use the specified technique for computation\n"
              << "  -l,--library   {pthreads|omp}        Use the specified library for computation"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse options
    if (argc < 2) {
        help(argv[0]);
        return 1;
    }
    // Runner function to use in threads
    std::string tech = "", lib = "";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            help(argv[0]);
            return 0;
        } else if (arg == "-t" || arg == "--technique") {
            i++;
            tech = argv[i];
        } else if (arg == "-l" || arg == "--library") {
            i++;
            lib = argv[i];
        } else {
            help(argv[0]);
            return 1;
        }
    }
    // Validate input args
    std::string fn = lib + "_" + tech;
    if (supportedRunners.find(fn) == supportedRunners.end()) {
        std::cerr << "[ERROR] Unsupported runner " << fn << std::endl;
        return 1;
    }
    // Setup input and output filestreams
    std::cin.tie(0)->sync_with_stdio(0);
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
    A.assign(N, std::vector<int> (N));
    for (auto &u : A) for (auto &v : u) std::cin >> v;
    threadInfos.assign(K, {});
    for (uint64_t i = 0; i < K; i++) threadInfos[i] = {i, 0};
    // Set up runner function
    void *(*runner) (void *) = supportedRunners[fn];
    // Start timer
    auto startTime = std::chrono::high_resolution_clock::now();
    if (lib == "pthreads") {
        // Create threads
        std::vector<pthread_t> threads(K);
        for (uint64_t i = 0; i < K; i++) pthread_create(&threads[i], NULL, runner, (void *)&threadInfos[i]);
        for (uint64_t i = 0; i < K; i++) pthread_join(threads[i], NULL);
    } else if (lib == "omp") {
        // Set number of threads
        omp_set_num_threads(K);
        // Call runner
        runner(NULL);
    }
    // Stop timer
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