#include "do-algorithms.hpp"
#include <set>
TraceMem<cbytes> *jWorkSpace[2];

void extractMeta(int tableID, char *encryptedData, int inSize, char *sensitiveAttr, uint sattrSize) {
    auto attrCount = sattrSize / sizeof(int); // number of sensitive attributes
    vector<int> attrIdx((int *)sensitiveAttr, (int *)sensitiveAttr + attrCount);
    if (tableID == 0) {
        parseInput(metaData_t1, encryptedData, inSize, tableID, attrIdx, initDomain);
    } else {
        parseInput(metaData_t2, encryptedData, inSize, tableID, attrIdx, initDomain);
    }
}

void extractData(int tableID, int nRecords) {
    data[tableID] = new TraceMem<pair<int,tbytes>>(nRecords);
    readDataBlockPair_Wrapper(tableID, tableID, 0, 0, data[tableID], 0, nRecords);
    ObliSortingNet<pair<int,tbytes>> net = ObliSortingNet<pair<int,tbytes>>(data[tableID], [](pair<int,tbytes> &r1, pair<int,tbytes> &r2) {
        return r1.first <= r2.first;
    });
    net.obliSort(data[tableID]->size, true);
    if (initDomain.empty()) {
        initDomain.push_back({INT_MAX, INT_MIN});
    }
    auto metaData = tableID == 0 ? &metaData_t1 : &metaData_t2;
    metaRecord mr;
    mr.values.resize(1);
    for (int i = 0; i < nRecords; i++) {
        auto pr = data[tableID]->read(i);
        mr.rid = i;
        initDomain[0].first = min(pr.first, initDomain[0].first);
        initDomain[0].second = max(pr.first, initDomain[0].second);
        mr.values[0] = pr.first;
        metaData->push_back(mr);
    }
}


int createPDS(int tableID) {
    PDS tpds;
    double delta = 1.0 / metaData_t1.size();
    int theta = 0;
    int beta = pow(2, sensitive_dim);
    tpds.privtree = make_shared<PrivTree>(&metaData_t1, nullptr, initDomain, eps_d, eps_r, delta, theta, beta);
    printf("start creating privtree\n");
    tpds.privtree->createTree();
    bool sorting = sensitive_dim > 1 ? false : true;
    tpds.privtree->augmentInput(tableID, &metaData_t1, 1, select_simulation, Select, sorting);
    if (!tpds.privtree->t1_inEnclave) {
        if (!select_simulation) {
            initBinStorage_OCALL(tableID, tpds.privtree->root->id);
        }
        tpds.privtree->binning(tableID, tpds.privtree->root, 1, select_simulation);
        if (!select_simulation) {
            tpds.privtree->collectBins(tableID, 1);
        }
    } else {
        tpds.privtree->binningInEnclave(tableID, 0, tpds.privtree->root, tableID+1, sorting);
        // tpds.privtree->cleanHelper();
    }
    // tpds.privtree->checkSplitCorrectness();
    tpds.indexes.resize(initDomain.size());
    pds.push_back(tpds);
    return pds.size()-1;
}

int initJoinPDS(int pfJoin, int selectJoin) {
    printf("read size %d %d\n", metaData_t1.size(), metaData_t2.size());
    PDS tpds, tpds2;
    delta_r = pow(1.0 / (metaData_t1.size()+metaData_t2.size()), 1.3);
    delta_c = delta_r;
    double eps = eps_d + eps_r + eps_c;
    double delta = delta_r + delta_c;
    int theta;
    if (pfJoin != 0) {
        theta = sqrt(metaData_t1.size()+metaData_t2.size());
        // theta = 0;
        // theta = 2*(1/eps*log(2/delta));
        printf("pfJoin ");
    } else {
        theta = 2*(1/eps*log(2/delta));
        // theta = sqrt(metaData_t1.size()+metaData_t2.size());
        // theta = 0;
    }
    printf("theta is %d\n", theta);
    int beta = pow(2, sensitive_dim);
    if (pfJoin != 0) {
        // foreign key join, no need to reserve delta_c for final compaction
        delta_r = delta;
    }
    if (selectJoin == 0) {
        tpds.privtree = make_shared<PrivTree>(&metaData_t1, &metaData_t2, initDomain, eps_d, eps_r, delta_r, theta, beta);
        tpds.privtree->createTree();
    } else {
        // beta = 2 for T1
        printf("start init select join\n");
        vector<pair<int, int>> joinDomain{initDomain[0]};

        tpds.privtree = make_shared<PrivTree>(&metaData_t1, nullptr, joinDomain, eps_d, eps_r, delta_r, theta, 2);
        tpds.privtree->createTree();
        tpds2.privtree = make_shared<PrivTree>(nullptr, &metaData_t2, initDomain, eps_d, eps_r, delta_r, theta, beta);
        tpds2.privtree->createTree();
        printf("done init select join\n");
    }
    
    pds.push_back(tpds);
    if (selectJoin == 1) {
        pds.push_back(tpds2);
    }
    return pds.size()-1;
}

