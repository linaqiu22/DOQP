#include "czsc-join.hpp"
#include "Enclave-utility.hpp"
#include "oblivious-blocks.hpp"

CZSCJoin::CZSCJoin(int tableID1, int tableID2, vector<metaRecord> *t1_metadata, vector<metaRecord> *t2_metadata, double eps, double delta) : tableID1(tableID1), tableID2(tableID2), eps(eps), delta(delta) {
    tWorkSpace[0] = new TraceMem<tbytes>(0);
    tWorkSpace[1] = new TraceMem<tbytes>(0);
    U = 2*(1/eps*log(2/delta));
    t1data_size = t1_metadata->size();
    t2data_size = t2_metadata->size();
    JoinRecord e;
    vector<JoinRecord> input;
    for (uint i = 0; i < t1_metadata->size(); i++) {
        vector<int> key(t1_metadata->at(i).values.begin(), t1_metadata->at(i).values.begin()+sensitive_dim);
        e = {tableID1, t1_metadata->at(i).rid, key, -1};
        input.push_back(e);
        T1.push_back(e);
    }
    vector<metaRecord>().swap(*t1_metadata);
    for (uint i = 0; i < t2_metadata->size(); i++) {
        vector<int> key(t2_metadata->at(i).values.begin(), t2_metadata->at(i).values.begin()+sensitive_dim);
        e = {tableID2, t2_metadata->at(i).rid, key, -1};
        input.push_back(e);
        T2.push_back(e);
    }
    vector<metaRecord>().swap(*t2_metadata);

    ObliSortingNet<JoinRecord> net1 = ObliSortingNet<JoinRecord>(&input, [](JoinRecord &r1, JoinRecord &r2) {
        if (r1.key == r2.key) {
            return r1.table < r2.table;
        } else {
            return r1.key < r2.key;
        }
    });
    net1.obliSort(input.size());
    int realKeys = populateKeyBinList(&input);
    vector<JoinRecord>().swap(input);
    ObliSortingNet<KeyBin> net2 = ObliSortingNet<KeyBin>(&keyBinList, [](KeyBin &e1, KeyBin &e2) {
        if (e1.key[0] == INT_MIN || e2.key[0] == INT_MIN) {
            return e1.key[0] > e2.key[0];
        }
        int e1_total1 = e1.noise1 + e1.count1;
        int e2_total1 = e2.noise1 + e2.count1;
        if (e1_total1 == e2_total1) {
            return e1.noise2+e1.count2 > e2.noise2+e2.count2;
        } else {
            return e1_total1 > e2_total1;
        }
    });
    net2.obliSort(keyBinList.size());
    printf("sort keybin list\n");
    pair<int, int> noiseSum = groupSparseKeys();
    printf("group sparse keys\n");
    if (noiseSum.first + T1.size() <= IO_BLOCKSIZE) {
        t1_inEnclave = true;
    } else {
        t1_inEnclave = false;
    }
    if (noiseSum.second + T2.size() <= IO_BLOCKSIZE) {
        t2_inEnclave = true;
    } else {
        t2_inEnclave = false;
    }
    if (!join_simulation) {
        expandJoinTables(noiseSum.first, noiseSum.second);
        if (!t1_inEnclave) {
            initBinStorage_OCALL(tableID1, 0);
            createBins(tableID1);
        }
        if (!t2_inEnclave) {
            initBinStorage_OCALL(tableID2, 0);
            createBins(tableID2);
        }
    }
}

