#include "htree.hpp"

HTree::HTree(vector<metaRecord> *t1meta, vector<metaRecord> *t2meta, vector<pair<int, int> > initDomain, double eps, double delta, bool pfjoin, float constant, bool uni):
                                                    t1meta(t1meta), t2meta(t2meta), eps(eps), delta(delta), pfjoin(pfjoin), constant(constant), uni(uni) {
    //sort t1meta and t2meta by the key attribute
    //scan to obtain histogram, first compact then expand
    //build rangeSanitizer
    //construct buckets (sort again)
    t1Size = t1meta->size();
    t2Size = t2meta->size();
    keyMin = initDomain[0].first;
    keyMax = initDomain[0].second;
    domainSize = keyMax - keyMin + 1;
    printf("keyMax %d keyMin %d domainsize %d\n", keyMax, keyMin, domainSize);
    height = ceil(log(domainSize) / log(K)) + 1;
    //* show the effect of CreateBuckets
    eps_h = 0.2 * eps; delta_h = 0.2 * delta;
    eps_b = eps - eps_h; delta_b = delta - delta_h;

    if (!t1meta->empty() && !t2meta->empty()) {
        accumulateSizes.resize(2);
    } else {
        accumulateSizes.resize(1);
    }

    //* independent
    if (!uni) {
        if (!pfjoin && !t1meta->empty()) {
            ObliSortingNet<metaRecord> net1 = ObliSortingNet<metaRecord>(t1meta, [](metaRecord &r1, metaRecord &r2) {
                return r1.values[0] < r2.values[0];
            });
            net1.obliSort(t1meta->size());
        }
        if (!t2meta->empty()) {
            ObliSortingNet<metaRecord> net2 = ObliSortingNet<metaRecord>(t2meta, [](metaRecord &r1, metaRecord &r2) {
                return r1.values[0] < r2.values[0];
            });
            net2.obliSort(t2meta->size());
        }
    }
}

void HTree::getHistBins(int tableID) {
    auto meta = tableID == 0 ? t1meta : t2meta;
    Sanitizer hbins(1, TraceMem<HNode>(meta->size()));
    int preKey = meta->at(0).values[0];
    int count = 1;
    for (int i = 1; i < meta->size(); i++) {
        if (meta->at(i).values[0] == preKey) {
            hbins[0].write(i-1, {-1, INT_MIN});
            count++;
        } else {
            hbins[0].write(i-1, {preKey-keyMin, count});
            count = 1;
            preKey = meta->at(i).values[0];
        }
    }
    hbins[0].write(meta->size()-1, {preKey-keyMin, count});

    //compaction
    compactMeta<HNode>(&hbins[0], nullptr, nullptr, [](HNode &e) {
        return e.second != INT_MIN;
    }, domainSize);
    expand(&hbins[0]);
    for (int i = 0; i < domainSize; i++) {
        auto pr = hbins[0].read(i);
        if (pr.first == i) {
            pr.first = pr.second;
        } else {
            pr.first = 0;
        }
        int noise = TGeom(eps_h, delta_h, height, false);
        pr.second = noise;
        hbins[0].write(i, pr);
    }
    rangeSanitizer.push_back(hbins);
}

