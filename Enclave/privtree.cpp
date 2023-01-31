#include "privtree.hpp"
#include <bitset>
TraceMem<tbytes> *tWorkSpace[2];
TraceMem<cbytes> *joinOutput;
uint32_t *selected_count;
int selected_count_idx = 1;
int realMatches = 0;


TreeNode::TreeNode(int id, int depth, vector<pair<int, int>> domain, int count1, int count2):id(id), depth(depth), domain(domain) {
    count[0] = count1;
    count[1] = count2;
};
TreeNode::TreeNode(){};

int TreeNode::split(int start_id, int beta) { // beta is max number of children
    int node_id = start_id;
    int dim = log2(beta);
    // vector<int> boundaries;
    int nSplit = 0;
    vector<int> dimSplit;
    for (int i = 0; i < dim; i++) {
        if (domain[i].second - domain[i].first >= granularity) {
            int mid = (domain[i].first + domain[i].second)/2;
            boundaries.push_back(mid);
            dimSplit.push_back(i);
            nSplit++;
        } else {
            boundaries.push_back(INT_MIN);
        }
    }
    int numChildren = pow(2, nSplit);
    if (nSplit == 0) {
        return 0;
    }
    children = new TreeNode[numChildren];
    // printf("parent domain (%d %d) (%d %d)\n", domain[0].first, domain[0].second, domain[1].first, domain[1].second);
    for (int i = 0; i < numChildren; i++) {
        children[i].id = node_id;
        node_id++;
        children[i].depth = depth+1;
        //compute domain
        bitset<10> binary(i); // number of sensitive attributes upper bounded by 1024
        int nxtSplitIdx = 0;
        for (int j = 0; j < dim; j++) {
            // if (nxtSplitIdx >= dimSplit.size()) {
            //     break;
            // }
            pair<int, int> drange;
            if (nxtSplitIdx < (int)dimSplit.size() && j == dimSplit[nxtSplitIdx]) {
                int d = nSplit-1-nxtSplitIdx;
                if (binary[d] == 0) {
                    drange = {domain[j].first, boundaries[j]};
                } else {
                    drange = {boundaries[j]+1, domain[j].second};
                }
                nxtSplitIdx++;
            } else {
                drange = {domain[j].first, domain[j].second}; 
                // printf("privtree branch test warning!!\n");
            }
            children[i].domain.push_back(drange);
            // printf("(%d %d) ", drange.first, drange.second);
        }
        // children[i].count = 0;
    }
    // printf("\n");
    return numChildren;
} 

bool TreeNode::canSplit() {
    bool split = false;
    for (auto p : domain) {
        if (p.second - p.first >= granularity) {
            split = true;
        }
    }
    return split;
}

PrivTree::PrivTree(vector<metaRecord> *t1data, vector<metaRecord> *t2data, vector<pair<int, int>> initDomain, double eps_d,
                    double eps_r, double delta, int theta, int beta): t1data(t1data), t2data(t2data), eps_d(eps_d), eps_r(eps_r),
                    delta(delta), theta(theta), beta(beta), initDomain(initDomain) {
    dim = initDomain.size();
    printf("dim %d\n", dim);
    printf("(%d, %d)\n", initDomain[0].first, initDomain[0].second);
    lambda = (2*beta-1.0)/((beta-1.0)*eps_d);
    gamma = lambda*log(beta);
    t1data_size = (t1data == nullptr) ? 0 : t1data->size();
    t2data_size = (t2data == nullptr) ? 0 : t2data->size();
    root = new TreeNode(node_id, 0, initDomain, t1data_size, t2data_size);
    node_id++;
    nodelist.push_back(root);
    tWorkSpace[0] = new TraceMem<tbytes>(0);
    tWorkSpace[1] = new TraceMem<tbytes>(0);
    // printf("add %d %d\n", workSpace[0], workSpace[1]);
}

PrivTree::PrivTree(){};

void PrivTree::createTree() {
    int level = 0;
    while(!nodelist.empty()) {
        decompose(level);
        // printf("level %d done\n", level);
        level++;
        // get count level by level linear scan (no communication with data owner)
        countInDomains();
    }
    // sort leaves in lexicographical order
    sort(leaves.begin(), leaves.end(), [](TreeNode *leaf1, TreeNode *leaf2){
        for (int i = 0; i < leaf1->domain.size(); i++) {
            if (leaf1->domain[i].first == leaf2->domain[i].first) {
                if (leaf1->domain[i].second == leaf2->domain[i].second) {
                    continue;
                } else {
                    return leaf1->domain[i].second < leaf2->domain[i].second;
                }
                
                // if (leaf1->domain[i].second == leaf2->domain[i].second) {
                //     continue;
                // } else {
                //     return leaf1->domain[i].second < leaf2->domain[i].second;
                // }
            }
            return leaf1->domain[i].first < leaf2->domain[i].first;
        }
        return false;
    });
    /* for (int i = 0; i < leaves.size(); i++) {
        if (100 < leaves[i]->domain[1].first || 50 > leaves[i]->domain[1].second) {
            continue;
        } else {
            printf("(%d %d) (%d %d)\n", leaves[i]->domain[0].first, leaves[i]->domain[0].second, leaves[i]->domain[1].first, leaves[i]->domain[1].second);
        }
    } */
}

