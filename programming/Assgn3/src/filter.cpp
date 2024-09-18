/**
 * @author Gautam Singh
 * @file Filter-CS21BTECH11018.cpp
 * @brief C++ source for implementing the Bakery Lock and testing it on a
 * multithreaded application. In this application, critical section and waiting
 * for each thread are simulated by exponential delays, with their own averages. 
 *
 * @date 2024-09-16
 */

// Headers
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <utility>
#include <string>
#include <random>
#include <thread>

// Classes and structs

/**
 * @class FilterLock
 * @brief An implementation of the filter lock. Threads become victims at levels
 * 1 to n - 1, and wait for threads at higher levels to acquire and release the
 * lock.
 */
class FilterLock {
private:
    size_t n;                    /// Number of threads
    std::vector<size_t> level;   /// Level of each thread
    std::vector<size_t> victim;  /// Victim at each level
public:
    /**
     * @brief Constructor method for BakeryLock
     * @param N Number of threads
     */
    FilterLock(size_t _n) : n(_n) {
        level.assign(n, 0);
        victim.assign(n, -1);
    }

    /**
     * @brief Method to acquire FilterLock
     * @param id Thread ID
     */
    void lock(int id) {
        for (int i = 1; i < n; i++) {   // Attempt to enter level i
            level[id] = i;
            victim[i] = id;
            // Waiting condition
            for (int j = 0; j < n; j++) {
                while ((j != id) && (level[j] >= i && victim[i] == id));
            }
        }
    }

    /**
     * @brief Method to release BakeryLock
     * @param id Thread ID
     */
    void unlock(int id) {
        level[id] = 0;
    }
};

// Global variables
int n, k;
double lambda_1, lambda_2;
std::exponential_distribution exp_dist_1, exp_dist_2;
std::mt19937 gen;
std::chrono::time_point<std::chrono::system_clock> start;

// Constants

/// @brief Input file
const char *INFILE = "inp-params.txt";
/// @brief Output file
const char *OUTFILE = "out.txt";

// Runner functions

void testCS(int id, FilterLock &lock, std::stringstream &log) {
    for (int i = 0; i < k; i++) {
        // Log CS entry request
        log << "CS Entry Request " << i + 1 << " at "
        << (std::chrono::system_clock::now() - start).count()
        << " ns by thread " << id + 1 << '\n';
        // Acquire lock
        lock.lock(id);
        // Log CS entry
        log << "CS Entry " << i + 1 << " at "
        << (std::chrono::system_clock::now() - start).count() 
        << " ns by thread " << id + 1 << '\n';
        // Sleep in CS
        std::this_thread::sleep_for(std::chrono::milliseconds((int)exp_dist_1(gen)));
        // Log CS exit request
        log << "CS Exit Request " << i + 1 << " at "
        << (std::chrono::system_clock::now() - start).count() 
        << " ns by thread " << id + 1 << '\n';
        // Release lock
        lock.unlock(id);
        // Log CS exit
        log << "CS Exit " << i + 1 << " at "
        << (std::chrono::system_clock::now() - start).count() 
        << " ns by thread " << id + 1 << '\n';
        // Wait before next entry
        std::this_thread::sleep_for(std::chrono::milliseconds((int)exp_dist_2(gen)));
    }
}

int main() {
    // Set up file IO streams
    std::fstream fin(INFILE, std::fstream::in), fout(OUTFILE, std::fstream::out);
    // Read parameters
    fin >> n >> k >> lambda_1 >> lambda_2;
    // Set up distributions and random number generator
    exp_dist_1 = std::exponential_distribution(lambda_1);
    exp_dist_2 = std::exponential_distribution(lambda_2);
    gen = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());
    // Set up logging
    std::vector<std::stringstream> thread_logs(n); 
    // Set up lock
    FilterLock lock(n);
    std::vector<std::thread> runner_threads(n);
    // Start timer
    start = std::chrono::system_clock::now();
    // Create threads
    for (int i = 0; i < n; i++) runner_threads[i] = std::thread(testCS, i, std::ref(lock), std::ref(thread_logs[i]));
    // Join threads
    for (int i = 0; i < n; i++) runner_threads[i].join();
    // End timer
    auto end = std::chrono::system_clock::now();
    // Write to output file
    for (auto &ss : thread_logs) fout << ss.str();
    // Write program end log
    fout << "Program ended at " << (end - start).count() << " ns\n";
    return 0;
}