int CZSCJoin::populateKeyBinList(vector<JoinRecord> *data) {
    vector<int> curKey, dummyKey(sensitive_dim, INT_MIN); 
    int count1 = 0, count2 = 0, realKeys = 0;
    if (!data->empty()) {
        curKey = data->at(0).key;
        if (data->at(0).table == tableID2) {
            count2 = 1;
        } else {
            count1 = 1;
        }
    }
    eps_g = (eps_d + eps_r);
    delta_g = delta - delta_c;
    for (uint i = 1; i < data->size(); i++) {
        // int noise1 = TGeom(eps_g/3*2, delta_g/3*2, 1);
        // int noise2 = TGeom(eps_g/3*2, delta_g/3*2, 1);
        int noise1 = TGeom(eps_g, delta_g, 1);
        int noise2 = TGeom(eps_g, delta_g, 1);
        if (data->at(i).key == curKey) {
            if (data->at(i).table == tableID2) {
                count2++;
            } else {
                count1++;
            }
            // INT_MIN is a flag for dummy entry
            keyBinList.push_back({dummyKey, 0, noise1, 0, noise2, (int)i-1});
            //* tmp 
            // keyBinList.push_back({INT_MIN+(int)i-1, 0, noise1, 0, noise2, (int)i-1});
            // noiseArray1.push_back({noiseSum1, {tableID1, INT_MAX, INT_MIN+(int)i-1, (int)i-1}});
            // noiseArray2.push_back({noiseSum2, {tableID2, INT_MAX, INT_MIN+(int)i-1, (int)i-1}});
            realKeys += 0;
        } else {
            keyBinList.push_back({curKey, count1, noise1, count2, noise2, (int)i-1});
            
            //*tmp
            // noiseArray1.push_back({noiseSum1, {tableID1, INT_MAX, curKey, (int)i-1}});
            // noiseArray2.push_back({noiseSum2, {tableID2, INT_MAX, curKey, (int)i-1}});
            
            realKeys++;
            curKey = data->at(i).key;
            if (data->at(i).table == tableID2) {
                count2 = 1;
                count1 = 0;
            } else {
                count1 = 1;
                count2 = 0;
            }
        }
        //*tmp
        // noiseSum1 += noise1;
        // noiseSum2 += noise2;
        
        int tmpNoisy = max(keyBinList.back().count1+keyBinList.back().noise1, keyBinList.back().count2+keyBinList.back().noise2);
        keyNoisyMax = max(tmpNoisy, keyNoisyMax);
    }
    // int noise1 = TGeom(eps_g/3*2, delta_g/3*2, 1);
    // int noise2 = TGeom(eps_g/3*2, delta_g/3*2, 1);
    int noise1 = TGeom(eps_g, delta_g, 1);
    int noise2 = TGeom(eps_g, delta_g, 1);
    keyBinList.push_back({curKey, count1, noise1, count2, noise2, (int)data->size()-1});
    
    //*tmp
    // noiseArray1.push_back({noiseSum1, {tableID1, INT_MAX, curKey, (int)data->size()-1}});
    // noiseArray2.push_back({noiseSum2, {tableID2, INT_MAX, curKey, (int)data->size()-1}});
    // noiseSum1 += noise1;
    // noiseSum2 += noise2;

    realKeys++;
    int tmpNoisy = max(keyBinList.back().count1+keyBinList.back().noise1, keyBinList.back().count2+keyBinList.back().noise2);
    keyNoisyMax = max(tmpNoisy, keyNoisyMax);
    //*tmp
    // int compactResNoise = TGeom(eps_c, delta_c, keyNoisyMax);
    // printf("add noise %d\n", compactResNoise);
    return realKeys;
}

