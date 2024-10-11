/**
 * @author Gautam Singh
 * @file ofs-CS21BTECH11018.cpp
 * @brief C++ source for implementing and benchmarking MRMW obstruction-free
 * snapshot object. In this application, sleep times for each thread are
 * simulated by exponential delays, with their own averages. 
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
#include <atomic>
#include <unordered_map>
#include <unordered_set>

// Classes and structs

/**
 * @brief An implementation of a timestamped value. It contains a value and a
 * timestamp.
 */
template<class T>
struct StampedValue {
    T value;        // Value stored
    uint16_t stamp; // Timestamp
    uint16_t id;    // Thread id

    /**
     * @brief Constructor method for StampedValue.
     * @param val Value.
     * @param ts Timestamp.
     * @param tid Thread id.
     */
    StampedValue(T val, uint16_t ts = 0, uint16_t tid = 0) : value(val), stamp(ts), id(tid) {}

    bool operator == (const StampedValue<T> stval) const {
        return value == stval.value and stamp == stval.stamp and id == stval.id;
    }
};

template<class T> using A = std::atomic<StampedValue<T>>;
template<class T> using P = std::unique_ptr<A<T>>;

thread_local uint16_t sn = 0;

/**
 * @class Implementation of obstruction-free MRMW snapshot interface.
 */
template<typename T>
class OFSnapshot {
private:
    std::vector<P<T>> shArr;
public:
    OFSnapshot(int m) : shArr(m) { for (P<T> &u : shArr) u = std::make_unique<A<T>>(0); }
    
    void update(int l, T v) {
        // Get thread id
        uint16_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        // Replace with pointer to own thread id and sequence number.
        *shArr[l] = StampedValue<T>(v, ++sn, tid);
    }

    std::vector<StampedValue<T>> collect() {
        std::vector<StampedValue<T>> copy;
        for (auto &u : shArr) copy.push_back(*u);
        return copy;
    }

    std::vector<T> snapshot() {
        std::vector<StampedValue<T>> oldCopy, newCopy;
        oldCopy = collect();
        while (true) {
            newCopy = collect();
            if (oldCopy != newCopy) {
                std::swap(oldCopy, newCopy);
                continue;
            }
            std::vector<T> ret;
            for (StampedValue<T> u : newCopy) ret.push_back(u.value);
            return ret;
        }
    }
};

// Global variables
uint32_t M, nw, ns, k;
double lambda_w, lambda_s;
bool term;
std::uniform_int_distribution<uint32_t> locDist, valDist;
std::exponential_distribution<double> writerSleepDist, snapshotSleepDist;
std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

// Constants

/// @brief Name of the input file.
const char* INFILE = "inp-params.txt";
/// @brief Name of the output file.
const char* OUTFILE = "out.txt";

// Runner functions

template<class T>
void writerThreadRunner(uint16_t id, OFSnapshot<T> &snapObj, std::stringstream &log) {
    while (!term) {
        // Get l, v
        uint32_t l = locDist(rng);
        T v = valDist(rng);
        // Perform write
        auto writeStart = std::chrono::system_clock::now();
        snapObj.update(l, v);
        auto writeEnd = std::chrono::system_clock::now();
        // Log write with timestamp
        log << std::format("[{:%FT%TZ}]", writeEnd) 
            << " Writer thread " << id << ": shArr[" << l << "] = " << v 
            << " in " << (writeEnd - writeStart).count() << " ns.\n";
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds((int)writerSleepDist(rng)));
    }
}

template<class T>
void snapshotThreadRunner(uint16_t id, OFSnapshot<T> &snapObj, std::stringstream &log) {
    for (uint32_t i = 0; i < k; i++) {
        // Do the snapshot
        auto collectStart = std::chrono::system_clock::now();
        std::vector<T> snap = snapObj.snapshot();
        auto collectEnd = std::chrono::system_clock::now();
        // Log snapshot
        log << std::format("[{:%FT%TZ}]", collectEnd) 
            << " Snapshot thread " << id << ": collect " << i + 1 << " {";
        for (uint32_t j = 0; j < M; j++) log << " " << j << ": " << snap[j] << (",}"[j + 1 == M]);
        log << " in " << (collectEnd - collectStart).count() << " ns.\n";
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds((int)snapshotSleepDist(rng)));
    }
}

int main(int argc, char *argv []) {
    // Input parsing
    std::fstream fin(INFILE, std::fstream::in);
    if (!fin) {
        std::cerr << "[ERROR] Input file " << INFILE << " not found.\n";
        return 1;
    }
    std::fstream fout(OUTFILE, std::fstream::out);
    if (!fout) {
        std::cerr << "[ERROR] Could not create output file " << OUTFILE << ".\n";
        return 1;
    }
    fin >> nw >> ns >> M >> lambda_w >> lambda_s >> k;
    // Set up the generators
    locDist = std::uniform_int_distribution<uint32_t>(0, M - 1);
    valDist = std::uniform_int_distribution<uint32_t>();
    writerSleepDist = std::exponential_distribution<double>(lambda_w);
    snapshotSleepDist = std::exponential_distribution<double>(lambda_s);
    // Set up the termination flag
    term = false;
    // Create threads
    std::vector<std::thread> writerThreads(nw), snapshotThreads(ns);
    // Create loggers
    std::vector<std::stringstream> writerLogs(nw), snapshotLogs(ns);
    // Create WFS object
    OFSnapshot<uint32_t> snapObj(M);
    for (uint16_t i = 0; i < nw; i++) {
        writerThreads[i] = std::thread{writerThreadRunner<uint32_t>, i, std::ref(snapObj), std::ref(writerLogs[i])};
    }
    for (uint16_t i = 0; i < ns; i++) {
        snapshotThreads[i] = std::thread{snapshotThreadRunner<uint32_t>, i, std::ref(snapObj), std::ref(snapshotLogs[i])};
    }
    for (auto &th : snapshotThreads) th.join();
    term = true;
    for (auto &th : writerThreads) th.join();
    // Write logs to output
    for (auto &lg : writerLogs) fout << lg.str();
    for (auto &lg : snapshotLogs) fout << lg.str();
    return 0;
}