int initHTree(int pfJoin, float constant, int ind) {
    bool uni = ind == 1 ? false : true;
    PDS hpds;
    // printf("read size %d %d\n", metaData_t1.size(), metaData_t2.size());
    bool t1Valid = !metaData_t1.empty();
    bool t2Valid = !metaData_t2.empty();
    bool pfjoin = pfJoin > 0 ? true : false;
    delta_r = pow(1.0 / (metaData_t1.size()+metaData_t2.size()), 1.3);
    delta_c = delta_r;
    if (pfJoin != 0) {
        // foreign key join, no need to reserve delta_c for final compaction
        delta_r += delta_c;
    }
    hpds.htree = make_shared<HTree>(&metaData_t1, &metaData_t2, initDomain, eps_d+eps_r, delta_r, pfjoin, constant, uni);
    int tableID1 = 0, tableID2 = 1;
    if (pfJoin == 0 && t1Valid) {
        hpds.htree->getHistBins(tableID1);
        metaData_t1.clear();
        // printf("t1 getHistBin\n");
    } else {
        Sanitizer hbins(1, TraceMem<HNode>(0));
        hpds.htree->rangeSanitizer.push_back(hbins);
    }
    if (t2Valid) {
        hpds.htree->getHistBins(tableID2);
        metaData_t2.clear();
        // printf("t2 getHistBin\n");
    }
    
    if (pfJoin == 0 && t1Valid) {
        hpds.htree->populateRangeSanitizer(tableID1); // table with primary key
        // printf("t1 populateRangeSanitizer\n");
    }
    if (t2Valid) {
        hpds.htree->populateRangeSanitizer(tableID2);
        // printf("t2 populateRangeSanitizer\n");
    }
    if (uni) {
        hpds.htree->constructBucketsOneTime();
        // printf("constructBucketsOneTime\n");
    } else {
        //* independent, assume that PDS is constructed beforehand, i.e, clock() after initHTree
        if (pfJoin == 0 && t1Valid) {
            hpds.htree->constructBuckets(tableID1);
            // printf("t1 contructBuckets\n");
        }
        if (t2Valid) {
            hpds.htree->constructBuckets(tableID2);
            // printf("t2 contructBuckets\n");
        }
    }
    hpds.indexes.resize(1);
    pds.push_back(hpds);
    return pds.size()-1;
}

