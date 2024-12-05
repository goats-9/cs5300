#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <random>
#include <chrono>
#include <atomic>
#include <cmath>
#include <fstream>
#include <mutex>

using namespace std;

const string inputFileName = "inp.txt";

class Timer
{
private:
    chrono::high_resolution_clock::time_point start_time;

public:
    Timer() { start_time = chrono::high_resolution_clock::now(); }

    double getDuration() const
    {
        auto end_time = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = end_time - start_time;
        return elapsed.count();
    }
};

class MerkleTree
{
public:
    int numOfLeaves;
    int treeSize;
    string *tree;
    atomic<bool> *nodeState;
    mutex *locks;

    MerkleTree(int numOfLeaves) : numOfLeaves(numOfLeaves)
    {
        treeSize = 2 * numOfLeaves - 1;
        tree = new string[treeSize];
        nodeState = new atomic<bool>[treeSize];
        this->locks = new mutex[treeSize];

        for (int i = 0; i < treeSize; i++)
        {
            nodeState[i] = false;
            tree[i] = "Node_" + to_string(i);
        }
    }

    ~MerkleTree()
    {
        delete[] tree;
        delete[] nodeState;
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
        int levels = static_cast<int>(log2(treeSize + 1));
        int maxWidth = 15;

        for (int level = 0; level < levels; ++level)
        {
            int nodesInLevel = pow(2, level);
            int startIdx = pow(2, level) - 1;

            for (int i = 0; i < nodesInLevel && (startIdx + i) < treeSize; ++i)
            {
                cout << tree[startIdx + i] << "(" << (nodeState[startIdx + i] ? "T" : "F") << ")";
                if (i < nodesInLevel - 1)
                {
                    cout << "   ";
                }
            }
            cout << endl;
        }
    }

    int leftChild(int index) const { return 2 * index + 1; }
    int rightChild(int index) const { return 2 * index + 2; }
    int parent(int index) const { return (index - 1) / 2; }
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
        if (tree->nodeState[temp])
        {
            tree->lockNode(temp);
            tree->nodeState[temp] = false;
            tree->unlockNode(temp);
            return;
        }
        this_thread::sleep_for(chrono::milliseconds(getExponentialTime(0.05)));
        tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";

        temp = tree->parent(temp);
    }

    if (tree->nodeState[temp])
    {
        tree->lockNode(temp);
        tree->nodeState[temp] = false;
        tree->unlockNode(temp);
        return;
    }

    this_thread::sleep_for(chrono::milliseconds(getExponentialTime(0.05)));
    tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";
}

int commonAncestor(int node1, int node2, MerkleTree &tree)
{
    while (node1 != node2)
    {
        node1 = tree.parent(node1);
        node2 = tree.parent(node2);
    }
    return node1;
}

int main()
{
    ifstream inputFile(inputFileName);
    if (!inputFile.is_open())
    {
        cerr << "Error opening file!" << endl;
        exit(1);
    }

    int leafCount, batchSize;
    inputFile >> leafCount >> batchSize;

    MerkleTree tree(leafCount);
    // tree.printTree();

    int *batch = new int[batchSize];

    for (int i = 0; i < batchSize; ++i)
    {
        inputFile >> batch[i];
    }

    inputFile.close();

    // cout << "Batch size: " << batchSize << endl;
    // cout << "Batch elements: ";
    // for (int i = 0; i < batchSize; ++i)
    // {
    //     cout << batch[i] << " ";
    // }
    // cout << endl;

    for (int i = 0; i < batchSize; i++)
    {
        for (int j = 0; j < batchSize; j++)
        {
            if (i == j)
                continue;
            int node1 = batch[i] + leafCount - 1;
            int node2 = batch[j] + leafCount - 1;

            int nodeToBeMarked = commonAncestor(node1, node2, tree);
            tree.nodeState[nodeToBeMarked] = true;
        }
    }

    // cout << "\nMarking Common Ancestors:\n";
    // tree.printTree();

    Timer timer;
    thread threads[batchSize];

    for (int i = 0; i < batchSize; ++i)
    {
        threads[i] = thread(updateUsingThread, ThreadTask(batch[i], &tree, i));
    }

    for (int i = 0; i < batchSize; ++i)
    {
        threads[i].join();
    }

    double timeTaken = timer.getDuration();
    // cout << "\nTree After Updates:\n";
    // tree.printTree();

    cout << "" << timeTaken << "" << endl;

    delete[] batch;
    return 0;
}