pair<int, int> CZSCJoin::groupSparseKeys() {
    // expand table 
    int noiseSum1 = 0, noiseSum2 = 0;
    for (auto &e : keyBinList) {
        int noisyCount1 = e.count1+e.noise1;
        int noisyCount2 = e.count2+e.noise2;
        if (noisyCount1 > 2*U || noisyCount2 > 2*U) {
            denseBins.push_back({ndense, noisyCount1, noisyCount2});
            e.bin_id = ndense;
            ndense++;
            noiseArray1.push_back({noiseSum1, {tableID1, INT_MAX, e.key, e.bin_id}});
            noiseArray2.push_back({noiseSum2, {tableID2, INT_MAX, e.key, e.bin_id}});
            noiseSum1 += e.noise1;
            noiseSum2 += e.noise2;
            crossProductSize += (e.count1+e.noise1) * (e.count2+e.noise2);
        } else {
            e.bin_id = -1;
        }
    }
    int sbinCount1 = 0, sbinCount2 = 0;
    vector<int> preKey(sensitive_dim, INT_MIN), dummyKey(sensitive_dim, INT_MIN);
    for (auto &e : keyBinList) {
        if (e.bin_id != -1) {
            continue;
        }
        if (e.count1 > 0 || e.count2 > 0) {
            n_sparsekeys++;
        }
        int noisyCount1 = e.count1 + e.noise1;
        int noisyCount2 = e.count2 + e.noise2;
        if (noisyCount1 <= 2*U && noisyCount2 <= 2*U) {
            if (sbinCount1+e.count1 > 4*U || sbinCount2+e.count2 > 4*U) {
                noiseArray1.push_back({noiseSum1, {tableID1, INT_MAX, preKey, ndense + n_sparsebins}});
                noiseSum1 += (4*U - sbinCount1);
                noiseArray2.push_back({noiseSum2, {tableID2, INT_MAX, preKey, ndense + n_sparsebins}});
                noiseSum2 += (4*U - sbinCount2);
                n_sparsebins++;
                sbinCount1 = e.count1; sbinCount2 = e.count2;
                if (e.key != dummyKey){
                    preKey = e.key;
                }
            } else {
                noiseArray1.push_back({INT_MAX, {tableID1, INT_MAX, dummyKey, ndense + n_sparsebins}});
                noiseArray2.push_back({INT_MAX, {tableID2, INT_MAX, dummyKey, ndense + n_sparsebins}});
                sbinCount1 += e.count1; sbinCount2 += e.count2;
                if (e.key != dummyKey) {
                    preKey = e.key;
                }
            }
            e.bin_id = ndense + n_sparsebins;
        }
    }
    noiseArray1.push_back({noiseSum1, {tableID1, INT_MAX, preKey, ndense + n_sparsebins}});
    noiseSum1 += (4*U - sbinCount1);
    noiseArray2.push_back({noiseSum2, {tableID2, INT_MAX, preKey, ndense + n_sparsebins}});
    noiseSum2 += (4*U - sbinCount2);
    n_sparsebins++;
    
    noisyBins = TGeom(eps_g/3, delta_g/3, 1); 
    return {noiseSum1, noiseSum2};
}