void HTree::populateRangeSanitizer(int tableID, bool ci) {
    int remainder = 0;
    int buckets = domainSize;
    if (buckets % K > 0) {
        remainder = 1;
    } else {
        remainder = 0;
    }
    // num of bucket at level 1 (level 0 is leaves)
    buckets = buckets / K + remainder;
    // bottom up
    for (int l = 1; l < height; l++) {
        rangeSanitizer[tableID].push_back(TraceMem<HNode>(buckets));
        for (int j = 0; j < buckets; j++) {
            int noise = TGeom(eps_h, delta_h, height, false);
            HNode hnode;
            int realSum = 0;
            int weightedNosiySum = 0;
            int endIdx = j == buckets-1 ? rangeSanitizer[tableID][l-1].size : (j+1)*K;
            for (int k = j*K; k < endIdx; k++) {
                auto pr = rangeSanitizer[tableID][l-1].read(k);
                realSum += pr.first;
                weightedNosiySum += pr.second;
            }
            if (ci) {
                float z = (pow(K, l+1)-pow(K, l))/(pow(K, l+1)-1.0)*(noise+realSum) + (pow(K, l)-1.0)/(pow(K, l+1)-1.0)*(weightedNosiySum+realSum);
                hnode = {realSum, (int)z-realSum};
            } else {
                hnode = {realSum, noise};
            }
            rangeSanitizer[tableID][l].write(j, hnode);
        }
        if (buckets % K > 0) {
            remainder = 1;
        } else {
            remainder = 0;
        }
        buckets = buckets / K + remainder;
    }
    // top down
    if (ci) {
        for (int l = height-1; l >= 1; l--) {
            for (int j = 0; j < (int)rangeSanitizer[tableID][l].size; j++) {
                int childZSum = 0;
                int leftChild = j*K;
                int rightChild = min((int)rangeSanitizer[tableID][l-1].size, (j+1)*K);
                for (int k = leftChild; k < rightChild; k++) {
                    auto pr = rangeSanitizer[tableID][l-1].read(k);
                    childZSum += pr.second;
                }
                float diff = (float)rangeSanitizer[tableID][l].read(j).second - (float)childZSum;
                diff = diff / (float)(rightChild - leftChild);
                for (int k = leftChild; k < rightChild; k++) {
                    // if (diff < 0 && abs(diff) > rangeSanitizer[tableID][l-1][k].noise) {
                    //     rangeSanitizer[tableID][l-1][k].noise = 0;
                    // } else {
                    auto pr = rangeSanitizer[tableID][l-1].read(k);
                    pr.second += diff;
                    rangeSanitizer[tableID][l-1].write(k, pr);
                    // if (rangeSanitizer[tableID][l-1][k].second + rangeSanitizer[tableID][l-1][k].first < 0) {
                        // rangeSanitizer[tableID][l-1][k].second = -rangeSanitizer[tableID][l-1][k].first;
                        // printf("possible error level %d node %d real %d dummy %d\n", l-1, k, rangeSanitizer[tableID][l-1][k].first, rangeSanitizer[tableID][l-1][k].second);
                    // }
                    // }
                }
            }
        }
    }
}

pair<int, int> HTree::BRC(Sanitizer *sanitizer, int from, int to) {
        float noise = 0;
        int realData = 0;
		int level = 0; // leaf-level, bottom
		do {
			while ((from % K != 0 || level == sanitizer->size()-1) && from < to) {
                auto pr = sanitizer->at(level).read(from);
                noise += pr.second;
                realData += pr.first;
				from++;
			}
			while ((to % K != K - 1 || level == sanitizer->size()-1) && from < to) {
                auto pr = sanitizer->at(level).read(to);
                noise += pr.second;
                realData += pr.first;
				to--;
			}
			if (from != to) {
				from /= K;
				to /= K;
				level++;
			} else {
                auto pr = sanitizer->at(level).read(from);
                noise += pr.second;
                realData += pr.first;
				return {realData, (int)realData+(int)noise};
			}
		} while (true);
}