void PrivTree::decompose(int level) {
    while (!nodelist.empty()) {
        TreeNode *node = nodelist.front();
        if (node->depth > level) {
            return;
        }
        nodelist.pop_front();
        float bias_count = max(node->count[0]+node->count[1] - node->depth*gamma, theta-gamma);
        float noisy_count = bias_count + Lap(1/lambda);
        if (noisy_count > theta && node->canSplit()) {
            // stop condition for small enough domain
            int numChildren = node->split(node_id, beta);
            node_id += numChildren;
            for (int i = 0; i < numChildren; i++) {
                nodelist.push_back(&node->children[i]);
            }
        } else {
            int noise1, noise2;
            noise1 = TGeom(eps_r, delta, 1);
            if (!(t2data == nullptr)) {
                noise2 = TGeom(eps_r, delta, 1);
            } else {
                noise2 = 0;
            }
            node->noise[0] = noise1;
            // node->noise[0] = 0;
            node->noise[1] = noise2;
            binNoisyMax = max(binNoisyMax, node->count[0]+noise1);
            binNoisyMax = max(binNoisyMax, node->count[1]+noise2);
            leaves.push_back(node);
        }
    }
}

void PrivTree::countInDomains() {
    for (auto iter=nodelist.begin(); iter != nodelist.end(); iter++) {
        if (t1data != nullptr) {
            for (uint i = 0; i < t1data->size(); i++) {
                if (inDomain(t1data->at(i), *iter)) {
                    (*iter)->count[0]++;
                }
            }
        }
        if (t2data != nullptr) {
            for (uint i = 0; i < t2data->size(); i++) {
                if (inDomain(t2data->at(i), *iter)) {
                    (*iter)->count[1]++;
                }
            }   
        }
    }
}

bool PrivTree::inDomain(metaRecord &record, TreeNode *node) {
    for (int i = 0; i < dim; i++) {
        if (record.values[i] < node->domain[i].first || record.values[i] > node->domain[i].second) {
            return false;
        }
    }
    return true;
}