// PrivTree
void createJoinPDS(int structureID, int tableID1, int tableID2) {
    PDS &tpds = pds[structureID];
    bool sorting = sensitive_dim > 1 ? false : true;
    tpds.privtree->augmentInput(tableID1, &metaData_t1, 1, join_simulation, Join, sorting);
    tpds.privtree->augmentInput(tableID2, &metaData_t2, 2, join_simulation, Join, sorting);
    printf("done augment tables\n");
    if (!tpds.privtree->t1_inEnclave) {
        if (!join_simulation) {
            initBinStorage_OCALL(tableID1, tpds.privtree->root->id);
        }
        tpds.privtree->binning(tableID1, tpds.privtree->root, 1, join_simulation);
        if (!join_simulation) {
            tpds.privtree->collectBins(tableID1, 1);
        }
    } else {
        tpds.privtree->binningInEnclave(tableID1, 0, tpds.privtree->root, 1, sorting);
        // tpds.privtree->cleanHelper();
    }
    if (!tpds.privtree->t2_inEnclave) {
        if (!join_simulation) {
            initBinStorage_OCALL(tableID2, tpds.privtree->root->id);
        }
        tpds.privtree->binning(tableID2, tpds.privtree->root, 2, join_simulation);
        if (!join_simulation) {
            tpds.privtree->collectBins(tableID2, 2);
        }
    } else {
        tpds.privtree->binningInEnclave(tableID2, 0, tpds.privtree->root, 2, sorting);
        // tpds.privtree->cleanHelper();
    }
    tpds.indexes.resize(initDomain.size());
}

// PrivTree
void createSelectJoinPDS(int structureID, int tableID1, int tableID2) {
    PDS &tpds1 = pds[structureID-1];
    PDS &tpds2 = pds[structureID];
    tpds1.privtree->augmentInput(tableID1, &metaData_t1, 1, join_simulation, Join, true);
    tpds2.privtree->augmentInput(tableID2, &metaData_t2, 2, join_simulation, Join, false);
    printf("done augment tables\n");
    // assume t1_inEnclave is true
    tpds1.privtree->binningInEnclave(tableID1, 0, tpds1.privtree->root, 1, true);
    // tpds1.privtree->cleanHelper();
    
    // assume t2_inEnclave is true
    tpds2.privtree->binningInEnclave(tableID2, 0, tpds2.privtree->root, 2, false);
    // tpds2.privtree->cleanHelper();
}

// HTree
void createJoinBuckets(int structureID, int tableID1, int tableID2, int pfJoin) {
    if (pfJoin == 0) {
        pds[structureID].htree->moveData(tableID1, Join, data[tableID1]);
    } else {
        pds[structureID].htree->constructPBuckets(tableID1, data[tableID1]);
    }
    if (tableID2 >= 0) {
        pds[structureID].htree->moveData(tableID2, Join, data[tableID2]);
    }
}

void createCZSC(int tableID1, char *eData_t1, int inSize_t1, int join_idx1, int tableID2, char *eData_t2, int inSize_t2, int join_idx2, int pfJoin) {
    vector<int> attrIdx_t1(sensitive_dim);
    vector<int> attrIdx_t2(sensitive_dim);
    for (int i = 0; i < sensitive_dim; i++) {
        attrIdx_t1[i] = i;
        attrIdx_t2[i] = i;
    }
    parseInput(metaData_t1, eData_t1, inSize_t1, tableID1, attrIdx_t1, initDomain);
    parseInput(metaData_t2, eData_t2, inSize_t2, tableID2, attrIdx_t2, initDomain);
    delta_r = pow(1.0 / (metaData_t1.size()+metaData_t2.size()), 1.3);
    delta_c = delta_r;
    double eps = eps_d + eps_r + eps_c;
    double delta = delta_r + delta_c;
    if (pfJoin != 0) {
        // foreign key join
        delta_c = 0;
    }
    // czsc_join = new CZSCJoin(tableID1, tableID2, &metaData_t1, &metaData_t2, 3*eps_d, 1.5*delta);
    czsc_join = new CZSCJoin(tableID1, tableID2, &metaData_t1, &metaData_t2, eps, delta);
}