// Uni-DO-joiin
void HTree::constructBucketsOneTime() {
    int t1Size = 0, noisyTotal0 = 0, t2Size = 0, noisyTotal1 = 0;
    if (!pfjoin) {
        for (int i = 0; i < domainSize; i++) {
            auto pr = rangeSanitizer[0][0].read(i);
            t1Size += pr.first;
            noisyTotal0 += pr.second;
        }
        noisyTotal0 += t1Size;
    } else {
        noisyTotal0 = domainSize;
    }

    for (int i = 0; i < domainSize; i++) {
        auto pr = rangeSanitizer[1][0].read(i);
        t2Size += pr.first;
        noisyTotal1 += pr.second;
    }
    noisyTotal1 += t2Size;
    int U = (2 / 0.3 * log(1/delta));
    int tmpPart = ceil((noisyTotal0+noisyTotal1) / U * height * constant);
    printf("t1Size %d t2Size %d noisyTotal0 %d noisyTotal1 %d tmpPart %d constant %f\n", t1Size, t2Size, noisyTotal0, noisyTotal1, tmpPart, constant);
    int avgPartSize = ceil((noisyTotal0 + noisyTotal1) / (float)tmpPart);
    printf("height %d, avgPartSize %d\n", height, avgPartSize);

    vector<pair<int, int>> partitionSizesTmp;
    int start = 0, end;
    int realPartSize1 = 0, noisyPartSize1 = 0, realPartSize2 = 0, noisyPartSize2 = 0;
    int accuReal1 = 0, accuPureNoise1 = 0, accuReal2 = 0, accuPureNoise2 = 0;
    int prevReal1 = -1, prevReal2 = -1;
    for (int i = 0; i < domainSize; i++) {
        auto pr1 = rangeSanitizer[0][0].read(i);
        auto pr2 = rangeSanitizer[1][0].read(i);
        realPartSize1 += pr1.first;
        noisyPartSize1 += (pr1.first+pr1.second);
        realPartSize2 += pr2.first;
        noisyPartSize2 += (pr2.first+pr2.second);
        if (noisyPartSize1+noisyPartSize2 > avgPartSize) {
            int noise1 = TGeom(eps_b, delta_b, 1);
            partitionSizes.push_back({realPartSize1, realPartSize1+noise1});
            int noise2 = TGeom(eps_b, delta_b, 1);
            partitionSizesTmp.push_back({realPartSize2, realPartSize2+noise2});
            if (accuReal1 <= prevReal1) {
                accumulateSizes[0].push_back({-1, accuPureNoise1});
            } else {
                accumulateSizes[0].push_back({accuReal1, accuPureNoise1});
                prevReal1 = accuReal1;
            }
            accuReal1 += realPartSize1;
            accuPureNoise1 += noise1;
            if (accuReal2 <= prevReal2) {
                accumulateSizes[1].push_back({-1, accuPureNoise2});
            } else {
                accumulateSizes[1].push_back({accuReal2, accuPureNoise2});
                prevReal2 = accuReal2;
            }
            accuReal2 += realPartSize2;
            accuPureNoise2 += noise2;
            boundaries.push_back(i);
            start = i+1;
            realPartSize1 = 0; noisyPartSize1 = 0;
            realPartSize2 = 0; noisyPartSize2 = 0;
        }   
        if (i == domainSize - 1) {
            if (boundaries.back() < domainSize - 1) {
                boundaries.push_back(domainSize-1);
                int noise1 = TGeom(eps_b, delta_b, 1);
                partitionSizes.push_back({realPartSize1, realPartSize1+noise1});
                int noise2 = TGeom(eps_b, delta_b, 1);
                partitionSizesTmp.push_back({realPartSize2, realPartSize2+noise2});
                if (accuReal1 <= prevReal1) {
                    accumulateSizes[0].push_back({-1, accuPureNoise1});
                } else {
                    accumulateSizes[0].push_back({accuReal1, accuPureNoise1});
                    prevReal1 = accuReal1;
                }
                if (accuReal2 <= prevReal2) {
                    accumulateSizes[1].push_back({-1, accuPureNoise2});
                } else {
                    accumulateSizes[1].push_back({accuReal2, accuPureNoise2});
                    prevReal2 = accuReal2;
                }
                accuReal1 += realPartSize1; accuPureNoise1 += noise1;
                accuReal2 += realPartSize2; accuPureNoise2 += noise2;
            }
        } 
    }
    numPart1 = boundaries.size();
    numPart2 = numPart1;
    boundaries.insert(boundaries.end(), boundaries.begin(), boundaries.end());
    partitionSizes.insert(partitionSizes.end(), partitionSizesTmp.begin(), partitionSizesTmp.end());
    int prev = 0;
    int keyStart, keyEnd;
    for (int i = 0; i < numPart1; i++) {
        keyStart = keyMin + prev; // inclusive
        keyEnd = keyMin + boundaries[i]; //inclusive
        Bucket *bucket = new Bucket(i, 0, keyStart, keyEnd, partitionSizes[i].second);
        bucketNoisyMax = max(bucketNoisyMax, partitionSizes[i].second);
        buckets.push_back(bucket);
        prev = boundaries[i] + 1;
    }
    prev = 0;  
    printf("numPart1 %d numPart2 %d\n", numPart1, numPart2);
    for (int i = numPart1; i < numPart1+numPart2; i++) {
        keyStart = keyMin + prev; // inclusive
        keyEnd = keyMin + boundaries[i]; //inclusive
        Bucket *bucket = new Bucket(i, 1, keyStart, keyEnd, partitionSizes[i].second);
        bucketNoisyMax = max(bucketNoisyMax, partitionSizes[i].second);
        buckets.push_back(bucket);
        prev = boundaries[i] + 1;
    }
    printf("pds1 size %d\n", accuReal1+accuPureNoise1);
    printf("pds2 size %d\n", accuReal2+accuPureNoise2);
}

