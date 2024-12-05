#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <random>
#include <chrono>
#include <vector>
#include <mutex>
#include <fstream>

using namespace std;

const string inputFileName = "inp.txt";

class Timer
{
private:
    std::chrono::high_resolution_clock::time_point start_time;

public:
    Timer() { start_time = std::chrono::high_resolution_clock::now(); }

    double getDuration() const
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end_time - start_time;
        return elapsed.count();
    }
};

class MerkleTree
{
public:
    int numOfLeaves;
    string *tree;
    std::mutex *locks;

    MerkleTree(int numOfLeaves)
    {
        this->numOfLeaves = numOfLeaves;
        int treeSize = 2 * numOfLeaves - 1;

        this->tree = new string[treeSize]();
        this->locks = new std::mutex[treeSize];

        for (int i = 0; i < treeSize; i++)
        {
            tree[i] = "Node_" + to_string(i);
        }
    }

    ~MerkleTree()
    {
        delete[] tree;
        delete[] locks;
    }

    void lockNode(int index)
    {
        locks[index].lock();
    }

    void unlockNode(int index)
    {
        locks[index].unlock();
    }

    void printTree() const
    {
        int treeSize = 2 * numOfLeaves - 1;
        int levels = (int)log2(treeSize + 1);
        int maxWidth = 20;

        vector<vector<string>> treeLevels(levels);

        int currentIndex = 0;
        for (int level = 0; level < levels; ++level)
        {
            int nodesInLevel = std::pow(2, level);
            int startIdx = std::pow(2, level) - 1;

            for (int i = 0; i < nodesInLevel && (startIdx + i) < treeSize; ++i)
            {
                treeLevels[level].push_back(tree[startIdx + i]);
            }
        }

        int maxSpacing = std::pow(2, levels - 1) * maxWidth;
        for (int level = 0; level < levels; ++level)
        {
            int currentSpacing = maxSpacing / std::pow(2, level);

            for (int i = 0; i < treeLevels[level].size(); ++i)
            {
                cout << treeLevels[level][i];

                if (i < treeLevels[level].size() - 1)
                {
                    for (int s = 0; s < currentSpacing - treeLevels[level][i].length(); ++s)
                    {
                        cout << " ";
                    }
                }
            }
            cout << endl;
        }
    }

    int leftChild(int index) const { return 2 * index + 1; }
    int rightChild(int index) const { return 2 * index + 2; }
    int parent(int index) const { return (index - 1) / 2; }
    int siblingIndex(int index) const { return (index % 2 == 0) ? index - 1 : index + 1; }
};

struct ThreadTask
{
    int updateIdx;
    MerkleTree *tree;
    int threadId;

    ThreadTask(int idx = 0, MerkleTree *tree = nullptr, int threadId = 0)
        : updateIdx(idx), tree(tree), threadId(threadId) {}
};

int getExponentialTime(double mean)
{
    random_device rd;
    mt19937 gen(rd());
    exponential_distribution<> d(1.0 / mean);
    return static_cast<int>(d(gen) * 1000);
}

void updateUsingThread(ThreadTask task)
{
    int idx = task.updateIdx;
    int threadId = task.threadId;
    MerkleTree *tree = task.tree;
    int leafCount = tree->numOfLeaves;
    int temp = idx + leafCount - 1;

    while (temp > 0)
    {
        tree->lockNode(temp);
        std::this_thread::sleep_for(std::chrono::milliseconds(getExponentialTime(0.05)));
        tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";
        tree->unlockNode(temp);

        temp = tree->parent(temp);
    }

    tree->lockNode(temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(getExponentialTime(0.05)));
    tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";
    tree->unlockNode(temp);
}

int main()
{
    ifstream inputFile(inputFileName);
    if (!inputFile.is_open())
    {
        cerr << "Error: Unable to open file " << inputFileName << endl;
        return 1;
    }

    int leafCount;
    inputFile >> leafCount;

    MerkleTree tree(leafCount);
    // cout << "Initial Tree:" << endl;
    // tree.printTree();

    int batchSize;
    inputFile >> batchSize;
    vector<int> batch(batchSize);

    for (int i = 0; i < batchSize; ++i)
    {
        inputFile >> batch[i];
    }
    inputFile.close();

    vector<thread> threads;
    vector<ThreadTask> threadData;

    for (int i = 0; i < batchSize; ++i)
    {
        threadData.emplace_back(batch[i], &tree, i);
    }

    Timer timer;

    for (int i = 0; i < batchSize; ++i)
    {
        threads.emplace_back(updateUsingThread, threadData[i]);
    }

    for (auto &th : threads)
    {
        th.join();
    }

    double timeTaken = timer.getDuration();
    // cout << "\nTree After Updates:\n";
    // tree.printTree();

    cout << "" << timeTaken << "" << endl;

    return 0;
}