int buildIndex(int structureID, int idx_dim, const char *str, join_type jtype) {
    vector<pair<number, bytes>> idxBinStart, idxBinEnd;
    if (jtype == HTreeDO) {
        pds[structureID].htree->prepareBinIdxData(idxBinStart, idxBinEnd, idx_dim);
    } 
    if (jtype == DO) {
        pds[structureID].privtree->prepareBinIdxData(idxBinStart, idxBinEnd, idx_dim);
    }
    pds[structureID].indexes[idx_dim].first = make_shared<BPlusTreeIndex>(make_shared<InMemoryStorage>(M_BLOCKSIZE), idxBinStart);
    pds[structureID].indexes[idx_dim].second = make_shared<BPlusTreeIndex>(make_shared<InMemoryStorage>(M_BLOCKSIZE), idxBinEnd);
    uint size1 = 2*sizeof(number)*idxBinStart.size();
    uint size2 = 2*sizeof(number)*idxBinEnd.size();
    char *s_idxBinStart = (char *)malloc(size1);
    char *s_idxBinEnd = (char *)malloc(size2);
    auto iter = s_idxBinStart;
    for (uint i = 0; i < idxBinStart.size(); i++) {
        memcpy(iter, &idxBinStart[i].first, sizeof(number));
        iter += sizeof(number);
        copy(idxBinStart[i].second.begin(), idxBinStart[i].second.begin()+sizeof(number), iter);
        iter += sizeof(number);
    }
    iter = s_idxBinEnd;
    for (uint i = 0; i < idxBinEnd.size(); i++) {
        memcpy(iter, &idxBinEnd[i].first, sizeof(number));
        iter += sizeof(number);
        copy(idxBinEnd[i].second.begin(), idxBinEnd[i].second.begin()+sizeof(number), iter);
        iter += sizeof(number);
    }
    write_index_OCALL(structureID, s_idxBinStart, size1, s_idxBinEnd, size2, str);
    if (jtype == DO) {
        printf("return privtree size %d\n", pds[structureID].privtree->leaves.size());
        return pds[structureID].privtree->leaves.size();
    }
    if (jtype == HTreeDO) {
        printf("return HTreeDO size %d\n", pds[structureID].htree->numPart1+pds[structureID].htree->numPart2);
        return pds[structureID].htree->numPart1+pds[structureID].htree->numPart2;
    }
    return 0;
}

int restoreIndex(int structureID, int idx_dim, char *s_idxBinStart, uint size1, char *s_idxBinEnd, uint size2) {
    // PDS tpds;
    vector<pair<number, bytes>> idxBinStart, idxBinEnd;
    uint numLeaves1 = size1 / (2*sizeof(number));
    uint numLeaves2 = size2 / (2*sizeof(number));
    vector<number> tmpArray((number*)s_idxBinStart, (number*)s_idxBinStart+numLeaves1*2);
    set<number> allBins;
    pair<number, bytes> pr;
    for (uint i = 0; i < numLeaves1; i++) {
        pr.first = tmpArray[2*i];
        pr.second = bytesFromNumber(tmpArray[2*i+1]);
        allBins.insert(tmpArray[2*i+1]);
        idxBinStart.push_back(pr);
    }
    tmpArray.clear();
    tmpArray.insert(tmpArray.end(), (number*)s_idxBinEnd, (number*)s_idxBinEnd+numLeaves2*2);
    for (uint i = 0; i < numLeaves2; i++) {
        pr.first = tmpArray[2*i];
        pr.second = bytesFromNumber(tmpArray[2*i+1]);
        allBins.insert(tmpArray[2*i+1]);
        idxBinEnd.push_back(pr);
    }
    vector<number> sortedLeaves(allBins.begin(), allBins.end());
    if (pds.size() <= (uint)structureID) {
        pds.resize(structureID+1);
        pds[structureID].privtree = make_shared<PrivTree>();
        for (auto id : sortedLeaves) {
            TreeNode *leaf_node = new TreeNode();
            leaf_node->id = id;
            pds[structureID].privtree->leaves.push_back(leaf_node);
        }
    }
    if (pds[structureID].indexes.size() <= (uint)idx_dim) {
        pds[structureID].indexes.resize(idx_dim+1);
    }
    pds[structureID].indexes[idx_dim].first = make_shared<BPlusTreeIndex>(make_shared<InMemoryStorage>(M_BLOCKSIZE), idxBinStart);
    pds[structureID].indexes[idx_dim].second = make_shared<BPlusTreeIndex>(make_shared<InMemoryStorage>(M_BLOCKSIZE), idxBinEnd);
    return allBins.size();
}

