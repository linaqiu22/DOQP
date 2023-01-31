#include "Enclave-utility.hpp"
#include "oblivious-blocks.hpp"
#pragma once

// typedef struct HNode {
//     int realData;
//     int noise;
// } HNode;

using HNode = pair<int, int>;
using Sanitizer = vector<TraceMem<HNode>>;
class Bucket {
    public:
    TraceMem<tbytes> *data;
    int id;
    int tableid;
    int keyMin, keyMax;
    int subDomainSize;
    int numData;
    Bucket(int id, int tableid, int keyMin, int keyMax, int numData);
    void populateData();
    void write(int i, tbytes &record);
    tbytes read(int i);
};

class HTree {
    vector<metaRecord> *t1meta;
    vector<metaRecord> *t2meta;
    TraceMem<pair<int,tbytes>> *t1data = nullptr;
    TraceMem<pair<int,tbytes>> *t2data = nullptr;
    vector<Bucket*> buckets;
    int numRealMatches = 0;

    public:
    bool uni; // Uni-DO-join
    float constant;
    bool pfjoin;
    int t1Size, t2Size;
    int keyMin, keyMax;
    double eps, delta;
    double eps_h, delta_h;
    double eps_b, delta_b;
    int domainSize;
    int height;
    vector<Sanitizer> rangeSanitizer; 
    int noisyOutputSize = 0;
    
    //bucketization
    vector<pair<int, int>> partitionSizes;
    vector<vector<pair<int, int>>> accumulateSizes; // {accuRealPartSize, accuPureNoise}
    vector<int> boundaries;
    int numPart1 = 0, numPart2 = 0;
    int bucketNoisyMax = 0;

    HTree(vector<metaRecord> *t1meta, vector<metaRecord> *t2meta, vector<pair<int, int> > initDomain, double eps, double delta, bool pfJoin, float constant, bool uni);
    void getHistBins(int tableID);
    void populateRangeSanitizer(int tableid, bool ci=true);
    pair<int, int> BRC(Sanitizer *rangeSanitizer, int from, int to);
    void constructBucketsOneTime();
    void constructBuckets(int tableID);
    void constructPBuckets(int tableID, TraceMem<pair<int,tbytes>> *data);
    void moveData(int tableID, query_type op, TraceMem<pair<int,tbytes>> *data);
    pair<int, int> bucketCrossProduct(int writeID, int qs, int qe, bool jattr);
    pair<int, int> pfJoin(int tableID1, int tableID2, int writeID);
    void prepareBinIdxData(vector<pair<number, bytes>> &idxBinStart, vector<pair<number, bytes>> &idxBinEnd, int idx_dim);
};