void PrivTree::augmentInput(int tableID, vector<metaRecord> *data, int table, bool simulation, query_type op, bool sorting) {
    vector<pair<int, metaRecord>> noiseArray;
    int noiseSum = 0;
    for (auto &&ptr : leaves) {
        metaRecord filler; // filler has rid INI_MAX
        filler.rid = INT_MAX;
        // instantiate filler
        for (uint i = 0; i < ptr->domain.size(); i++) {
            filler.values.push_back(ptr->domain[i].first);
        }
        noiseArray.push_back({noiseSum, filler});
        noiseSum += ptr->noise[table-1];
    }
    bool inEnclave;
    if (table == 1) {
        // if (op == Select) {
        //     t1_inEnclave = false;
        //     inEnclave = false;
        // } else {
            t1_inEnclave = noiseSum+data->size() <= IO_BLOCKSIZE ? true : false;
            inEnclave = t1_inEnclave;
        // }
    } else {
        t2_inEnclave = noiseSum+data->size() <= IO_BLOCKSIZE ? true : false;
        inEnclave = t2_inEnclave;
    }
    TraceMem<pair<int,tbytes>> *rbin;
    if (inEnclave) {
        if (table == 1) {
            global_rbin1 = new TraceMem<pair<int,tbytes>>(noiseSum+data->size());
            rbin = global_rbin1;
        } else {
            global_rbin2 = new TraceMem<pair<int,tbytes>>(noiseSum+data->size());
            rbin = global_rbin2;
        }
        printf("Size of PDS %d\n", noiseSum+data->size());
        printf("number of bins: %d\n", leaves.size());
    }
    // expand noiseArray
    metaRecord placeholder;
    noiseArray.resize(noiseSum, {-1, placeholder});
    expand(noiseArray); 
    int mark = INT_MAX;
    int rid_pos = textSizeVec[tableID] - sizeof(int)*2;
    // int key1Count = 0, key2Count = 0;
    if (!simulation && inEnclave) {
        //TODO
        char *dummyRecord = new char[textSizeVec[tableID]];
        for (int i = 0; i < noiseSum; i++) {
            tbytes record;
            auto dummy_iter = dummyRecord;
            // if (noiseArray[i].second.values[0] == 1) {
            //     key1Count++;
            // }
            // if (noiseArray[i].second.values[0] == 2) {
            //     key2Count++;
            // }
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
            rbin->write(i, {noiseArray[i].second.values[0], record});
        }
        // printf("key 1 has noise %d\n", key1Count);
        // printf("key 2 has noise %d\n", key2Count);
        readDataBlockPair_Wrapper(tableID, tableID, 0, 0, rbin, noiseSum, data->size());
        //* only need for iterative compaction binning
        if (!sorting){
            if (table == 1 && root->rbin1 == nullptr) {
                root->rbin1 = new TraceMem<tbytes>(rbin->size);
            }
            if (table == 2 && root->rbin2 == nullptr) {
                root->rbin2 = new TraceMem<tbytes>(rbin->size);
            }
            for (int i = 0; i < rbin->size; i++) {
                if (table == 1) {
                    root->rbin1->write(i, rbin->read(i).second);
                } else {
                    root->rbin2->write(i, rbin->read(i).second);
                }
            }
        }
    }
    // int preKey = 1;
    // int keyCount = 0;
    // for (int i = 0; i < rbin->size; i++) {
    //     auto record = rbin->read(i);
    //     uchar tmpInt[sizeof(int)];
    //     for (int k = 0; k < sizeof(int); k++) {
    //         tmpInt[k] = record[k];
    //     }
    //     vector<int> key = deconstructNumbers<int>(tmpInt, sizeof(int));
    //     if (key[0] == preKey) {
    //         keyCount++;
    //     } 
    // }
    // printf("key %d count %d\n", preKey, keyCount);
    
    //* expand table OCALL
    if (!simulation && !inEnclave) {
        int expandBlockSize = min(OCALL_MAXSIZE, noiseSum);
        char *noise = (char *)malloc(expandBlockSize*EDATA_BLOCKSIZE);
        // int n_attr_add1 = textSizeVec[tableID]/sizeof(int);
        char *dummyRecord = new char[textSizeVec[tableID]];
        char *encryptedDummy = new char[EDATA_BLOCKSIZE];
        int num_iter = ceil((float)noiseArray.size()/expandBlockSize);
        for (int i = 0; i < num_iter; i++) {
            auto noise_iter = noise;
            auto end = (i+1)*expandBlockSize < noiseArray.size() ? (i+1)*expandBlockSize : noiseArray.size();
            for (int j = i*expandBlockSize; j < end; j++) {
                auto dummy_iter = dummyRecord;
                for (auto &&v : noiseArray[j].second.values) {
                    memcpy(dummy_iter, &v, sizeof(int));
                    dummy_iter += sizeof(int);
                }
                if (op == Join) { //! why this is only for join op
                    memcpy(dummyRecord+rid_pos, &mark, sizeof(int));
                }
                encryptRecord(textSizeVec[tableID], dummyRecord, encryptedDummy);
                memcpy(noise_iter, encryptedDummy, EDATA_BLOCKSIZE);
                noise_iter += EDATA_BLOCKSIZE;
            }
            int n_noise = end - i*expandBlockSize;
            expand_table_OCALL(tableID, n_noise, noise, n_noise*EDATA_BLOCKSIZE);
        }
        delete []encryptedDummy;
        delete []dummyRecord;
        free(noise);
    }
    /* if (!inEnclave) {
        vector<metaRecord> *tbin;
        if (table == 1) {
            root->bin1 = new vector<metaRecord>;
            tbin = root->bin1;
        } else {
            root->bin2 = new vector<metaRecord>;
            tbin = root->bin2;
        }
        tbin->insert(tbin->end(), data->begin(), data->end());
        data->clear();
        for (auto &&dummy : noiseArray) {
            tbin->push_back(dummy.second);
        }
        printf("Size of PDS %d\n", tbin->size());
        printf("number of bins: %d\n", leaves.size());
    } */
}

