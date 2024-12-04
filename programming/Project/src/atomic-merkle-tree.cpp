#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <random>
#include <chrono>
#include <atomic>
#include <cmath>

using namespace std;

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

    MerkleTree(int numOfLeaves) : numOfLeaves(numOfLeaves)
    {
        treeSize = 2 * numOfLeaves - 1;
        tree = new string[treeSize];
        nodeState = new atomic<bool>[treeSize];

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

    bool expected = true;
    bool desired = false;

    while (temp > 0)
    {
        if (tree->nodeState[temp])
        {
            while (tree->nodeState[temp].compare_exchange_strong(expected, desired))
            {
            }
            return;
        }
        this_thread::sleep_for(chrono::milliseconds(getExponentialTime(0.05)));
        tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";
        tree->nodeState[temp] = false;

        temp = tree->parent(temp);
    }

    if (tree->nodeState[temp].compare_exchange_strong(expected, desired))
    {
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
    MerkleTree tree(8);
    tree.printTree();

    int batch[] = {0, 1, 3, 4, 6};
    int batchSize = sizeof(batch) / sizeof(batch[0]);

    for (int i = 0; i < batchSize; i++)
    {
        for (int j = 0; j < batchSize; j++)
        {
            if (i == j)
                continue;
            int node1 = batch[i] + 7;
            int node2 = batch[j] + 7;

            int nodeToBeMarked = commonAncestor(node1, node2, tree);
            tree.nodeState[nodeToBeMarked] = true;
        }
    }

    cout << "\nMarking Common Ancestors:\n";
    tree.printTree();

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
    cout << "\nTree After Updates:\n";
    tree.printTree();

    cout << "Time taken for updating the batch: " << timeTaken << " ms" << endl;

    return 0;
}