// Ind-DO-join
void HTree::constructBuckets(int tableID) {
    int t1Size = 0, noisyTotal0 = 0, t2Size = 0, noisyTotal1 = 0;
    if (!pfjoin) {
        for (int i = 0; i < domainSize; i++) {
            auto pr = rangeSanitizer[0][0].read(i);
            t1Size += pr.first;
            noisyTotal0 += pr.second;
        }
        noisyTotal0 += t1Size;
    } else {
        noisyTotal0 = domainSize;
    }
    if (rangeSanitizer.size() == 2) {
        for (int i = 0; i < domainSize; i++) {
            auto pr = rangeSanitizer[1][0].read(i);
            t2Size += pr.first;
            noisyTotal1 += pr.second;
        }
        noisyTotal1 += t2Size;
    }

    int U = (2 / 0.3 * log(1/delta));
    int tmpPart = ceil((noisyTotal0+noisyTotal1) / U * height * constant);
    printf("noisyTotal0 %d noisyTotal1 %d tmpPart %d constant %d\n", noisyTotal0, noisyTotal1, tmpPart, (int)constant);
    int avgPartSize = tableID == 0 ? ceil(noisyTotal0 / (float)tmpPart) : ceil(noisyTotal1 / (float)tmpPart);
    printf("height %d, U %d, avgPartSize %d\n", height, U, avgPartSize);

    Sanitizer* sanitizer = tableID == 0 ? &rangeSanitizer[0] : &rangeSanitizer[1];
    int start = 0, end;
    int realPartSize = 0, noisyPartSize = 0;
    int accuReal = 0, accuPureNoise = 0;
    for (int i = 0; i < domainSize; i++) {
        auto pr = rangeSanitizer[tableID][0].read(i);
        realPartSize += pr.first;
        noisyPartSize += (pr.first+pr.second);
        if (noisyPartSize > avgPartSize) {
            int noise = TGeom(eps_b, delta_b, 1);
            partitionSizes.push_back({realPartSize, realPartSize+noise});
            accumulateSizes[tableID].push_back({accuReal, accuPureNoise});
            accuReal += realPartSize;
            accuPureNoise += noise;
            boundaries.push_back(i);
            realPartSize = 0;
            noisyPartSize = 0;
        }   
        if (i == domainSize - 1) {
            if (boundaries.back() < domainSize - 1) {
                boundaries.push_back(domainSize-1);
                int noise = TGeom(eps_b, delta_b, 1);
                partitionSizes.push_back({realPartSize, realPartSize+noise});
                accumulateSizes[tableID].push_back({accuReal, accuPureNoise});
                accuReal += realPartSize;
                accuPureNoise += noise;
            }
        } 
    }
    if (tableID == 0) {
        numPart1 = boundaries.size();
        start = 0;
        end = numPart1;
    } else {
        numPart2 = boundaries.size() - numPart1;
        start = numPart1;
        end = boundaries.size();
    }
    int prev = 0;
    int keyStart, keyEnd;
    
    for (int i = start; i < end; i++) {
        keyStart = keyMin + prev; // inclusive
        keyEnd = keyMin + boundaries[i]; //inclusive
        Bucket *bucket = new Bucket(i, tableID, keyStart, keyEnd, partitionSizes[i].second);
        bucketNoisyMax = max(bucketNoisyMax, partitionSizes[i].second);
        buckets.push_back(bucket);
        prev = boundaries[i] + 1;
    }
    printf("pds size %d\n", accuReal+accuPureNoise);
}