void PrivTree::binning(int tableID, TreeNode *node, int table, bool simulation) {
    if (node->children == nullptr) {
        return;
    }
    queue<pair<int, vector<metaRecord>*>> bq;
    if (table == 1) {
        bq.push({node->id, node->bin1});
    } else {
        bq.push({node->id, node->bin2});
    }
    int nSplit = 0;
    int virtualBinID = node_id;
    CompactNet<tbytes> cnet(tWorkSpace[0], tWorkSpace[1]);
    for (int d = 0; d < dim; d++) {
        if (node->boundaries[d] == INT_MIN) {
            continue;
        }
        nSplit++;
        for (int i = 0; i < pow(2, nSplit-1); i++) {
            vector<metaRecord> *in = bq.front().second;
            int binID = bq.front().first;
            bq.pop();
            vector<metaRecord> *out1 = new vector<metaRecord>;
            vector<metaRecord> *out2 = new vector<metaRecord>;
            // compactMeta<metaRecord>(in, out1, out2, [&d, &node](metaRecord &e){ 
            //     return e.values[d] <= node->boundaries[d];
            // });
            if (!simulation) { //TODO: this fill is problematic I guess
                int out1_fill_size = (IO_BLOCKSIZE - (out1->size() % IO_BLOCKSIZE)) % IO_BLOCKSIZE;
                if (in->size() <= IO_BLOCKSIZE) {
                    out1_fill_size = 0;
                }
                // out1_fill_size = 1;
                cnet.compact(tableID, binID, in->size()+out1_fill_size, virtualBinID, [&d, &node](vector<int> &e) { 
                    return e[d] <= node->boundaries[d];
                });
            }
            
            bq.push({virtualBinID++, out1}); bq.push({virtualBinID++, out2});
            vector<metaRecord>().swap(*in);
            delete in;
        }
    }
    if (!simulation) {
        delete_bin_OCALL(tableID, node->id);
    }
    int numChildren = pow(2, nSplit);
    for (int i = 0; i < numChildren; i++) {
        auto virtualBinID = bq.front().first;
        auto childBin = bq.front().second;
        if (table == 1) {
            node->children[i].bin1 = childBin;
        } else {
            node->children[i].bin2 = childBin;
        }
        if (!simulation) {
            // move_bin_OCALL(tableID, virtualBinID, node->children[i].id, min, max);
            move_resize_bin_OCALL(tableID, virtualBinID, node->children[i].id, childBin->size());
        }
        bq.pop();
    }
    for (int i = 0; i < numChildren; i++) {
        if (node->children[i].children != nullptr) {
            binning(tableID, &node->children[i], table, simulation);
        }
    }
}

void PrivTree::binningInEnclave(int tableID, int pos, TreeNode *node, int table, bool sorting) {//TODO if using direct sort, will it be faster
    //* use sorting
    if (sorting) {
        TraceMem<pair<int,tbytes>> *rbin;
        if (table == 1) {
            rbin = global_rbin1;
        } else {
            rbin = global_rbin2;
        }
        ObliSortingNet<pair<int,tbytes>> net = ObliSortingNet<pair<int,tbytes>>(rbin, [](pair<int,tbytes> &r1, pair<int,tbytes> &r2) {
            uchar tmpInt[sensitive_dim * sizeof(int)];
            for (int k = 0; k < sensitive_dim * sizeof(int); k++) {
                tmpInt[k] = r1.second[k];
            }
            vector<int> r1Key = deconstructNumbers<int>(tmpInt, sensitive_dim * sizeof(int));
            for (int k = 0; k < sensitive_dim * sizeof(int); k++) {
                tmpInt[k] = r2.second[k];
            }
            vector<int> r2Key = deconstructNumbers<int>(tmpInt, sensitive_dim * sizeof(int));
            return r1Key <= r2Key;
            // return r1.first <= r2.first;
        });
        net.obliSort(rbin->size, true);

        int binStart = 0;
        for (int i = 0; i < leaves.size(); i++) {
            int binSize = leaves[i]->noise[tableID] + leaves[i]->count[tableID];
            TraceMem<tbytes> *bin;
            if (table == 1) {
                leaves[i]->rbin1 = new TraceMem<tbytes>(binSize);
                bin = leaves[i]->rbin1;
            } else {
                leaves[i]->rbin2 = new TraceMem<tbytes>(binSize);
                bin = leaves[i]->rbin2;
            }
            // printf("tableID %d domain (%d %d) (%d %d) binSize %d\n", tableID, leaves[i]->domain[0].first, leaves[i]->domain[0].second, leaves[i]->domain[1].first, leaves[i]->domain[1].second, binSize);
            for (int j = 0; j < binSize; j++) {
                auto record = rbin->read(binStart+j);
                // uchar tmpInt[sensitive_dim * sizeof(int)];
                // for (int k = 0; k < sensitive_dim * sizeof(int); k++) {
                //     tmpInt[k] = record.second[k];
                // }
                // vector<int> key = deconstructNumbers<int>(tmpInt, sensitive_dim * sizeof(int));
                // if (key[0] < leaves[i]->domain[0].first || key[0] >= leaves[i]->domain[0].second || key[1] < leaves[i]->domain[1].first || key[1] >= leaves[i]->domain[1].second) {
                //     printf("key (%d %d) ", key[0], key[1]);
                // }

                bin->write(j, record.second);
            }
            binStart += binSize;
        }
        rbin->freeSpace();
    }

    //* use iterative compaction for sensitive_dim > 1  
    if (!sorting) {
        // printf("start iterative compaction\n");
        if (node->children == nullptr) {
            return;
        }
        queue<pair<int, TraceMem<tbytes>*>> bq;
        if (table == 1) {
            bq.push({node->id, node->rbin1});
        } else {
            bq.push({node->id, node->rbin2});
        }
        int nSplit = 0;
        int virtualBinID = node_id;
        for (int d = 0; d < dim; d++) {
            if (node->boundaries[d] == INT_MIN) {
                continue;
            }
            nSplit++;
            for (int i = 0; i < pow(2, nSplit-1); i++) {
                TraceMem<tbytes> *in = bq.front().second;
                int binID = bq.front().first;
                bq.pop();
                TraceMem<tbytes> *out1 = new TraceMem<tbytes>(0);
                TraceMem<tbytes> *out2 = new TraceMem<tbytes>(0);
                // printf("before compactMeta\n");
                compactMeta<tbytes>(in, out1, out2, [&d, &node](tbytes &e){ 
                    //TODO
                    uchar tmpInt[sizeof(int)];
                    int start = d*sizeof(int);
                    for (int k = start; k < (d+1)*sizeof(int); k++) {
                        tmpInt[k-start] = e[k];
                    }
                    vector<int> key = deconstructNumbers<int>(tmpInt, sizeof(int));
                    return key[0] <= node->boundaries[d];
                });
                bq.push({virtualBinID++, out1}); bq.push({virtualBinID++, out2});
                in->freeSpace();
                // printf("out1 size %d out2 size %d\n", out1->size, out2->size);
            }
        }
        int numChildren = pow(2, nSplit);
        for (int i = 0; i < numChildren; i++) {
            auto virtualBinID = bq.front().first;
            auto childBin = bq.front().second;
            if (table == 1) {
                node->children[i].rbin1 = childBin;
            } else {
                node->children[i].rbin2 = childBin;
            }
            bq.pop();
        }
        for (int i = 0; i < numChildren; i++) {
            if (node->children[i].children != nullptr) {
                binningInEnclave(tableID, 0, &node->children[i], table, sorting);
            }
        }
    }
}