void CZSCJoin::expandJoinTables(int noiseSum1, int noiseSum2) {
    compactMetaVector<pair<int, JoinRecord>>(&noiseArray1, nullptr, nullptr, [](pair<int, JoinRecord> &e){
        return e.first < INT_MAX;
    }, ndense+n_sparsebins+noisyBins);
    compactMetaVector<pair<int, JoinRecord>>(&noiseArray2, nullptr, nullptr, [](pair<int, JoinRecord> &e){
        return e.first < INT_MAX;
    }, ndense+n_sparsebins+noisyBins);
    //* tmp
    int N = T1.size() + T2.size();
    // noisyBins = 0;
    // noisyBins = N/(2*U) - n_sparsebins; //* set noisyBins to pad nSparse to constant
    // noiseArray1.resize(ndense+n_sparsebins+noisyBins);
    // noiseArray2.resize(ndense+n_sparsebins+noisyBins);

    for (int i = 0; i < ndense+n_sparsebins+noisyBins; i++) {
        if (i >= ndense+n_sparsebins) {
            noiseArray1[i].first = noiseSum1;
            noiseArray1[i].second.bin_id = i;
            noiseArray1[i].second.key[0] = INT_MIN + (i-ndense-n_sparsebins) + 1;
            noiseArray1[i].second.rid = INT_MAX;
            noiseArray2[i].first = noiseSum2;
            noiseArray2[i].second.bin_id = i;
            noiseArray2[i].second.key[0] = INT_MIN + (i-ndense-n_sparsebins) + 1;
            noiseArray2[i].second.rid = INT_MAX;
            noiseSum1 += 4*U;
            noiseSum2 += 4*U;
            bool flag = false;
            for (auto &&e : keyBinList) {
                if (!flag && e.key[0] == INT_MIN) {
                    e.key = noiseArray1[i].second.key;
                    e.bin_id = i;
                    flag = true;
                }
            }
        }
    }
    n_sparsebins += noisyBins;
    printf("ndense %d nsparse %d N/2U %d U %d\n", ndense, n_sparsebins, N/(2*U), U);
    printf("crossProductSize before %d ", crossProductSize);
    crossProductSize += n_sparsebins*16*U*U;
    printf("crossProductSize %llu\n", crossProductSize);
    noiseArray1.resize(noiseSum1);
    expand(noiseArray1);
    noiseArray2.resize(noiseSum2);
    expand(noiseArray2);
    
    //* expand table OCALL
    if (!t1_inEnclave) {
        expandTable(tableID1);
        for (auto &&record : T1) {
            for (auto &&e : keyBinList) {
                if (e.key == record.key) {
                    record.bin_id = e.bin_id;
                }
            }
        }
        for (auto &&dummy : noiseArray1) {
            T1.push_back(dummy.second);
        }
    } else {
        vector<JoinRecord>().swap(T1);
        rbin1 = new TraceMem<tbytes>(noiseSum1+t1data_size);
        char *dummyRecord = new char[textSizeVec[tableID1]]; // hardcoded tableID1
        int binid_pos = textSizeVec[tableID1] - sizeof(int);
        int rid_pos = binid_pos - sizeof(int);
        int mark = INT_MAX;
        tbytes record;
        for (auto &&dummy : noiseArray1) {
            for (int k = 0; k < sensitive_dim; k++) {
                memcpy(dummyRecord+k*sizeof(int), &dummy.second.key[k], sizeof(int));
            }
            memcpy(dummyRecord+rid_pos, &mark, sizeof(int));
            memcpy(dummyRecord+binid_pos, &dummy.second.bin_id, sizeof(int));
            for (int j = 0; j < textSizeVec[tableID1]; j++) {
                record[j] = dummyRecord[j];
            }
            rbin1->write(record);
        }
        readDataBlock_Wrapper(tableID1, tableID1, 0, 0, rbin1, noiseSum1, t1data_size, &keyBinList);

        //* if use iterative compaction, requires bin id
        //* currently use obli sort
        ObliSortingNet<tbytes> net = ObliSortingNet<tbytes>(rbin1, [&binid_pos](tbytes &r1, tbytes &r2) {
            uchar tmpInt[sizeof(int)];
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r1[binid_pos+k];
            }
            vector<int> r1Bin = deconstructNumbers<int>(tmpInt, sizeof(int));
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r1[binid_pos - sizeof(int)+k];
            }
            vector<int> r1ID = deconstructNumbers<int>(tmpInt, sizeof(int));
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r2[binid_pos+k];
            }
            vector<int> r2Bin = deconstructNumbers<int>(tmpInt, sizeof(int));
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r2[binid_pos - sizeof(int)+k];
            }
            vector<int> r2ID = deconstructNumbers<int>(tmpInt, sizeof(int));
            if (r1Bin[0] == r2Bin[0]) {
                return r1ID[0] < r2ID[0];
            } else {
                return r1Bin[0] < r2Bin[0];
            }
        });
        net.obliSort(rbin1->size, true);
    }
    if (!t2_inEnclave) {
        expandTable(tableID2); 
        for (auto &&record : T2) {
            for (auto &&e : keyBinList) {
                if (e.key == record.key) {
                    record.bin_id = e.bin_id;
                }
            }
        }
        for (auto &&dummy : noiseArray2) {
            T2.push_back(dummy.second);
        }
    } else {
        vector<JoinRecord>().swap(T2);
        rbin2 = new TraceMem<tbytes>(noiseSum2+t2data_size);
        char *dummyRecord = new char[textSizeVec[tableID2]]; // hardcoded tableID1
        int binid_pos = textSizeVec[tableID2] - sizeof(int);
        int rid_pos = binid_pos - sizeof(int);
        int mark = INT_MAX;
        tbytes record;
        for (auto &&dummy : noiseArray2) {
            for (int k = 0; k < sensitive_dim; k++) {
                memcpy(dummyRecord+k*sizeof(int), &dummy.second.key[k], sizeof(int));
            }
            memcpy(dummyRecord+rid_pos, &mark, sizeof(int));
            memcpy(dummyRecord+binid_pos, &dummy.second.bin_id, sizeof(int));
            for (int j = 0; j < textSizeVec[tableID2]; j++) {
                record[j] = dummyRecord[j];
            }
            rbin2->write(record);
        }
        readDataBlock_Wrapper(tableID2, tableID2, 0, 0, rbin2, noiseSum2, t2data_size, &keyBinList);

        //* if use iterative compaction, requires bin id
        //* currently use obli sort
        ObliSortingNet<tbytes> net = ObliSortingNet<tbytes>(rbin2, [&binid_pos](tbytes &r1, tbytes &r2) {
            uchar tmpInt[sizeof(int)];
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r1[binid_pos+k];
            }
            vector<int> r1Bin = deconstructNumbers<int>(tmpInt, sizeof(int));
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r1[binid_pos - sizeof(int)+k];
            }
            vector<int> r1ID = deconstructNumbers<int>(tmpInt, sizeof(int));
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r2[binid_pos+k];
            }
            vector<int> r2Bin = deconstructNumbers<int>(tmpInt, sizeof(int));
            for (int k = 0; k < sizeof(int); k++) {
                tmpInt[k] = r2[binid_pos - sizeof(int)+k];
            }
            vector<int> r2ID = deconstructNumbers<int>(tmpInt, sizeof(int));
            if (r1Bin[0] == r2Bin[0]) {
                return r1ID[0] < r2ID[0];
            } else {
                return r1Bin[0] < r2Bin[0];
            }
        });
        net.obliSort(rbin2->size, true);
    }
}

