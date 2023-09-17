#include "CONFIG.hpp"
#include "type_def.hpp"
#include "trace_mem.h"
#include <queue>
using namespace std;
#pragma once

typedef struct {
    int table;
    int rid;
    vector<int> key;
    int bin_id;
} JoinRecord;

class CZSCJoin {
    vector<KeyBin> keyBinList;
    vector<JoinRecord> T1, T2;
    TraceMem<tbytes> *rbin1 = nullptr;
    TraceMem<tbytes> *rbin2 = nullptr;
    vector<pair<int, JoinRecord>> noiseArray1, noiseArray2;
    vector<vector<JoinRecord>*> tq;
    int n_sparsekeys = 0;
    int noisyBins = 0;
    // int noiseSum1 = 0, noiseSum2 = 0; //*tmp
    public:
    int tableID1, tableID2;
    int t1data_size, t2data_size;
    int realMatches = 0;
    double eps, delta, eps_g, delta_g;
    int U;
    int ndense = 0, n_sparsebins = 0;
    bool t1_inEnclave, t2_inEnclave;
    vector<tuple<int, int, int>> denseBins;
    int keyNoisyMax = 0, noisyOutputSize = 0;
    unsigned long long crossProductSize = 0;
    CZSCJoin(int tableID1, int tableID2, vector<metaRecord> *t1_metadata, vector<metaRecord> *t2_metadata, double eps, double delta);
    int populateKeyBinList(vector<JoinRecord> *data);
    pair<int, int> groupSparseKeys();
    void expandJoinTables(int noiseSum1, int noiseSum2);
    void expandTable(int tableID);
    void createBins(int tableID);
    void binning(int tableID, int start, int end, int readBinID);
    pair<int,int> join(int writeID, bool pfJoin=false);
    int checkOutputLen(bool optimized=false);
};