void PrivTree::collectBins(int tableID, int table) {
    uint size = leaves.size()*sizeof(int);
    char *leaves_id = (char*)malloc(size);
    char *binSizes = (char*)malloc(size);
    auto liter = leaves_id;
    auto biter = binSizes;
    for (auto &&ptr : leaves){
        memcpy(liter, &ptr->id, sizeof(int));
        liter += sizeof(int);
        int binSize = (ptr->count[table-1]+ptr->noise[table-1]);
        memcpy(biter, &binSize, sizeof(int));
        biter += sizeof(int);
    }
    collect_resize_bins_OCALL(tableID, leaves_id, size, binSizes, size);
    free(leaves_id);
    free(binSizes);
}

void PrivTree::prepareBinIdxData(vector<pair<number, bytes>> &idxBinStart, vector<pair<number, bytes>> &idxBinEnd, int idx_dim) {
    for (auto &&ptr : leaves) {
        int binStart = ptr->domain[idx_dim].first;
        int binEnd = ptr->domain[idx_dim].second;
        bytes id = bytesFromNumber(ptr->id);
        idxBinStart.push_back({binStart, id});
        idxBinEnd.push_back({binEnd, id});
    }
}


void PrivTree::checkSplitCorrectness() {
    int countSum = 0;
    int binSum = 0;
    for (auto &&ptr : leaves) {
        countSum += (ptr->count[0] + ptr->count[1]);
        for (uint i = 0; i < ptr->bin1->size(); i++) {
            if (ptr->bin1->at(i).rid != INT_MAX) {
                binSum++;
            }
        }
        if (ptr->bin2 == nullptr) {
            continue;
        }
        for (uint i = 0; i < ptr->bin2->size(); i++) {
            if (ptr->bin2->at(i).rid != INT_MAX) {
                binSum++;
            }
        }
    }
    int t1Size = t1data_size;
    int t2Size = (t2data == nullptr) ? 0 : t2data_size;
    assert(countSum == t1Size+t2Size);
    assert(binSum == countSum);
}

// select on non-join attribute, assume to be table 2
vector<TreeNode*> PrivTree::select(int tableID, int qs, int qe) {
    vector<TreeNode*> selectedBins;
    int binNoisyMaxTmp = 0;
    for (int i = 0; i < leaves.size(); i++) {
        // assume that the predicate is on the second dimension
        if (qe < leaves[i]->domain[1].first || qs > leaves[i]->domain[1].second) {
            continue;
        } else {
            selectedBins.push_back(leaves[i]);
            binNoisyMaxTmp = max(binNoisyMaxTmp, leaves[i]->rbin2->size);
            // printf("(%d %d) (%d %d)\n", leaves[i]->domain[0].first, leaves[i]->domain[0].second, leaves[i]->domain[1].first, leaves[i]->domain[1].second);
        }
    }
    binNoisyMax = binNoisyMaxTmp;
    return selectedBins;
}

