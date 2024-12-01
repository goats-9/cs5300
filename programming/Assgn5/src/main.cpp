/**
 * @author Gautam Singh
 * @file Src-CS21BTECH11018.cpp
 * @brief C++-20 source for implementation of savings account object.
 *
 * @date 2024-12-01
 */

// Headers
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <format>
#include <iomanip>
#include <random>

// Constants and global variables
std::chrono::high_resolution_clock::time_point startTime;
constexpr int INITIAL_BALANCE = 10000;

int n, p, t;
double alpha;

const std::string INFILE = "inp-params.txt";
const std::string OUTFILE = "output.txt";

std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
std::uniform_int_distribution<int> dist(1, 3), accDist;
std::uniform_real_distribution<double> withdrawDist(1, 100), depositDist(200, 500);
std::exponential_distribution<double> sleepDist;

// Helper function to get current time since start of the program.
uint32_t getTimeStamp() {
    return std::chrono::nanoseconds(std::chrono::high_resolution_clock::now() - startTime).count();
}

/**
 * @brief A lock-based implementation of a savings account in a bank. Has the
 * following features.
 * 1. Implements deposit and withdraw actions. Withdraws can be ordinary or
 *    preferred, with preferred withdrawals having higher priority.
 * 2. Starts with an initial balance to prevent blocking in case no further
 *    deposits are made.
 */
class SavingsAccount {
private:
    // Account-related info
    uint32_t accNumber, preferredWaiting = 0;
    double balance = 0;
    // Locks and conditions
    std::mutex lock;
    std::condition_variable condition, balanceCondition;

public:
    SavingsAccount(uint32_t n) : accNumber(n) {balance = INITIAL_BALANCE;}

    // Withdraw method
    void withdraw(bool preferred, double amount, int id, std::vector<std::string> &log) {
        log.push_back(std::format(
            "[{:9} ns] Thread {} requests {} withdrawal of {} from account {}.",
            getTimeStamp(), id + 1, (preferred ? "preferred" : "ordinary"), amount, accNumber
        ));
        // Acquire lock
        std::unique_lock<std::mutex> guard(lock);

        log.push_back(std::format(
            "[{:9} ns] Thread {} requests {} withdrawal of {} from account {} and enters the CS.",
            getTimeStamp(), id + 1, (preferred ? "preferred" : "ordinary"), amount, accNumber
        ));
        bool log_wait = false;
        // A trick for handling two cases: preferred as a boolean is interpreted
        // as 1. 
        preferredWaiting += preferred;
        // Check if there are preferred withdrawals
        while (preferredWaiting > preferred) {
            if (!log_wait) {
                log.push_back(std::format(
                    "[{:9} ns] Thread {} requesting {} withdrawal of {} from "
                    "account {} blocks due to pending preferred withdrawal.",
                    getTimeStamp(), id + 1, (preferred ? "preferred" : "ordinary"), amount, accNumber
                ));
                log_wait = true;
            }
            condition.wait(guard);
        }
        if (log_wait) {
            log.push_back(std::format(
                "[{:9} ns] Thread {} requesting {} withdrawal of {} from "
                "account {} wakes up to complete withdrawal.\n",
                getTimeStamp(), id + 1, (preferred ? "preferred" : "ordinary"), amount, accNumber
            ));
        }
        preferredWaiting -= preferred;

        log_wait = false;
        // Check if withdrawal will block
        while (balance < amount) {
            if (!log_wait) {
                log.push_back(std::format(
                    "[{:9} ns] Thread {} requesting {} withdrawal of {} from "
                    "account {} blocks due to insufficient funds.",
                    getTimeStamp(), id + 1, (preferred ? "preferred" : "ordinary"), amount, accNumber
                ));
                log_wait = true;
            }
            balanceCondition.wait(guard);
        }
        log.push_back(std::format(
            "[{:9} ns] Thread {} requesting {} withdrawal of {} from "
            "account {} completes withdrawal and wakes up sleeping threads.",
            getTimeStamp(), id + 1, (preferred ? "preferred" : "ordinary"), amount, accNumber
        ));
        balance -= amount;
        // Wake other threads up.
        condition.notify_all();
    }

    // Deposit method
    void deposit(double amount, int id, std::vector<std::string> &log) {
        log.push_back(std::format(
            "[{:9} ns] Thread {} requests deposit of {} to account {}.",
            getTimeStamp(), id + 1, amount, accNumber
        ));
        std::unique_lock<std::mutex> guard(lock);

        log.push_back(std::format(
            "[{:9} ns] Thread {} requesting deposit of {} to account {}, "
            "completes the deposit and wakes up sleeping threads.",
            getTimeStamp(), id + 1, amount, accNumber
        ));
        balance += amount;
        condition.notify_all();
    }
};

std::vector<std::unique_ptr<SavingsAccount>> accounts;
std::vector<std::string> logs;

// Runner function for threads
void runner(int id, std::vector<std::string> &log) {
    for (int k = 1; k <= t; k++) {
        // Choose random operation, random account and random amount
        int op = dist(rng);
        int i = accDist(rng);
        if (op == 1) {
            // Deposit amount
            double amount = depositDist(rng);
            accounts[i]->deposit(amount, id, log);
        } else if (op == 2) {
            // Ordinary withdrawal of amount
            double amount = withdrawDist(rng);
            accounts[i]->withdraw(false, amount, id, log);
        } else if (op == 3) {
            // Preferred withdrawal of amount
            double amount = withdrawDist(rng);
            accounts[i]->withdraw(true, amount, id, log);
        }
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)sleepDist(rng)));
    }
}

int main() {
    std::fstream fin, fout;
    try {
        fin = std::fstream(INFILE, std::fstream::in);
        fout = std::fstream(OUTFILE, std::fstream::out);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    // File IO
    fin >> n >> p >> t >> alpha;

    // Create accounts
    for (int i = 1; i <= p; i++) accounts.push_back(std::make_unique<SavingsAccount>(i));
    // Set up randomness
    accDist = std::uniform_int_distribution<int>(0, p - 1);
    sleepDist = std::exponential_distribution<double>(alpha);

    // Create thread information: id and logs.
    std::vector<std::thread> runners(n);
    std::vector<std::vector<std::string>> logs(n);
    startTime = std::chrono::high_resolution_clock::now();
    // Create threads
    for (int i = 0; i < n; i++) runners[i] = std::thread(runner, i, std::ref(logs[i]));
    // Join threads
    for (auto &th : runners) th.join();
    // Collect and sort logs to output
    std::vector<std::string> allLogs;
    for (auto &log : logs) allLogs.insert(allLogs.end(), log.begin(), log.end());
    std::sort(allLogs.begin(), allLogs.end());
    for (auto &log : allLogs) fout << log << '\n';
    return 0;
}