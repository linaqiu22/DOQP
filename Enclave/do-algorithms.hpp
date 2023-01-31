#include "privtree.hpp"
#include "bplustree.hpp"
#include "czsc-join.hpp"
#include "htree.hpp"
#include "Enclave-utility.hpp"
#pragma once

typedef struct {
    shared_ptr<PrivTree> privtree;
    shared_ptr<HTree> htree;
    vector<pair<shared_ptr<BPlusTreeIndex>, shared_ptr<BPlusTreeIndex>>> indexes;
} PDS;

/* typedef struct {
    shared_ptr<HTree> htree;
    vector<pair<shared_ptr<BPlusTreeIndex>, shared_ptr<BPlusTreeIndex>>> indexes;
} HPDS */

static vector<PDS> pds;
// static HTree *htree;
static vector<int> allBins;
static CZSCJoin *czsc_join;
static TraceMem<pair<int,tbytes>>* data[2]; // only in htree
static vector<metaRecord> metaData_t1, metaData_t2;
static vector<pair<int, int>> initDomain;

void extractMeta(int tableID, char *encryptedData, int inSize, char *sensitiveAttr, uint sattrSize);
void extractData(int tableID, int nRecords);
int createPDS(int tableID);
int initJoinPDS(int pfJoin, int selectJoin);
int initHTree(int pfJoin, float constant, int ind);
// void tmpAugmentInput(int structureID, int tableID1, int tableID2);
void createJoinPDS(int structureID, int tableID1, int tableID2);
void createSelectJoinPDS(int structureID, int tableID1, int tableID2);
void createJoinBuckets(int structureID, int tableID1, int tableID2, int pfJoin);
void createCZSC(int tableID1, char *eData_t1, int inSize_t1, int join_idx1, int tableID2, char *eData_t2, int inSize_t2, int join_idx2, int pfJoin);

int buildIndex(int structureID, int idx_dim, const char*str, join_type jtype);
int restoreIndex(int structureID, int idx_dim, char *s_idxBinStart, uint size1, char *s_idxBinEnd, uint size2);
void restoreJoinMeta(int tableID, int binID, int pos, int join_idx, char *encryptedBin, uint dsize);
int idxSelect(int structureID, int idx_dim, int queryStart, int queryEnd, char *s_bins, uint dsize, char *s_records, uint rdsize, join_type jtype);
int innerJoin(int structureID, int tableID1, int tableID2, int writeID, join_type jtype, int qs, int qe, int jattr);
int pfJoin(int structureID, int tableID1, int tableID2, int writeID, join_type jtype);
int czscJoin(int tableID1, int tableID2, int writeID, int pfJoin);
void joinCompact(int structureID, int writeID, int size, join_type jtype);