pair<int, int> PrivTree::selectJoin(vector<TreeNode*> &selectedBins, int tableID1, int tableID2, int writeID, int qs, int qe) {
    int prvStart = 0;
    printf("selectjoin selectedbins size %d\n", selectedBins.size());
    int crossProductSize = 0;
    // int noisyTableSize = 0;

    for (int i = 0; i < selectedBins.size(); i++) {
        int binSize = selectedBins[i]->rbin2->size;
        int t2s = selectedBins[i]->domain[0].first;
        int t2e = selectedBins[i]->domain[0].second;
        for (int j = 0; j < leaves.size(); j++) {
            int t1s = leaves[j]->domain[0].first;
            int t1e = leaves[j]->domain[0].second;
            if (t1s > t2e) {
                break;
            }
            if (t2s > t1e) {
                continue;
            }
            crossProductSize += binSize * leaves[j]->rbin1->size;
        }
    }
    printf("cross product size %d\n", crossProductSize);
    joinOutput = new TraceMem<cbytes>(crossProductSize);
    selected_count = new uint32_t[crossProductSize+1];
    selected_count[0] = 0;
    int outputSize = 0;
    for (int i = 0; i < selectedBins.size(); i++) {
        // range of the join attr
        int t2s = selectedBins[i]->domain[0].first;
        int t2e = selectedBins[i]->domain[0].second;
        for (int j = prvStart; j < leaves.size(); j++) {
            int t1s = leaves[j]->domain[0].first;
            int t1e = leaves[j]->domain[0].second;
            if (t1s > t2e) {
                break;
            }
            if (t2s > t1e) {
                prvStart = j;
                continue;
            }
            // join
            int nreal1 = leaves[j]->rbin1->size;
            int nreal2 = selectedBins[i]->rbin2->size;
            // printf("join T2 (%d %d) T1 (%d %d) %d * %d\n", t2s, t2e, t1s, t1e, nreal2, nreal1);
            numRealMatches = crossProduct(leaves[j]->rbin1, selectedBins[i]->rbin2, tableID1, tableID2, nreal1, nreal2, writeID, false, qs, qe, 0);
            outputSize += (nreal1 * nreal2);
        }
    }
    printf("correctness check: real matches %d\n", numRealMatches);
    printf("actual cross product size %d\n", outputSize);
    joinOutput->resize(outputSize);
    uint32_t *selected_count_copy = new uint32_t[outputSize+1];
    selected_count_copy[0] = 0;
    for (int i = 1; i < outputSize+1; i++) {
        selected_count_copy[i] = selected_count[i];
    }
    delete []selected_count;
    selected_count = selected_count_copy;
    return {numRealMatches, 0};
}

pair<int,int> PrivTree::join(int tableID1, int tableID2, int writeID, int qs, int qe) {
    int readBlockSize = min(binNoisyMax, OCALL_MAXSIZE);
    char *dblock1, *dblock2;
    if (!t1_inEnclave) {
        dblock1 = (char*)malloc(readBlockSize * EDATA_BLOCKSIZE);
        printf("table 1 not in enclave; ");
    }
    if (!t2_inEnclave) {
        dblock2 = (char*)malloc(readBlockSize * EDATA_BLOCKSIZE);
        printf("table 2 not in enclave; ");
    }

    int outputSize = 0;
    // int nDummy = 0; //TODO remove nReal
    int crossProductSize = getNoisySize();
    int binPairKKSSize = getBinPairKKSSize();
    bool useOCALL = false;
    if (crossProductSize > 2*IO_BLOCKSIZE) {
        useOCALL = true;
        printf("intermediate join output written to untrusted storage %d\n", crossProductSize);
    }
    if (!useOCALL) {
        printf("intermediate join output kept in enclave, X product size %lld\n", crossProductSize);
        printf("binpair kks noisy output size %lld\n", binPairKKSSize);
        joinOutput = new TraceMem<cbytes>(crossProductSize);
        selected_count = new uint32_t[crossProductSize+1];
        selected_count[0] = 0;
    }
    for (int i = 0; i < leaves.size(); i++) {
        int readFlag1 = readBlockSize;
        int iter1 = 0;
        while (iter1 * readBlockSize < readFlag1) {
            int pos1 = iter1 * readBlockSize;
            iter1++;
            int nreal1;
            if (!t1_inEnclave) {
                readDataBlock_OCALL(&readFlag1, tableID1, leaves[i]->id, pos1, dblock1, readBlockSize * EDATA_BLOCKSIZE);
                if (readFlag1 < iter1 * readBlockSize) {
                    nreal1 = readFlag1 - (iter1-1) * readBlockSize;
                } else {
                    nreal1 = readBlockSize;
                }
            } else {
                // printf("leaf %d domain (%d %d) rbin1 size %d\n", leaves[i]->id, leaves[i]->domain[0].first, leaves[i]->domain[0].second, leaves[i]->rbin1->size);
                nreal1 = leaves[i]->rbin1->size;
            }
            tWorkSpace[0]->resize(nreal1);
            if (!t1_inEnclave) {
                parseDataBlock<tbytes>(tableID1, tWorkSpace[0], dblock1, 0, nreal1);
            } else {
                for (int j = 0; j < nreal1; j++) {
                    tWorkSpace[0]->write(j, leaves[i]->rbin1->read(pos1+j));
                }
                // leaves[i]->rbin1->freeSpace(); // reduce heap size for large inputs
            }
            int readFlag2 = readBlockSize;
            int iter2 = 0;
            while (iter2 * readBlockSize < readFlag2) {
                int pos2 = iter2 * readBlockSize;
                iter2++;
                int nreal2;
                if (!t2_inEnclave) {
                    readDataBlock_OCALL(&readFlag2, tableID2, leaves[i]->id, pos2, dblock2, readBlockSize * EDATA_BLOCKSIZE);
                    if (readFlag2 < iter2 * readBlockSize) {
                        nreal2 = readFlag2 - (iter2-1) * readBlockSize;
                    } else {
                        nreal2 = readBlockSize;
                    }
                } else {
                    nreal2 = leaves[i]->rbin2->size;
                }
                tWorkSpace[1]->resize(nreal2);
                if (!t2_inEnclave) {
                    parseDataBlock<tbytes>(tableID2, tWorkSpace[1], dblock2, 0, nreal2);
                } else {
                    for (int j = 0; j < nreal2; j++) {
                        tWorkSpace[1]->write(j, leaves[i]->rbin2->read(pos2+j));
                    }
                    // leaves[i]->rbin2->freeSpace(); // reduce heap size for large inputs
                }
                numRealMatches = crossProduct(tWorkSpace[0], tWorkSpace[1], tableID1, tableID2, nreal1, nreal2, writeID, useOCALL, qs, qe, 0);
                outputSize += (nreal1 * nreal2);
            }
        }
    }
    if (!t1_inEnclave) {
        free(dblock1);
    }
    if (!t2_inEnclave) {
        free(dblock2);
    }
    printf("correctness check: real matches %d\n", numRealMatches);
    printf("noisy output size %d\n", outputSize);
    
    if (!useOCALL) {
        joinOutput->resize(joinOutput->count); //TODO remove?
        printf("joinOutput size %d count %d crossProduct %d\n", joinOutput->size, joinOutput->count, crossProductSize);
    }
    int inEnclaveSize = useOCALL ? 0 : joinOutput->count;
    return {numRealMatches, outputSize-inEnclaveSize};
}

