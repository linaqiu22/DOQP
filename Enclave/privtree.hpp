#include <list>
#include <queue>
#include "math.h"
#include "type_def.hpp"
#include "Enclave-utility.hpp"
#include "oblivious-blocks.hpp"
#include "bplustree.hpp"
// using namespace std;
#pragma once

class TreeNode {
    public:
    int id;
    int depth;
    vector<metaRecord> *bin1 = nullptr;
    vector<metaRecord> *bin2 = nullptr;
    TraceMem<tbytes> *rbin1 = nullptr;
    TraceMem<tbytes> *rbin2 = nullptr;
    vector<pair<int, int> > domain; // inclusive
    vector<int> boundaries; // divide domain and get beta children
    TreeNode *children = nullptr;
    int count[2] = {0, 0};
    int noise[2] = {0, 0};
    int granularity = 1;
    TreeNode(int id, int depth, vector<pair<int, int> > domain, int count1, int count2);
    TreeNode();
    int split(int start_id, int beta);
    bool canSplit();
};

class PrivTree {
    vector<metaRecord> *t1data;
    vector<metaRecord> *t2data;
    // vector<int> keyHelperList;
    int numRealMatches = 0;
    TraceMem<pair<int,tbytes>> *global_rbin1 = nullptr;
    TraceMem<pair<int,tbytes>> *global_rbin2 = nullptr;
    // a place to store leaves? H(T)

    public:
    int noisyOutputSize = 0;
    double eps_d;
    double eps_r, delta;
    int theta;
    int beta;
    double lambda;
    double gamma;
    int dim;
    uint t1data_size, t2data_size;
    bool t1_inEnclave, t2_inEnclave;
    int node_id = 0;
    vector<pair<int, int>> initDomain;
    int binNoisyMax = 0;

    TreeNode *root;
    list<TreeNode*> nodelist;
    vector<TreeNode*> leaves;

    PrivTree(vector<metaRecord> *t1data, vector<metaRecord> *t2data, vector<pair<int, int> > initDomain, double eps_d, double eps_r, double delta, int theta, int beta);
    PrivTree();
    void createTree();
    void decompose(int level);
    void countInDomains();
    bool inDomain(metaRecord &record, TreeNode *node);
    void augmentInput(int tableID, vector<metaRecord> *data, int table, bool simulation, query_type op, bool sorting);
    
    void binning(int tableID, TreeNode *node, int table, bool simulation);
    void binningInEnclave(int tableID, int pos, TreeNode *node, int table, bool sorting);
    void collectBins(int tableID, int table);
    void prepareBinIdxData(vector<pair<number, bytes>> &idxBinStart, vector<pair<number, bytes>> &idxBinEnd, int idx_dim);
    void checkSplitCorrectness();
    vector<TreeNode*> select(int tableID, int qs, int qe);
    pair<int, int> selectJoin(vector<TreeNode*> &selectedBins, int tableID1, int tableID2, int writeID, int qs, int qe);
    pair<int,int> join(int tableID1, int tableID2, int writeID, int qs, int qe);
    pair<int, int> pfJoin(int tableID1, int tableID2, int writeID);
    // void crossProduct(int tableID1, int tableID2, int nreal1, int nreal2);
    int getNoisySize();
    int getBinPairKKSSize();
    int checkJoinOutput(int join_idx1, int join_idx2);
    void checkSelction(int start, int end, int query_dim, vector<int> *selectedBins);
    // void cleanHelper();
};