// PF-join
void HTree::constructPBuckets(int tableID, TraceMem<pair<int,tbytes>> *data) {
    t1data = data;
    //* not expensive
    if (t1data->size < domainSize) {
        t1data->resize(domainSize);
        tbytes dummy;
        char dummyChar[textSizeVec[tableID]]; 
        int rid_pos = textSizeVec[tableID] - sizeof(int)*2;
        int mark = INT_MAX;
        memcpy(dummyChar, &mark, sizeof(int));
        memcpy(dummyChar+rid_pos, &mark, sizeof(int));
        for (int i = 0; i < textSizeVec[tableID]; i++) {
            dummy[i] = dummyChar[i];
        }
        for (int i = t1Size; i < domainSize; i++) {
            t1data->write(i, {-1, dummy});
        }
        expand(t1data);
    }
}

void HTree::moveData(int tableID, query_type op, TraceMem<pair<int,tbytes>> *data) {
    //* one-time
    if (uni) {
        if (tableID == 0) {
            t1data = data;
        } else {
            t2data = data;
        }
    }
    vector<pair<int, metaRecord>> noiseArray;
    int noiseSum = 0;
    int start, end;
    if (tableID == 0) {
        start = 0; end = numPart1;
    } else {
        start = numPart1; end = partitionSizes.size();
    }
    metaRecord filler; // filler has rid INI_MAX
    filler.values.resize(1);
    for (int i = start; i < end; i++) {
        filler.rid = INT_MAX;
        // instantiate filler
        filler.values[0] = buckets[i]->keyMin;
        noiseArray.push_back({noiseSum, filler});
        if (partitionSizes[i].second - partitionSizes[i].first < 0) {
            printf("possible error!!\n");
        }
        noiseSum += (partitionSizes[i].second - partitionSizes[i].first);
    }
    metaRecord placeholder;
    placeholder.values.resize(1);
    noiseArray.resize(noiseSum, {-1, placeholder});
    expand(noiseArray);

    //* independent
    if (!uni) {
        if (tableID == 0) {
            t1data = new TraceMem<pair<int,tbytes>>(noiseSum+t1Size);
            data = t1data;
        } else {
            t2data = new TraceMem<pair<int,tbytes>>(noiseSum+t2Size);
            data = t2data;
        }
    } else {
        //* union
        data->resize(data->size+noiseSum);
    }
    
    int rdataSize = tableID == 0 ? t1Size : t2Size;
    int mark = INT_MAX;
    int rid_pos = textSizeVec[tableID] - sizeof(int)*2;
    char *dummyRecord = new char[textSizeVec[tableID]];    
    tbytes record;
    // int part0Dummy = 0;
    for (int i = 0; i < noiseSum; i++) {
        auto dummy_iter = dummyRecord;
        for (auto &&v : noiseArray[i].second.values) {
            memcpy(dummy_iter, &v, sizeof(int));
            dummy_iter += sizeof(int);
        }
        if (op == Join) { //! why this is only for join op
            memcpy(dummyRecord+rid_pos, &mark, sizeof(int));
        }
        for (int j = 0; j < textSizeVec[tableID]; j++) {
            record[j] = dummyRecord[j];
        }
        //* 
        if (uni) {
            data->write(rdataSize+i, {-1, record});
        } else {
            data->write(i, {noiseArray[i].second.values[0], record}); //use key for sorting
        }
    }
    //* union
    if (uni) {
        accumulateSizes[tableID].resize(rdataSize);
        expand(accumulateSizes[tableID]);
        for (int i = 0; i < rdataSize; i++) {
            int r_idx = i + accumulateSizes[tableID][i].second;
            // printf("%d ", r_idx);
            auto pr = data->read(i);
            pr.first = r_idx;
            data->write(i, pr);
        }
        // printf("\n");
        expand(data, false);
    } else {
        //* independent 
        readDataBlockPair_Wrapper(tableID, tableID, 0, 0, data, noiseSum, rdataSize);
        ObliSortingNet<pair<int,tbytes>> net = ObliSortingNet<pair<int,tbytes>>(data, [](pair<int,tbytes> &r1, pair<int,tbytes> &r2) {
            return r1.first <= r2.first;
        });
        net.obliSort(data->size, true);
    }
    delete_table_OCALL(tableID);
    int binStart = 0;
    for (int i = start; i < end; i++) {
        auto curBucket = buckets[i];
        curBucket->populateData();
        for (int j = 0; j < curBucket->numData; j++) {     
            auto record = data->read(binStart+j);
            curBucket->write(j, record.second);
        }
        binStart += curBucket->numData;
        writeDataBlock_Wrapper(tableID, i, 0, curBucket->data, curBucket->numData);
    }
    data->freeSpace();
    delete []dummyRecord;
}