//TODO initialize selected_count
pair<int, int> PrivTree::pfJoin(int tableID1, int tableID2, int writeID) {
    int readBlockSize = min(binNoisyMax, OCALL_MAXSIZE);
    char *dblock = (char*)malloc(readBlockSize * EDATA_BLOCKSIZE);
    // int numRealMatches = 0;
    int outputSize = 0;
    TraceMem<pair<int, tbytes>> binpair(0);
    joinOutput = new TraceMem<cbytes>(0);
    //! assume every bin pair can fit in allocated heap/EPC

    int t1_rid_pos = textSizeVec[tableID1] - sizeof(int)*2;
    int t2_rid_pos = textSizeVec[tableID2] - sizeof(int)*2;
    int textSize = t1_rid_pos + t2_rid_pos + sizeof(int);
    char *plaintext = new char[textSize];
    char *encryptedMatch = new char[EDATA_BLOCKSIZE];
    char *res = (char*)malloc(min(binNoisyMax, OCALL_MAXSIZE) * EDATA_BLOCKSIZE);
    for (int i = 0; i < leaves.size(); i++) {
        int offset = leaves[i]->count[0] + leaves[i]->noise[0];
        int binpairSize = offset + leaves[i]->count[1] + leaves[i]->noise[1];
        joinOutput->resize(binpairSize);
        selected_count = new uint32_t[binpairSize+1];
        selected_count[0] = 0;
        tWorkSpace[0]->resize(binpairSize);
        binpair.resize(binpairSize);
        int readFlag1 = readBlockSize;
        int iter1 = 0;
        if (!t1_inEnclave) {
            while (iter1 * readBlockSize < readFlag1) {
                int pos1 = iter1 * readBlockSize;
                readDataBlock_OCALL(&readFlag1, tableID1, leaves[i]->id, pos1, dblock, readBlockSize * EDATA_BLOCKSIZE);
                iter1++;
                int nreal1;
                if (readFlag1 < iter1 * readBlockSize) {
                    nreal1 = readFlag1 - (iter1-1) * readBlockSize;
                } else {
                    nreal1 = readBlockSize;
                }
                parseDataBlock<tbytes>(tableID1, tWorkSpace[0], dblock, pos1, nreal1);
            }
            for (int j = 0; j < offset; j++) {
                binpair.write(j, {0, tWorkSpace[0]->read(j)});
            }
        } else {
            // printf("offset %d binsize %d\n", offset, leaves[i]->rbin1->size);
            for (int j = 0; j < offset; j++) {
                //TODO
                binpair.write(j, {0, leaves[i]->rbin1->read(j)});
            }
        }
        int readFlag2 = readBlockSize;
        int iter2 = 0;
        if (!t2_inEnclave) {
            while (iter2 * readBlockSize < readFlag2) {
                int pos2 = iter2 * readBlockSize;
                readDataBlock_OCALL(&readFlag2, tableID2, leaves[i]->id, pos2, dblock, readBlockSize * EDATA_BLOCKSIZE);
                iter2++;
                int nreal2;
                if (readFlag2 < iter2 * readBlockSize) {
                    nreal2 = readFlag2 - (iter2-1) * readBlockSize;
                } else {
                    nreal2 = readBlockSize;
                }
                parseDataBlock<tbytes>(tableID2, tWorkSpace[0], dblock, offset+pos2, nreal2);
            }
            for (int j = offset; j < binpairSize; j++) {
                binpair.write(j, {1, tWorkSpace[0]->read(j)});
            }
        } else {
            // printf("offset %d binsize %d\n", binpairSize-offset, leaves[i]->rbin2->size);
            for (int j = 0; j < binpairSize-offset; j++) {
                //TODO
                binpair.write(offset+j, {1, leaves[i]->rbin2->read(j)});
            }
        }
        numRealMatches = sortMergeJoin(&binpair);
        // enclaveCompact(binpairSize-offset);
        ORCompact(binpairSize-offset);
        int counter = 0;
        auto riter = res;
        for (int j = 0; j < binpairSize-offset; j++) {
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
        outputSize += leaves[i]->count[1] + leaves[i]->noise[1]; //! assume that foreign key table is the 2nd input table
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

int PrivTree::getNoisySize() {
    int outputLen = 0;
    for (auto ptr : leaves) {
        outputLen += (ptr->count[0]+ptr->noise[0]) * (ptr->count[1]+ptr->noise[1]);
    }
    return outputLen;
}

int PrivTree::getBinPairKKSSize() {
    int outputLen = 0;
    for (auto ptr : leaves) {
        int s = max(ptr->count[0]+ptr->noise[0], ptr->count[1]+ptr->noise[1]);
        outputLen += (ptr->count[0]+ptr->noise[0]) * (ptr->count[1]+ptr->noise[1]) + TGeom(eps_r, delta, s);
    }
    return outputLen;
}

int PrivTree::checkJoinOutput(int join_idx1, int join_idx2) {
    int outputLen = 0;
    int joinSize = 0;
    for (auto ptr : leaves) {
        for (uint i = 0; i < ptr->bin1->size(); i++) {
            if (ptr->bin1->at(i).rid == INT_MAX) {
                // printf("t1 join key %d\n", ptr->bin1->at(i).values[join_idx1]);
                continue;
            }
            for (uint j = 0; j < ptr->bin2->size(); j++) {
                if (ptr->bin2->at(j).rid == INT_MAX) {
                    continue;
                }
                if (ptr->bin1->at(i).values[join_idx1] == ptr->bin2->at(j).values[join_idx2]) {
                    joinSize++;
                }
            }
        }
        outputLen += (ptr->count[0]+ptr->noise[0]) * (ptr->count[1]+ptr->noise[1]);
        // printf("bin1 size %d, bin2 size %d\n", (ptr->count[0]+ptr->noise[0]), (ptr->count[1]+ptr->noise[1]));
    }
    printf("join result size: %d\n", joinSize);
    printf("number of bins: %d\n", leaves.size());
    printf("privtree join output len: %d\n", outputLen);
    return joinSize;
}

void PrivTree::checkSelction(int start, int end, int query_dim, vector<int> *selectedBins) {
    int outputLen = 0;
    int selectSize = 0;
    int minNumBins = 0;
    uint binIter = 0;
    for (auto ptr : leaves) {
        bool selected = false;
        for (uint i = 0; i < ptr->bin1->size(); i++) {
            if (ptr->bin1->at(i).rid == INT_MAX || ptr->bin1->at(i).values[query_dim] < start || ptr->bin1->at(i).values[query_dim] > end) {
                continue;
            } else {
                selectSize++;
                selected = true;
            }
        }
        if (selected) {
            minNumBins++;
            while (binIter < selectedBins->size() && selectedBins->at(binIter) < ptr->id) {
                binIter++;
            }
            assert(selectedBins->at(binIter) == ptr->id);
        }
    }
    binIter = 0;
    while (binIter < selectedBins->size()) {
        outputLen += leaves[selectedBins->at(binIter)]->bin1->size();
        binIter++;
    }
    /* cout << "selection size: " << selectSize << endl;
    cout << "min num of selected bins: " << minNumBins << endl;
    cout << "selection output len: " << outputLen << endl;
    cout << "actual num of selected bins: " << selectedBins.size() << endl; */
    printf("selection size: %d\n", selectSize);
    printf("min num of selected bins: %d\n", minNumBins);
    printf("selection output len: %d\n", outputLen);
    printf("actual num of selected bins: %d\n", selectedBins->size());
}

/* void PrivTree::cleanHelper() {
    keyHelperList.clear();
} */