void CZSCJoin::expandTable(int tableID) {
    auto &noiseArray = tableID == 0 ? noiseArray1 : noiseArray2;
    char *noise = (char *)malloc(2*IO_BLOCKSIZE*EDATA_BLOCKSIZE);
    char *dummyRecord = new char[textSizeVec[tableID]];
    char *encryptedDummy = new char[EDATA_BLOCKSIZE];
    int mark = INT_MAX;
    uint num_iter = ceil((float)noiseArray.size()/(2*IO_BLOCKSIZE));
    int rid_pos = textSizeVec[tableID] - sizeof(int)*2;
    for (uint i = 0; i < num_iter; i++) {
        auto noise_iter = noise;
        auto end = 2*(i+1)*IO_BLOCKSIZE < noiseArray.size() ? 2*(i+1)*IO_BLOCKSIZE : noiseArray.size();
        for (uint j = 2*i*IO_BLOCKSIZE; j < end; j++) {
            memcpy(dummyRecord, &noiseArray[j].second.key, sizeof(int));
            memcpy(dummyRecord+rid_pos, &noiseArray[j].second.rid, sizeof(int));
            memcpy(dummyRecord+rid_pos+sizeof(int), &noiseArray[j].second.bin_id, sizeof(int));
            encryptRecord(textSizeVec[tableID], dummyRecord, encryptedDummy);
            memcpy(noise_iter, encryptedDummy, EDATA_BLOCKSIZE);
            noise_iter += EDATA_BLOCKSIZE;
        }
        int n_noise = end - 2*i*IO_BLOCKSIZE;
        expand_table_OCALL(tableID, n_noise, noise, n_noise*EDATA_BLOCKSIZE);
    }
    delete []encryptedDummy;
    delete []dummyRecord;
    free(noise);
}