pair<int, int> HTree::bucketCrossProduct(int writeID, int qs, int qe, bool jattr) {
    // assume all join records fit in enclave
    numRealMatches = 0; 
    int i = 0, j = numPart1;
    pair<int, int> keyRange1, keyRange2;
    int xprodsize = 0;
    if (jattr) {
        printf("select on join attr ");
        printf("qs %d qe %d\n", qs, qe);
    } else {
        printf("select on non-join attr ");
        printf("qs %d qe %d\n", qs, qe);
    }
    int tmpbucketNoisyMax = 0;
    unordered_map<int, pair<int,int>> qualifiedCounts;
    int prevCumRealMatches = 0;
    int bucket_pair = 0;
    while (i < numPart1 && j < numPart1+numPart2) {
        keyRange1.first = buckets[i]->keyMin;
        keyRange1.second = buckets[i]->keyMax;
        keyRange2.first = buckets[j]->keyMin;
        keyRange2.second = buckets[j]->keyMax;
        if (keyRange1.second < keyRange2.first || (jattr && (keyRange1.second < qs || keyRange1.first > qe))) {
            i++;
        } else if (keyRange1.first > keyRange2.second || (jattr && (keyRange2.second < qs || keyRange2.first > qe))) {
            // assume that [qs, qe] is applied on table 2
            j++;
        } else {
            /* if (!jattr && qualifiedCounts.find(j) == qualifiedCounts.end()) {
                // assume that [qs, qe] is applied on non-join attr of table 2 
                int count = 0;
                selected_count = new uint32_t[buckets[j]->numData+1];
                selected_count[0] = 0;
                for (int r = 0; r < buckets[j]->numData; r++) {
                    selected_count[selected_count_idx] = count;
                    selected_count_idx++;
                    auto raw = buckets[j]->data->read(r);
                    bytes r2Bytes;
                    for (int k = sensitive_dim*sizeof(int); k < (sensitive_dim+1)*sizeof(int); k++) {
                        r2Bytes.push_back(raw[k]);
                    }
                    vector<int> r2Val = deconstructIntegers(1, r2Bytes, sizeof(int));
                    if (r2Val[0] >= qs && r2Val[0] <= qe) {
                        count++;
                    } 
                }
                int noisyCount = count + TGeom(eps_b, delta_b, 1);
                qualifiedCounts.insert({j, {count,noisyCount}});
                printf("qualified %d noisyCount %d bucketSize %d\n", count, noisyCount, buckets[j]->numData);
                if (noisyCount < buckets[j]->numData) {
                    ORCompact(buckets[j]->numData, buckets[j]->data);
                    buckets[j]->numData = noisyCount;
                    partitionSizes[j].second = noisyCount;
                }
                free(selected_count);
                selected_count_idx = 1;
            }  */

            tmpbucketNoisyMax = max(tmpbucketNoisyMax, partitionSizes[i].second);
            tmpbucketNoisyMax = max(tmpbucketNoisyMax, partitionSizes[j].second);
            xprodsize += (partitionSizes[i].second * partitionSizes[j].second);
            bucket_pair++;

            if (keyRange2.second > keyRange1.second) {
                i++;
            } else if (keyRange1.second > keyRange2.second) {
                j++;
            } else {
                i++;
                j++;
            }
        }
    }
    printf("xprodsize %d bucket pair %d\n", xprodsize, bucket_pair);
    printf("bucketNoisyMax %d tmpbucketNoisyMax %d\n", bucketNoisyMax, tmpbucketNoisyMax);
    bucketNoisyMax = tmpbucketNoisyMax;
    joinOutput = new TraceMem<cbytes>(xprodsize);
    selected_count = new uint32_t[xprodsize+1];
    selected_count[0] = 0;
    i = 0; j = numPart1;
    while (i < numPart1 && j < numPart1+numPart2) {
        keyRange1.first = buckets[i]->keyMin;
        keyRange1.second = buckets[i]->keyMax;
        keyRange2.first = buckets[j]->keyMin;
        keyRange2.second = buckets[j]->keyMax;
        if (keyRange1.second < keyRange2.first || (jattr && (keyRange1.second < qs || keyRange1.first > qe))) {
            i++;
        } else if (keyRange1.first > keyRange2.second || (jattr && (keyRange2.second < qs || keyRange2.first > qe))) {
            j++;
        } else {
            numRealMatches = crossProduct(buckets[i]->data, buckets[j]->data, buckets[i]->tableid, buckets[j]->tableid, buckets[i]->numData, buckets[j]->numData, writeID, false, qs, qe, jattr);        
            if (keyRange2.second > keyRange1.second) {
                i++;
            } else if (keyRange1.second > keyRange2.second) {
                j++;
            } else {
                i++;
                j++;
            }
        }
    }
    printf("correctness check: real matches %d\n", numRealMatches);
    return {numRealMatches, 0};
}