void restoreJoinMeta(int tableID, int binID, int pos, int join_idx, char *encryptedBin, uint dsize) {
    uint numRecords = dsize / EDATA_BLOCKSIZE;
    int structureID = tableID / 2;
    if ((int)pds.size() <= structureID) {
        pds.resize(structureID+1);
        pds.back().privtree = make_shared<PrivTree>();
    }
    if ((int)pds[structureID].privtree->leaves.size() <= pos) {
        pds[structureID].privtree->leaves.resize(pos+1);
        pds[structureID].privtree->leaves[pos] = new TreeNode();
        pds[structureID].privtree->leaves[pos]->bin1 = new vector<metaRecord>;
        pds[structureID].privtree->leaves[pos]->bin2 = new vector<metaRecord>;
        pds[structureID].privtree->leaves[pos]->id = binID;
    }
    pds[structureID].privtree->leaves[pos]->count[tableID] = numRecords;
    pds[structureID].privtree->binNoisyMax = max(pds[structureID].privtree->binNoisyMax, (int)numRecords);
    auto eiter = encryptedBin;
    uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    uchar *plaintext = new uchar[cipherSize];
    uchar *ciphertext = new uchar[cipherSize];
    uchar tag_in[TAG_SIZE];
    for (uint i = 0; i < numRecords; i++) {
        memcpy(ciphertext, eiter, cipherSize);
        eiter += cipherSize;
        memcpy(tag_in, eiter, TAG_SIZE);
        eiter += (EDATA_BLOCKSIZE - cipherSize);
        sgx_status_t status = SGX_SUCCESS;
        status = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *) aes_key, (const uint8_t *) ciphertext,
                        cipherSize, (uint8_t *) plaintext, (const uint8_t *) hardcoded_iv, IV_LENGTH,
                        NULL, 0, (const sgx_aes_gcm_128bit_tag_t*) tag_in);
        if(status != SGX_SUCCESS) {
	        printf("Decrypt failed\n");
        }
        vector<int> structuredRecord = deconstructNumbers<int>(plaintext, textSizeVec[tableID]);
        metaRecord mr;
        mr.rid = structuredRecord.back();
        mr.values.push_back(structuredRecord[join_idx]);
        if (tableID == 0) {
            pds[structureID].privtree->leaves[pos]->bin1->push_back(mr);
        } else {
            pds[structureID].privtree->leaves[pos]->bin2->push_back(mr);
        }
    }
}

int idxSelect(int structureID, int idx_dim, int queryStart, int queryEnd, char *s_bins, uint dsize, char *s_records, uint rdsize, join_type jtype) {
    number dimStart = 0;
    number dimEnd = ULLONG_MAX;
    bool binSearch = false;
    vector<pair<number,bytes>> result1, result2;
    pds[structureID].indexes[idx_dim].first->search(queryEnd+1, dimEnd, result1, binSearch);
    pds[structureID].indexes[idx_dim].second->search(dimStart, queryStart-1, result2, binSearch);
    vector<int> filterBins; 
    for (auto &&element : result1) {
        vector<number> value = deconstructNumbers(element.second);
        int binID = value[0];
        filterBins.push_back(binID);
    }
    for (auto &&element : result2) {
        vector<number> value = deconstructNumbers(element.second);
        int binID = value[0];
        filterBins.push_back(binID);
        // printf("%d ", binID);
    }
    sort(filterBins.begin(), filterBins.end());
    vector<int> selectedBins;
    if (allBins.empty()) {
        if (jtype == DO) {
            for (auto &&ptr : pds[structureID].privtree->leaves) {
                allBins.push_back(ptr->id);
            }
            sort(allBins.begin(), allBins.end());
        } else {
            for (int i = 0; i < pds[structureID].htree->boundaries.size(); i++) {
                allBins.push_back(i);
            }
        }
    }
    // sort(allBins.begin(), allBins.end());
    sortedVecDifference(allBins, filterBins, selectedBins);
    auto iter = s_bins;
    int nSelectedRecords = 0;
    for (auto bid : selectedBins) {
        if (jtype == HTreeDO) {
            nSelectedRecords += pds[structureID].htree->partitionSizes[bid].second;
        }
        memcpy(iter, &bid, sizeof(int));
        iter += sizeof(int);
    }
    memcpy(s_records, &nSelectedRecords, sizeof(int));
    // printf("selected bins %d records %d\n", selectedBins.size(), nSelectedRecords);
    return selectedBins.size();
    // compare and check correctness 
    // pds[structureID].privtree->checkSelction(queryStart, queryEnd, dim, &selectedBins);
}