void CZSCJoin::createBins(int tableID) {
    tq.clear();
    int numBins = ndense + n_sparsebins;
    //* tmp
    // int numBins = keyBinList.size();
    if (tableID == 0) {
        tq.push_back(&T1);
    } else {
        tq.push_back(&T2);
    }
    binning(tableID, 0, numBins-1, 0);
    int counter = 0;
    for (auto &&ptr : tq) {
        if (ptr == nullptr) {
            counter++;
            continue;
        } else {
            int binID = ptr->at(0).bin_id;
            move_resize_bin_totable_OCALL(tableID, counter, binID, ptr->size());
            vector<JoinRecord>().swap(*ptr);
            counter++;
        }
    }
}

void CZSCJoin::binning(int tableID, int start, int end, int readBinID) {
    if (start == end) {
        return;
    }
    int mid = (start + end) / 2;
    int curSize = tq.size();
    int writeBinID = curSize;
    tq.resize(curSize + 2);
    tq[writeBinID] = new vector<JoinRecord>;
    tq[writeBinID+1] = new vector<JoinRecord>;
    vector<JoinRecord> *in = tq[readBinID];
    vector<JoinRecord> *out1 = tq[writeBinID];
    vector<JoinRecord> *out2 = tq[writeBinID+1];
    compactMetaVector<JoinRecord>(in, out1, out2, [&mid](JoinRecord &e) {
        return e.bin_id <= mid;
    });
    int out1_fill_size = (IO_BLOCKSIZE - (out1->size() % IO_BLOCKSIZE)) % IO_BLOCKSIZE;

    vector<JoinRecord>().swap(*tq[readBinID]);
    tq[readBinID] = nullptr;
    delete_bin_OCALL(tableID, readBinID);
    // printf("delete bin ocall table %d readBinID %d\n", tableID, readBinID);
    binning(tableID, start, mid, writeBinID);
    binning(tableID, mid+1, end, writeBinID+1);
}

