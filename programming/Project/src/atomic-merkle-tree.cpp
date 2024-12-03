#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <pthread.h>
#include <random>
#include <unistd.h>
#include <chrono>
#include <atomic>

using namespace std;

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
    atomic<bool> *nodeState;
    pthread_mutex_t *locks;

    MerkleTree(int numOfLeaves)
    {
        this->numOfLeaves = numOfLeaves;
        int treeSize = 2 * numOfLeaves - 1;

        this->tree = new string[treeSize]();
        this->locks = new pthread_mutex_t[treeSize];
        this->nodeState = new std::atomic<bool>[treeSize];

        for (int i = 0; i < treeSize; i++)
        {
            nodeState[i] = false;
            pthread_mutex_init(&locks[i], nullptr);
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
        pthread_mutex_lock(&locks[index]);
    }

    void unlockNode(int index)
    {
        pthread_mutex_unlock(&locks[index]);
    }

    void printTree() const
    {
        int treeSize = 2 * numOfLeaves - 1;
        int levels = (int)log2(treeSize + 1);
        int maxWidth = 15;

        vector<vector<string>> treeLevels(levels);
        vector<vector<string>> stateLevels(levels);

        int currentIndex = 0;
        for (int level = 0; level < levels; ++level)
        {
            int nodesInLevel = std::pow(2, level);
            int startIdx = std::pow(2, level) - 1;

            for (int i = 0; i < nodesInLevel && (startIdx + i) < treeSize; ++i)
            {
                treeLevels[level].push_back(tree[startIdx + i]);
                stateLevels[level].push_back(nodeState[startIdx + i] ? "T" : "F");
            }
        }

        int maxSpacing = std::pow(2, levels - 1) * maxWidth;
        for (int level = 0; level < levels; ++level)
        {
            int currentSpacing = maxSpacing / std::pow(2, level);

            for (int i = 0; i < treeLevels[level].size(); ++i)
            {
                cout << treeLevels[level][i] << "(" << stateLevels[level][i] << ")";

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
    {
        this->updateIdx = idx;
        this->tree = tree;
        this->threadId = threadId;
    }
};

int getExponentialTime(double mean)
{
    random_device rd;
    mt19937 gen(rd());
    exponential_distribution<> d(1.0 / mean);
    return static_cast<int>(d(gen) * 1000);
}

void *updateUsingThread(void *arg)
{
    ThreadTask *data = static_cast<ThreadTask *>(arg);
    int idx = data->updateIdx;
    int threadId = data->threadId;
    MerkleTree *tree = data->tree;
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
            return nullptr;
        }
        usleep(getExponentialTime(0.05) * 1000);
        tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";
        tree->nodeState[temp] = false;

        temp = tree->parent(temp);
    }

    if (tree->nodeState[temp].compare_exchange_strong(expected, desired))
    {
        return nullptr;
    }

    tree->lockNode(temp);
    usleep(getExponentialTime(0.05) * 1000);
    tree->tree[temp] = "Updated_" + to_string(temp) + "(" + to_string(threadId) + ")";
    tree->unlockNode(temp);

    return nullptr;
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

    pthread_t *threads = new pthread_t[batchSize];
    ThreadTask *threadData = new ThreadTask[batchSize];

    for (int i = 0; i < batchSize; i++)
    {
        for (int j = 0; j < batchSize; j++)
        {
            if (i == j)
                continue;
            int node1 = batch[i] + 7;
            int node2 = batch[j] + 7;

            int nodeToBeMarked = commonAncestor(node1, node2, tree);
            // cout << "(" << batch[i] << ", " << batch[i + 1] << ") = " << nodeToBeMarked << endl;
            tree.nodeState[nodeToBeMarked] = true;
        }
    }
    cout << "\nMarking Common Ancestors:\n";
    tree.printTree();

    Timer timer;

    for (int i = 0; i < batchSize; ++i)
    {
        threadData[i] = ThreadTask(batch[i], &tree, i);
        pthread_create(&threads[i], nullptr, updateUsingThread, &threadData[i]);
    }

    for (int i = 0; i < batchSize; ++i)
    {
        pthread_join(threads[i], nullptr);
    }

    double timeTaken = timer.getDuration();
    cout << "\nTree After Updates:\n";
    tree.printTree();

    cout << "Time taken for updating the batch: " << timeTaken << " ms" << endl;

    delete[] threads;
    delete[] threadData;

    return 0;
}
