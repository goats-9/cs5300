/**
 * @author Gautam Singh
 * @file ofs-CS21BTECH11018.cpp
 * @brief C++ source for implementing and benchmarking MRMW obstruction-free
 * snapshot object. In this application, sleep times for each thread are
 * simulated by exponential delays, with their own averages. 
 *
 * @date 2024-10-18
 */

// Headers
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <algorithm>
#include <utility>
#include <string>
#include <random>
#include <thread>
#include <atomic>

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

/**
 * @brief A struct containing relevant information carried by a log.
 */
struct Log {
    int id; // Thread id
    std::chrono::system_clock::time_point tm; // Time point
    std::string lg; // Log entry

    /**
     * @brief Constructor function for `Log`.
     * @param id Thread id
     * @param tm Time point
     * @param lg Log entry
     */
    Log(int id, std::chrono::system_clock::time_point &tm, std::string &lg) : id(id), tm(tm), lg(lg) {}

    /**
     * @brief Compares logs using the tuple (tm, id).
     */
    bool operator< (const Log& l) const {
        return std::make_pair(tm, id) < std::make_pair(l.tm, l.id);
    }

    /**
     * @brief Output a log entry.
     */
    friend std::ostream& operator<< (std::ostream& o, const Log& l) {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(l.tm.time_since_epoch()).count() % 1000000000;
        auto t = std::chrono::system_clock::to_time_t(l.tm);
        return o << std::put_time(localtime(&t), "[%FT%T") << '.' << ns << "Z] " << l.lg << '\n';
    }
};

thread_local uint16_t sn = 0;

/**
 * @class Implementation of obstruction-free MRMW snapshot interface.
 * @param m Size of the shared array.
 */
template<typename T>
class OFSnapshot {
private:
    std::vector<P<T>> shArr;
public:
    OFSnapshot(int m) : shArr(m) { for (auto &u : shArr) u = std::make_unique<A<T>>(0); }
    
    /**
     * @brief Set the value at memory location `l` to `v`.
     * @param l Location whose value is to be updated.
     * @param v New value to be inserted at location `l`.
     */
    void update(int l, T v) {
        // Get thread id
        uint16_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        // Replace with pointer to own thread id and sequence number.
        *shArr[l] = StampedValue<T>(v, ++sn, tid);
    }

    /**
     * @brief Helper function to collect the contents of the shared array.
     * @return Array of stamped values representing the contents of the shared
     * array.
     */
    std::vector<StampedValue<T>> collect() {
        std::vector<StampedValue<T>> copy;
        for (auto &u : shArr) copy.push_back(*u);
        return copy;
    }

    /**
     * @brief Return a linearizable snapshot of the shared array.
     * @return Snapshot consisting of the values stored in the shared array
     * which is linearizable within the interval of this function.
     */
    std::vector<T> snapshot() {
        std::vector<StampedValue<T>> oldCopy, newCopy;
        // Perform initial collect
        oldCopy = collect();
        while (true) {
            // Second collect
            newCopy = collect();
            if (oldCopy != newCopy) {
                // Swap old and new collects
                std::swap(oldCopy, newCopy);
                // Attempt second collect again
                continue;
            }
            // We have a clean collect, this is our snapshot
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
void writerThreadRunner(uint16_t id, OFSnapshot<T> &snapObj, std::vector<Log> &log) {
    std::stringstream ss;
    while (!term) {
        // Get l, v
        uint32_t l = locDist(rng);
        T v = valDist(rng);
        // Perform write
        auto writeStart = std::chrono::system_clock::now();
        snapObj.update(l, v);
        auto writeEnd = std::chrono::system_clock::now();
        // Log write with timestamp
        ss.str(std::string());
        ss << "Writer thread " << id << ": shArr[" << l << "] = " << v 
            << " in " << (writeEnd - writeStart).count() << " ns.";
        auto s = ss.str();
        Log lg(id, writeEnd, s);
        log.push_back(lg);
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds((int)writerSleepDist(rng)));
    }
}

template<class T>
void snapshotThreadRunner(uint16_t id, OFSnapshot<T> &snapObj, std::vector<Log> &log) {
    std::stringstream ss;
    for (uint32_t i = 0; i < k; i++) {
        // Do the snapshot
        auto collectStart = std::chrono::system_clock::now();
        std::vector<T> snap = snapObj.snapshot();
        auto collectEnd = std::chrono::system_clock::now();
        // Log snapshot
        ss.str(std::string());
        ss << "Snapshot thread " << id << ": collect " << i + 1 << " {";
        for (uint32_t j = 0; j < M; j++) ss << " " << j << ": " << snap[j] << (",}"[j + 1 == M]);
        ss << " in " << (collectEnd - collectStart).count() << " ns.";
        auto s = ss.str();
        Log lg(id, collectEnd, s);
        log.push_back(lg);
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
    std::vector<std::vector<Log>> writerLogs(nw), snapshotLogs(ns);
    // Create OFS object
    OFSnapshot<uint32_t> snapObj(M);
    // Start the threads: writer followed by snapshot threads
    for (uint16_t i = 0; i < nw; i++) {
        writerThreads[i] = std::thread{writerThreadRunner<uint32_t>, i, std::ref(snapObj), std::ref(writerLogs[i])};
    }
    for (uint16_t i = 0; i < ns; i++) {
        snapshotThreads[i] = std::thread{snapshotThreadRunner<uint32_t>, i, std::ref(snapObj), std::ref(snapshotLogs[i])};
    }
    // Join snapshot threads
    for (auto &th : snapshotThreads) th.join();
    // Tell writer threads to stop
    term = true;
    // Join writer threads
    for (auto &th : writerThreads) th.join();
    // Write logs to output
    std::vector<Log> out;
    for (auto &lg : writerLogs) out.insert(out.end(), lg.begin(), lg.end());
    for (auto &lg : snapshotLogs) out.insert(out.end(), lg.begin(), lg.end());
    sort(out.begin(), out.end());
    for (auto &lg : out) fout << lg;
    return 0;
}