int innerJoin(int structureID, int tableID1, int tableID2, int writeID, join_type jtype, int qs, int qe, int jattr) {
    int compactResNoise;
    if (jtype == DO) {
        printf("binNoisyMax is %d\n", pds[structureID].privtree->binNoisyMax);
        compactResNoise = TGeom(eps_c, delta_c, pds[structureID].privtree->binNoisyMax);
    } else if (jtype == HTreeDO) {
        printf("bucketNoisyMax is %d\n", pds[structureID].htree->bucketNoisyMax);
        compactResNoise = TGeom(eps_c, delta_c, pds[structureID].htree->bucketNoisyMax);
    }
    /* if (join_simulation) {
        int joinSize = pds[structureID].privtree->checkJoinOutput(0, 0); //join_idx1=0, join_idx2=0, hardcoded for debug
        printf("privtree join output len compact: %d\n", joinSize+compactResNoise);
        return joinSize;
    }  */
    printf("inner cross product join\n");
    pair<int, int> joinStats;
    if (jtype == DO) {
        if (jattr == 1) {
            // normal join
            joinStats = pds[structureID].privtree->join(tableID1, tableID2, writeID, qs, qe);
        } else {
            // select on non-join attr
            vector<TreeNode*> selectedBins = pds[structureID].privtree->select(tableID2, qs, qe);
            joinStats = pds[structureID-1].privtree->selectJoin(selectedBins, tableID1, tableID1, writeID, qs, qe);
            int updatedBinNoisyMax = max(pds[structureID].privtree->binNoisyMax, pds[structureID-1].privtree->binNoisyMax);
            compactResNoise = TGeom(eps_c, delta_c, updatedBinNoisyMax);
        }     
        pds[structureID].privtree->noisyOutputSize = joinStats.first + compactResNoise;
        printf("compacted noisy output size %d\n", pds[structureID].privtree->noisyOutputSize);
    } else if (jtype == HTreeDO) {
        if (jattr == 1) {
            // select on join attr
            joinStats = pds[structureID].htree->bucketCrossProduct(writeID, qs, qe, true);
        } else {
            // normal join
            joinStats = pds[structureID].htree->bucketCrossProduct(writeID, qs, qe, false);
        }
        pds[structureID].htree->noisyOutputSize = joinStats.first + compactResNoise;
        printf("compacted noisy output size %d\n", pds[structureID].htree->noisyOutputSize);
    }
    return joinStats.second;  // number of join matches written to untrusted storage
}

int pfJoin(int structureID, int tableID1, int tableID2, int writeID, join_type jtype) {
    printf("primary foreign key join\n");
    pair<int, int> joinStats;
    if (jtype == DO) {
        joinStats = pds[structureID].privtree->pfJoin(tableID1, tableID2, writeID);
    } else if (jtype == HTreeDO) {
        joinStats = pds[structureID].htree->pfJoin(tableID1, tableID2, writeID);
    }
    return joinStats.second;
}