pair<int, int> CZSCJoin::join(int writeID, bool pfJoin) {
    printf("start join\n");
    int numRealMatches = 0;
    uint outputSize = 0;
    int numBins = ndense+n_sparsebins;
    int binNoisyMax = max(keyNoisyMax, 4*U);
    int readBlockSize = min(binNoisyMax, OCALL_MAXSIZE);
    if (pfJoin) {
        readBlockSize = binNoisyMax;
    }
    TraceMem<pair<int, tbytes>> binpair(binNoisyMax*2);
    int binid_pos1 = textSizeVec[tableID1] - sizeof(int);
    int binid_pos2 = textSizeVec[tableID2] - sizeof(int);
    uchar tmpInt[sizeof(int)];
    char *dblock1, *dblock2;
    if (!t1_inEnclave) {
        dblock1 = (char*)malloc(readBlockSize * EDATA_BLOCKSIZE);
        printf("table 1 not in enclave; ");
    }
    if (!t2_inEnclave) {
        dblock2 = (char*)malloc(readBlockSize * EDATA_BLOCKSIZE);
        printf("table 2 not in enclave; ");
    }
    bool useOCALL = false;
    if (!pfJoin && crossProductSize > 2*IO_BLOCKSIZE) {
        useOCALL = true;
        printf("intermediate join output written to untrusted storage %d\n", crossProductSize);
    }
    if (!useOCALL) {
        if (!pfJoin) {
            joinOutput = new TraceMem<cbytes>(crossProductSize);
            printf("intermediate join output kept in enclave, X product size %u \n", crossProductSize);
            selected_count = new uint32_t[crossProductSize+1];
            selected_count[0] = 0;
        } else {
            joinOutput = new TraceMem<cbytes>(0);
        }
    }
    //*tmp
    // int numBins = keyBinList.size();
    int abs_pos1 = 0, abs_pos2 = 0; // for in enclave cross product
    int accumulate = 0;

    int t1_rid_pos = textSizeVec[tableID1] - sizeof(int)*2; // for in enclave pfjoin
    int t2_rid_pos = textSizeVec[tableID2] - sizeof(int)*2;
    int textSize = t1_rid_pos + t2_rid_pos + sizeof(int);
    char *plaintext = new char[textSize];
    char *encryptedMatch = new char[EDATA_BLOCKSIZE];
    char *res = (char*)malloc(min(binNoisyMax, OCALL_MAXSIZE) * EDATA_BLOCKSIZE);
    
    for (int i = 0; i < numBins; i++) {
        int readFlag1 = readBlockSize;
        int iter1 = 0;
        if (pfJoin) {
            binpair.resize(binNoisyMax*2);
            binpair.count = 0;
            joinOutput->resize(binNoisyMax*2);
        }
        while (iter1 * readBlockSize < readFlag1) {
            int pos1 = iter1 * readBlockSize;
            iter1++;
            int nreal1 = -1;
            if (!t1_inEnclave) {
                readDataBlock_OCALL(&readFlag1, tableID1, i, pos1, dblock1, readBlockSize * EDATA_BLOCKSIZE);
                if (readFlag1 < iter1 * readBlockSize) {
                    nreal1 = readFlag1 - (iter1-1) * readBlockSize;
                } else {
                    nreal1 = readBlockSize;
                }
                tWorkSpace[0]->resize(nreal1);
                parseDataBlock<tbytes>(tableID1, tWorkSpace[0], dblock1, 0, nreal1);
            } else {
                tWorkSpace[0]->resize(readBlockSize);
                for (int j = 0; j < readBlockSize; j++) {
                    if (abs_pos1+pos1+j >= rbin1->size) {
                        nreal1 = j;
                        abs_pos1 += nreal1;
                        break;
                    }
                    auto record = rbin1->read(abs_pos1+pos1+j);
                    for (int k = 0; k < sizeof(int); k++) {
                        tmpInt[k] = record[binid_pos1+k];
                    }
                    vector<int> r1Bin = deconstructNumbers<int>(tmpInt, sizeof(int));
                    if (r1Bin[0] > i) {
                        if (r1Bin[0] == i+1) {
                            nreal1 = j;
                            abs_pos1 += nreal1;
                            break;
                        }
                    }
                    if (!pfJoin) {
                        tWorkSpace[0]->write(j, record);
                    } else {
                        binpair.write({0, record});
                    }
                }
                if (nreal1 == -1) {
                    nreal1 = readBlockSize;
                    abs_pos1 += nreal1;
                }
                tWorkSpace[0]->resize(nreal1);
            }
            int readFlag2 = readBlockSize;
            int iter2 = 0;
            while (iter2 * readBlockSize < readFlag2) {
                int pos2 = iter2 * readBlockSize;
                iter2++;
                int nreal2 = -1;
                if (!t2_inEnclave) {
                    readDataBlock_OCALL(&readFlag2, tableID2, i, pos2, dblock2, readBlockSize * EDATA_BLOCKSIZE);
                    if (readFlag2 < iter2 * readBlockSize) {
                        nreal2 = readFlag2 - (iter2-1) * readBlockSize;
                    } else {
                        nreal2 = readBlockSize;
                    }
                    tWorkSpace[1]->resize(nreal2);
                    parseDataBlock<tbytes>(tableID2, tWorkSpace[1], dblock2, 0, nreal2);
                } else {
                    tWorkSpace[1]->resize(readBlockSize);
                    for (int j = 0; j < readBlockSize; j++) {
                        if (abs_pos2+pos2+j >= rbin2->size) {
                            nreal2 = j;
                            abs_pos2 += nreal2;
                            break;
                        }
                        auto record = rbin2->read(abs_pos2+pos2+j);
                        for (int k = 0; k < sizeof(int); k++) {
                            tmpInt[k] = record[binid_pos2+k];
                        }
                        vector<int> r2Bin = deconstructNumbers<int>(tmpInt, sizeof(int));
                        if (r2Bin[0] > i) { //* or != i
                            if (r2Bin[0] == i+1) {
                                nreal2 = j;
                                abs_pos2 += nreal2;
                                break;
                            }
                        }
                        if (!pfJoin) {
                            tWorkSpace[1]->write(j, record);
                        } else {
                            binpair.write({1, record});
                        }
                    }
                    if (nreal2 == -1) {
                        nreal2 = readBlockSize;
                        abs_pos2 += nreal2;
                    }
                    tWorkSpace[1]->resize(nreal2);
                }
                if (pfJoin) {
                    binpair.resize(nreal1+nreal2);
                    numRealMatches = sortMergeJoin(&binpair);
                    joinOutput->resize(nreal1+nreal2);
                    enclaveCompact(nreal2);
                    int counter = 0;
                    auto riter = res;
                    for (int j = 0; j < nreal2; j++) {
                        auto joinRow = joinOutput->read(j);
                        for (int k = 0; k < textSize; k++) {
                            plaintext[k] = joinRow[k];
                        }
                        encryptRecord(textSize, plaintext, encryptedMatch);
                        memcpy(riter, encryptedMatch, EDATA_BLOCKSIZE);
                        riter += EDATA_BLOCKSIZE;
                        counter++;
                        if (counter == OCALL_MAXSIZE) {
                            write_join_output_OCALL(writeID, res, OCALL_MAXSIZE*EDATA_BLOCKSIZE);
                            riter = res;
                            counter = 0;
                        }
                    } 
                    if (counter > 0) {
                        write_join_output_OCALL(writeID, res, counter*EDATA_BLOCKSIZE);
                    }
                    outputSize += nreal2;
                } else {
                    numRealMatches = crossProduct(tWorkSpace[0], tWorkSpace[1], tableID1, tableID2, nreal1, nreal2, writeID, useOCALL);
                    outputSize += (nreal1 * nreal2);
                }
            }
        }
    }
    if (!t1_inEnclave) {
        free(dblock1);
    }
    if (!t2_inEnclave) {
        free(dblock2);
    }
    binpair.freeSpace();
    free(res);
    delete []plaintext;
    delete []encryptedMatch;
    printf("correctness check: real matches %d\n", numRealMatches);
    printf("noisy output size %u\n", outputSize);
    int inEnclaveSize = useOCALL ? 0 : joinOutput->count;
    return {numRealMatches, outputSize-inEnclaveSize};
    // return {numRealMatches, outputSize};
}

int CZSCJoin::checkOutputLen(bool optimized) {
    int rlen = 0;
    unsigned long len = 0;
    for (auto &e : keyBinList) {
        rlen += (e.count1*e.count2);
        len += (e.count1+e.noise1)*(e.count2+e.noise2);
    }
    printf("join result size: %d\n", rlen);
    printf("Normal czsc21 join output len: %lu\n", len);
    len = 0;
    for (int i = 0; i < ndense; i++) {
        len += (get<1>(denseBins[i])*get<2>(denseBins[i]));
    }
    len += 16*pow(U,2)*(n_sparsebins+noisyBins);
    // int N = T1.size() + T2.size();
    // len += 16*pow(U,2)* N / (2*U);
    printf("ndense %d nsparse %d noisyBins %d\n", ndense, n_sparsebins, noisyBins);
    printf("Keys consolidated czsc21 join output len: %lu\n", len);
    return rlen;
}