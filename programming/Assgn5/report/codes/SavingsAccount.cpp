class SavingsAccount {
private:
    uint32_t accNumber, preferredWaiting = 0;
    double balance = 0;
    std::mutex lock;
    std::condition_variable preferredCondition, balanceCondition;

public:
    SavingsAccount(uint32_t n);

    // Withdraw method
    void withdraw(bool preferred, double amount, int id, std::vector<std::string> &log);

    // Deposit method
    void deposit(double amount, int id, std::vector<std::string> &log);
};