pair<int, int> HTree::pfJoin(int tableID1, int tableID2, int writeID) {
    int t1offset = 0;
    int binPairSize = 0;
    int t1_rid_pos = textSizeVec[tableID1] - sizeof(int)*2;
    int t2_rid_pos = textSizeVec[tableID2] - sizeof(int)*2;
    int textSize = t1_rid_pos + t2_rid_pos + sizeof(int);
    char *plaintext = new char[textSize];
    char *encryptedMatch = new char[EDATA_BLOCKSIZE];
    char *res = (char*)malloc(min(bucketNoisyMax, OCALL_MAXSIZE) * EDATA_BLOCKSIZE);
    joinOutput = new TraceMem<cbytes>(0);
    TraceMem<pair<int, tbytes>> binpair(0);
    int outputSize = 0;
    for (int i = 0; i < numPart2; i++) {
        binPairSize = buckets[i]->subDomainSize + buckets[i]->numData;
        joinOutput->resize(binPairSize);
        selected_count = new uint32_t[binPairSize+1];
        selected_count[0] = 0;
        binpair.resize(binPairSize);
        for (int j = t1offset; j < t1offset+buckets[i]->subDomainSize; j++) {
            binpair.write(j-t1offset, {0, t1data->read(j).second});
        }
        t1offset += buckets[i]->subDomainSize;
        for (int j = 0; j < buckets[i]->numData; j++) {
            binpair.write(j+buckets[i]->subDomainSize, {1, buckets[i]->read(j)});
        }
        numRealMatches = sortMergeJoin(&binpair);
        // enclaveCompact(binPairSize-buckets[i]->subDomainSize);
        ORCompact(binPairSize-buckets[i]->subDomainSize);
        int counter = 0;
        auto riter = res;
        for (int j = 0; j < binPairSize-buckets[i]->subDomainSize; j++) {
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
        outputSize += buckets[i]->numData;
        delete []selected_count;
    }
    binpair.freeSpace();
    free(res);
    delete []plaintext;
    delete []encryptedMatch;
    printf("pfJoin done\n");
    printf("correctness check: real matches %d\n", numRealMatches);
    printf("noisy output size %d\n", outputSize);
    return {numRealMatches, outputSize};
}

void HTree::prepareBinIdxData(vector<pair<number, bytes>> &idxBinStart, vector<pair<number, bytes>> &idxBinEnd, int idx_dim) {
    for (auto &&ptr : buckets) {
        int binStart = ptr->keyMin;
        int binEnd = ptr->keyMax;
        bytes id = bytesFromNumber(ptr->id);
        idxBinStart.push_back({binStart, id});
        idxBinEnd.push_back({binEnd, id}); //inclusive
    }
}


Bucket::Bucket(int id, int tableid, int keyMin, int keyMax, int numData) : id(id), tableid(tableid),
                                                        keyMin(keyMin), keyMax(keyMax), numData(numData){
    subDomainSize = keyMax - keyMin + 1;
}

void Bucket::populateData() {
    data = new TraceMem<tbytes>(numData);
}

void Bucket::write(int i, tbytes &record) {
    data->write(i, record);
}

tbytes Bucket::read(int i) {
    return data->read(i);
}