void joinCompact(int structureID, int writeID, int us_size, join_type jtype) {
    /* vector<bool> distinguishedStatus;
    bool flipped = enclaveJoinConsolidate(pds[structureID].privtree->noisyOutputSize);
    distinguishedStatus.push_back(!flipped);
    
    
    
    int emptySlots = (int)IO_BLOCKSIZE - (int)OCALL_MAXSIZE - pds[structureID].privtree->noisyOutputSize;

    int pos = 0;
    while (pos < us_size && emptySlots > 0) {
        // joinOutput->resize((int)IO_BLOCKSIZE - (int)OCALL_MAXSIZE);
        // printf("joinOutput size %d\n", joinOutput->size);
        int readNum = min(emptySlots, us_size-pos);
        readDataBlock_Wrapper<cbytes>(writeID, writeID, 0, pos, joinOutput, pds[structureID].privtree->noisyOutputSize, readNum);
        pos += readNum;
        enclaveCompact(pds[structureID].privtree->noisyOutputSize);
        printf("repeated join compact\n");
    }
    real = 0;
    noise = 0;
    printf("joinOutput size %d\n", joinOutput->size);
    for (int i = 0; i < joinOutput->size; i++) {
        auto record = joinOutput->read(i);
        bytes recordBytes;
        for (int j = 0; j < sizeof(int); j++) {
            recordBytes.push_back(record[j]);
        }
        vector<int> recordInts = deconstructIntegers(writeID, recordBytes, sizeof(int));
        if (recordInts[0] >= 0) {
            if (recordInts[0] != INT_MAX) {
                real++;
            } else {
                noise++;
            }
        }
    }
    printf("check compact real %d noise %d total %d\n", real, noise, real+noise);
    writeDataBlock_Wrapper(writeID, 0, 0, joinOutput, pds[structureID].privtree->noisyOutputSize); */
    if (us_size == 0) {
        int noisyOutputSize;
        if (jtype == DO) {
            noisyOutputSize = pds[structureID].privtree->noisyOutputSize;
        } else if (jtype == CZSC21) {
            noisyOutputSize = czsc_join->noisyOutputSize;
        } else {
            noisyOutputSize = pds[structureID].htree->noisyOutputSize;
        }
        if (noisyOutputSize < joinOutput->size) {
            printf("In enclave join compact\n");
            // enclaveCompact(noisyOutputSize); //Goodrich
            ORCompact(noisyOutputSize); // faster
            writeDataBlock_Wrapper(writeID, 0, 0, joinOutput, noisyOutputSize);
            printf("compact done\n");
        }
    } else {
        if ((jtype == DO && pds[structureID].privtree->noisyOutputSize > us_size) || (jtype == CZSC21 && czsc_join->noisyOutputSize > us_size)) {
            return; // a signal of error
        }
        printf("mixed join compact\n");
        initBinStorage_OCALL(writeID, 0);
        jWorkSpace[0] = new TraceMem<cbytes>(0);
        jWorkSpace[1] = new TraceMem<cbytes>(0);
        CompactNet<cbytes> cnet(jWorkSpace[0], jWorkSpace[1]);
        //! this function is incorrect
        cnet.compact(writeID, 0, us_size, 1, [](vector<int> &match) {
            return match[0] != INT_MAX;
        }, true);   
        printf("compact done\n");
    }

    joinOutput->freeSpace();
    free(selected_count);
    selected_count_idx = 1;
    realMatches = 0;
    jDistArray.clear();
}

int czscJoin(int tableID1, int tableID2, int writeID, int pfJoin) {
    //TODO add compaction
    printf("keyNoisyMax is %d\n", czsc_join->keyNoisyMax);
    int compactResNoise = TGeom(eps_c, delta_c, czsc_join->keyNoisyMax);
    if (join_simulation) {
        int joinSize = czsc_join->checkOutputLen(czsc_optimized);
        printf("CZSC21 join output len compact: %d\n", joinSize+compactResNoise);
        return joinSize;

    } else {
        bool isPFJoin = false;
        if (pfJoin != 0) {
            isPFJoin = true;
            printf("CZSC PFJoin\n");
        } else {
            printf("CZSC Join\n");
        }
        pair<int, int> joinStats = czsc_join->join(writeID, isPFJoin);
        if (!isPFJoin) {
            int compactResSize = joinStats.first + compactResNoise;
            czsc_join->noisyOutputSize = compactResSize;
            printf("compacted noisy output size %d\n", compactResSize);
        }
        // initBinStorage_OCALL(writeID, 0);
        // compact(writeID, 0, joinStats.second, 1, nullptr, [](vector<int> &match) {
        //     return match[0] >= 0;
        // }, true);
        // move_resize_bin_totable_OCALL(writeID, 0, 0, compactResSize);
        return joinStats.second;
    }
}