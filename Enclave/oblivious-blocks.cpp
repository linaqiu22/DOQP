#include "oblivious-blocks.hpp"
// #include "do-algorithms.hpp"

int prev_pow_two(int x) {
    int y = 1;
    while (y < x) y <<= 1;
    return y >>= 1;
}

/* void compactRecord(int tableID, int dim, int pos, TraceMem<tbytes> *in, TraceMem<tbytes> *&out1, TraceMem<tbytes> *&out2, vector<int> &keyHelperList, function<bool(int&)> func) {
    int inSize = in->size;
    vector<int> distArray(inSize, 0);
    bool buildList = false;
    if (keyHelperList.size() == 0) {
        buildList = true;
    }
    int counter = 0;
    uchar tmpInt[sizeof(int)];
    for (int i = 0; i < inSize; i++) {
        if (buildList) {
            tbytes record = in->read(i);
            int k = 0;
            for (int j = dim*sizeof(int); j < (dim+1)*sizeof(int); j++) {
                tmpInt[k] = record[j];
                k++;
            }
            vector<int> key((int*)tmpInt, (int*)tmpInt+1);
            keyHelperList.push_back(key[0]);
        }
        int distance = 0;
        if (func(keyHelperList[pos+i])) {
            distance = i - counter;
            counter++;
        } else {
            distance = 0;
        }
        distArray[i] = distance;
    }
    for (int i = 0; i < log2(inSize); i++) {
        for (int j = pow(2, i); j < inSize; j++) {
            auto record = in->read(j);
            auto r_key = keyHelperList[pos+j];
            int distance = distArray[j];
            int offset = (distance % (int)pow(2, i+1)); 
            int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            //* swap
            auto toSwap = in->read(dest);
            auto s_key = keyHelperList[pos+dest];
            if (offset > 0) {
                in->write(j, toSwap);
                in->write(dest, record);
                distance -= offset;
                distArray[j] = 0;
                distArray[dest] = distance;
                keyHelperList[pos+j] = s_key;
                keyHelperList[pos+dest] = r_key;
            } else {
                in->write(j, record);
                in->write(dest, toSwap);
                keyHelperList[pos+j] = r_key;
                keyHelperList[pos+dest] = s_key;
            }
        }
    }
    //construct out1 and out2
    if (out1 != nullptr && out2 != nullptr) {
        // out1->insert(counter, *in, 0);
        out2->insert(inSize-counter, *in, counter);
        in->resize(counter);
        out1 = in;
    }
    vector<int>().swap(distArray);
} */

/* void expand(vector<pair<int, metaRecord>> &data) {
    int m = data.size(); 
    for (int j = prev_pow_two(m); j >= 1; j /= 2) {
        for (int i = m - j - 1; i >= 0; i--) {
            auto e = data[i];
            int dest_i = e.first;
            auto e1 = data[i + j];
            if (dest_i >= i + j) {
                // assert(ind_func(e1) == -1);
                data[i] = e1;
                data[i+j] = e;
            } else {
                data[i] = e;
                data[i+j] = e1;
            }
        }
    }
    pair<int, metaRecord> fillIn;
    for (int i = 0; i < m; i++) {
        auto e = data[i];
        if (e.first == i) {
            fillIn = e;
        } 
        data[i] = fillIn;
    }
} */

/* template<typename T>
void CompactNet<T>::compact(int tableID, int readBinID, uint dataSize, int writeBinID, function<bool(vector<int>&)> func, bool inplace) {
    vector<bool> distinguishedStatus; // IO Block status (with or without distinguished)
    consolidateData(tableID, readBinID, dataSize, distinguishedStatus, func, inplace);
    if (distinguishedStatus.size() > 1) {
        blockCompact(tableID, readBinID, writeBinID, distinguishedStatus, inplace);
    } else {
        writeDataBlock_Wrapper(tableID, writeBinID, 0, workSpace0, workSpace0->size);
        writeDataBlock_Wrapper(tableID, writeBinID+1, 0, workSpace1, workSpace1->size);
        workSpace0->resize(0);
        workSpace1->resize(0);
    }
} */
//* inplace is the flag for join compaction, combined with size of join output, to manage the join compaction
//TODO: treat it as one large array, the block size if either sqrt(N) or MAX_OCALLSIZE. This should improve from 74s for 20000 input 
/* template<typename T>
void CompactNet<T>::consolidateData(int tableID, int readBinID, uint dataSize, vector<bool> &distinguishedStatus, function<bool(vector<int>&)> func, bool inplace) {
    compactBlockSize = IO_BLOCKSIZE;
    if (inplace) {
        compactBlockSize = sqrt(dataSize);
        compactBlockSize = IO_BLOCKSIZE;
    }
    uint numBlocks = ceil(dataSize / (double)compactBlockSize);
    if (inplace) {
        numBlocks += 2;
    }
    int lastBlockDataSize = dataSize % compactBlockSize == 0 ? compactBlockSize : dataSize % compactBlockSize;
    // char *dblock;
    uint startPos = 0;
    int lastBlockStatus = -1; int rcounter = -1; int readFlag;
    if (numBlocks < 2) { 
        // dblock = (char*)malloc(lastBlockDataSize * EDATA_BLOCKSIZE);
        //* tmp
        // readDataBlock_OCALL(&readFlag, -1, readBinID, 0, dblock, lastBlockDataSize * EDATA_BLOCKSIZE, (int)inplace);
        readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, 0, workSpace0, 0, lastBlockDataSize);
        // parseDataBlock(tableID, workSpace0, dblock, lastBlockDataSize);
        workSpaceCompact(tableID, readBinID, startPos, 0, lastBlockStatus, func, lastBlockDataSize, rcounter, inplace);
        // free(dblock);
        return;
        // split it to workSpace0 and [1], no push to distinguishedStatus, return(done), use writeBinID to write directly to untrusted stoarge
    }
    // dblock = (char*)malloc(IO_BLOCKSIZE * EDATA_BLOCKSIZE);
    //* tmp
    // readDataBlock_OCALL(&readFlag, -1, readBinID, startPos, dblock, IO_BLOCKSIZE * EDATA_BLOCKSIZE, (int)inplace);
    readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, startPos, workSpace0, 0, compactBlockSize);
    // parseDataBlock(tableID, workSpace0, dblock);
    uint idx = 1;
    uint workSpaceStart = startPos;
    for (; idx < numBlocks; idx++) {
        uint wsDataSize = compactBlockSize;
        if (idx == numBlocks-1) {
            //* tmp
            // readDataBlock_OCALL(&readFlag, -1, readBinID, workSpaceStart+IO_BLOCKSIZE, dblock, lastBlockDataSize * EDATA_BLOCKSIZE, (int)inplace);
            readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, workSpaceStart+compactBlockSize, workSpace1, 0, lastBlockDataSize);
            // parseDataBlock(tableID, workSpace1, dblock, lastBlockDataSize);
            wsDataSize += lastBlockDataSize;
        } else {
            //* tmp
            // readDataBlock_OCALL(&readFlag, -1, readBinID, workSpaceStart+IO_BLOCKSIZE, dblock, IO_BLOCKSIZE * EDATA_BLOCKSIZE, (int)inplace);
            readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, workSpaceStart+compactBlockSize, workSpace1, 0, compactBlockSize);
            // parseDataBlock(tableID, workSpace1, dblock);
            wsDataSize += compactBlockSize;
        }
        // do compaction for workSpace (flip or not), write out the first IO_BLOCK (no distinguished/full)
        bool flipped = workSpaceCompact(tableID, readBinID, workSpaceStart, workSpaceStart-startPos, lastBlockStatus, func, wsDataSize, rcounter, inplace);

        workSpaceStart += compactBlockSize; 
        if (idx == numBlocks-1) {
            if (flipped == true) {
                if (!(rcounter == compactBlockSize || rcounter == 0)) {
                    printf("possible error rcounter %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", rcounter);
                    return;
                }
                distinguishedStatus.push_back(false); 
                if (rcounter == compactBlockSize) {
                    distinguishedStatus.push_back(true);
                } else {
                    distinguishedStatus.push_back(false);
                }
            } else {
                if (lastBlockStatus == 0){
                    distinguishedStatus.push_back(false); distinguishedStatus.push_back(false);
                } else if(lastBlockStatus == 2) {
                    distinguishedStatus.push_back(true); distinguishedStatus.push_back(false);
                } else if (lastBlockStatus == 3) {
                    distinguishedStatus.push_back(true); distinguishedStatus.push_back(true);
                } else {
                    printf("invalid last blocks status\n");
                }
            }
        } else {
            distinguishedStatus.push_back(!flipped);
        }
        // distinguishedStatus.push_back(true);
    }
    // write out the last block
    // encryptDataBlock(tableID, workSpace0, dblock, lastBlockDataSize);
    //* tmp
    // writeDataBlock_OCALL(tableID, readBinID, workSpaceStart, dblock, lastBlockDataSize * EDATA_BLOCKSIZE);
    writeDataBlock_Wrapper(tableID, readBinID, workSpaceStart, workSpace0, lastBlockDataSize);
    // free(dblock);
    // workSpace0.resize(IO_BLOCKSIZE); workSpace1.resize(IO_BLOCKSIZE);
} */

/* template<typename T>
bool CompactNet<T>::workSpaceCompact(int tableID, int readBinID, uint absStart, uint relativeStart, int &lastBlockStatus, function<bool(vector<int>&)> func, uint inSize, int &rcounter, bool inplace) {
    // uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    bytes empty_r(textSizeVec[tableID], '\0');
    auto in = workSpace0;
    if (inSize > 2*compactBlockSize) {
        printf("workSpace has invalid number of data\n");
    }
    bool flipped = true;
    int counter = 0; 
    vector<int> distArray(inSize, 0);
    if (inSize <= compactBlockSize) {
        workSpace0->resize(inSize);
        workSpace1->resize(0);
        flipped = false;
        // int counter = 0;
        //* construct dist array
        // vector<int> distArray(inSize, 0);
        for (uint i = relativeStart; i < relativeStart+inSize; i++) {
            auto record = in->read(i-relativeStart);
            bytes recordBytes;
            for (int j = 0; j < textSizeVec[tableID]; j++) {
                recordBytes.push_back(record[j]);
            }
            vector<int> recordInts = deconstructIntegers(tableID, recordBytes);
            if (!inplace) {
                uchar bin_id_array[sizeof(int)];
                copy(recordBytes.begin()+textSizeVec[tableID]-sizeof(int), recordBytes.begin()+textSizeVec[tableID], bin_id_array);
                int bin_id = vector<int>((int*)bin_id_array, (int*)bin_id_array+1)[0];
                recordInts.back() = bin_id;
            }
            
            int distance = 0; 
            if (((empty_r == recordBytes || func(recordInts))&&!flipped) || (!(empty_r == recordBytes || func(recordInts))&&flipped)) {
                distance = i - relativeStart - counter;
                counter++;
            } else {
                distance = 0;
            }
            distArray[i-relativeStart] = distance;
        } 
    } else {
        workSpace1->resize(inSize-compactBlockSize);

        //* compare number of distinguished elements (n1) and that of non-distinguished (n2)
        //* n1 >= IO_BLOCKSIZE flipped = false; otherwise, flipped = true
        int flippedCounter = 0;
        // vector<int> distArray(inSize, 0);
        vector<int> distArrayFlipped(inSize, 0);
        for (uint i = relativeStart; i < relativeStart+inSize; i++) {
            int idx;
            if (i-relativeStart < workSpace0->size) {
                in = workSpace0;
                idx = i - relativeStart;
            } else {
                in = workSpace1;
                idx = i - relativeStart - workSpace0->size;
            }
            auto record = in->read(idx);
            bytes recordBytes;
            for (int j = 0; j < textSizeVec[tableID]; j++) {
                recordBytes.push_back(record[j]);
            }
            vector<int> recordInts = deconstructIntegers(tableID, recordBytes);

            if (!inplace) {
                uchar bin_id_array[sizeof(int)];
                copy(recordBytes.begin()+textSizeVec[tableID]-sizeof(int), recordBytes.begin()+textSizeVec[tableID], bin_id_array);
                int bin_id = vector<int>((int*)bin_id_array, (int*)bin_id_array+1)[0];
                recordInts.back() = bin_id;
            }

            if (empty_r == recordBytes || func(recordInts)) {
                distArray[i-relativeStart] = i - relativeStart - counter;
                counter++;
            } else {
                distArrayFlipped[i-relativeStart] = i - relativeStart - flippedCounter;
                flippedCounter++;
            }
            if (counter >= compactBlockSize) {
                flipped = false;
            }
        }
        if (inSize < 2*compactBlockSize) {
            flipped = false;
        }
        if (counter == 0) {
            lastBlockStatus = 0;
        } else if (counter <= compactBlockSize) {
            lastBlockStatus = 2;
        } else {
            lastBlockStatus = 3;
        }
        rcounter = counter;
        if (flipped) {
            distArray = distArrayFlipped;
            counter = flippedCounter;
        }
    }
    //* original j = 0
    auto dest_in = workSpace0;
    for (uint i = 0; i < log2(inSize); i++) {
        for (uint j = pow(2, i); j < inSize; j++) {
            int idx;
            if (j < workSpace0->size) {
                in = workSpace0;
                idx = j;
            } else {
                in = workSpace1;
                idx = j - workSpace0->size;
            }
            auto record = in->read(idx);
            int distance = distArray[j];
            int offset = (distance % (int)pow(2, i+1)); // either 0 or 2^i int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            // int dest = j - offset;
            int dest = j - pow(2, i);
            int dest_idx;
            if (dest < workSpace0->size) {
                dest_in = workSpace0;
                dest_idx = dest;
            } else {
                dest_in = workSpace1;
                dest_idx = dest - workSpace0->size;
            }
            //* swap
            auto toSwap = dest_in->read(dest_idx);
            if (offset == 0) {
                in->write(idx, record);
                dest_in->write(dest_idx, toSwap);
            } else {
                in->write(idx, toSwap);
                dest_in->write(dest_idx, record);
                distance -= offset;
                distArray[j] = 0;
                distArray[dest] = distance;
            }
        }
    }
    if (inSize <= compactBlockSize) {
        if (!inplace) {
            int addSize = workSpace0->size - counter;
            int orgSize = workSpace1->size;
            workSpace1->resize(orgSize+addSize);
            int tmpCounter = counter;
            for (int j = orgSize; j < orgSize+addSize; j++) {
                workSpace1->write(j, workSpace0->read(tmpCounter));
                tmpCounter++;
            }
            // workSpace1.insert(workSpace1.end(), workSpace0.begin()+counter, workSpace0.end());
        }
        workSpace0->resize(counter);
        return flipped;
    }
    // int writeDataSize = inSize < IO_BLOCKSIZE ? inSize : IO_BLOCKSIZE;
    int writeDataSize = compactBlockSize;
    // char *dblock = (char*)malloc(writeDataSize * EDATA_BLOCKSIZE);
    // encryptDataBlock(tableID, workSpace0, dblock, writeDataSize);
    //* tmp
    // writeDataBlock_OCALL(tableID, readBinID, absStart, dblock, writeDataSize * EDATA_BLOCKSIZE);
    writeDataBlock_Wrapper(tableID, readBinID, absStart, workSpace0, writeDataSize);
    workSpace0->erase(writeDataSize);
    int addSize = workSpace1->size;
    int orgSize = workSpace0->size;
    workSpace0->resize(orgSize+addSize);
    for (int j = orgSize; j < orgSize+addSize; j++) {
        workSpace0->write(j, workSpace1->read(j-orgSize));
    }
    // workSpace0.insert(workSpace0.end(), workSpace1.begin(), workSpace1.end());
    // free(dblock);
    return flipped;
}
 */
/* template<typename T>
void CompactNet<T>::blockCompact(int tableID, int readBinID, int writeBinID, vector<bool> &distinguishedStatus, bool inplace) {
    uint inSize = distinguishedStatus.size();
    vector<int> blockDistArray(inSize, 0);
    int counter = 0;
    for (uint i = 0; i < inSize; i++) {
        auto mark = distinguishedStatus[i];
        int distance = 0;
        if (mark) {
            distance = i - counter;
            counter++;
        } else {
            distance = 0;
        }
        blockDistArray[i] = distance;
    }
    if (inplace) {
        printf("distinguished blocks %d\n", counter);
    }
    // char *dblock = (char*)malloc(IO_BLOCKSIZE * EDATA_BLOCKSIZE);
    int readFlag;
    for (uint i = 0; i < log2(inSize); i++) {
        for (uint j = pow(2, i); j < inSize; j++) {
            int distance = blockDistArray[j];
            int offset = (distance % (int)pow(2, i+1));
            int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            //* tmp
            // readDataBlock_OCALL(&readFlag, -1, readBinID, j*IO_BLOCKSIZE, dblock, IO_BLOCKSIZE * EDATA_BLOCKSIZE, (int)inplace);
            readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, j*compactBlockSize, workSpace0, 0, compactBlockSize);
            // parseDataBlock(tableID, workSpace0, dblock);
            
            //* tmp
            // readDataBlock_OCALL(&readFlag, -1, readBinID, dest*IO_BLOCKSIZE, dblock, IO_BLOCKSIZE * EDATA_BLOCKSIZE, (int)inplace);
            // readFlag = readDataBlock_Wrapper(-1, readBinID, dest*IO_BLOCKSIZE, dblock, IO_BLOCKSIZE, (int)inplace);
            // parseDataBlock(tableID, workSpace1, dblock);
            // auto toSwap = workSpace1;
            bool swap = false;
            if (offset == 0) {
                // readFlag = readDataBlock_Wrapper(-1, readBinID, j*IO_BLOCKSIZE, dblock, IO_BLOCKSIZE, (int)inplace);
                // parseDataBlock(tableID, workSpace0, dblock);
                writeDataBlock_Wrapper(tableID, readBinID, j*compactBlockSize, workSpace0, compactBlockSize);
                workSpace0->resize(0);
                readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, dest*compactBlockSize, workSpace1, 0, compactBlockSize);
                // parseDataBlock(tableID, workSpace1, dblock);
                writeDataBlock_Wrapper(tableID, readBinID, dest*compactBlockSize, workSpace1, compactBlockSize);
                workSpace1->resize(0);
            } else {
                swap = true;
                // workSpace1 = workSpace0;
                // workSpace0 = toSwap;
                distance -= offset;
                blockDistArray[j] = 0;
                blockDistArray[dest] = distance;

                writeDataBlock_Wrapper(tableID, readBinID, dest*compactBlockSize, workSpace0, compactBlockSize);
                workSpace0->resize(0);
                readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, dest*compactBlockSize, workSpace1, 0, compactBlockSize);
                // parseDataBlock(tableID, workSpace1, dblock);
                writeDataBlock_Wrapper(tableID, readBinID, j*compactBlockSize, workSpace1, compactBlockSize);
                workSpace1->resize(0);
            }
        }
    }
    // if (!inplace) {
    copy_bin_OCALL(tableID, readBinID, writeBinID, 0, counter*compactBlockSize);
    // printf("inSize %d\n", inSize);
    if (!inplace) {
        copy_bin_OCALL(tableID, readBinID, writeBinID+1, counter*compactBlockSize, inSize*compactBlockSize);
    }
    // }
    // free(dblock);
    // if (inplace) {
    //     printf("compact done\n");
    // }
} */


void enclaveCompact(int noisyOutputSize) {
    int inSize = joinOutput->size;
    /* vector<int> distArray;
    int counter = 0;
    for (int i = 0; i < inSize; i++) {
        auto joinRow = joinOutput->read(i);
        bytes joinRowBytes;
        for (int k = 0; k < sizeof(int); k++) {
            joinRowBytes.push_back(joinRow[k]);
        }
        vector<int> intArray = deconstructIntegers(0, joinRowBytes, sizeof(int));
        int distance = 0;
        if (intArray[0] >= 0) {
            distance = i - counter;
            counter++;
        } else {
            distance = 0;
        }
        distArray.push_back(distance);
    } */
    // printf("jDistArray size %d\n", jDistArray.size());
    for (int i = 0; i < log2(inSize); i++) {
        // for (uint j = 0; j < inSize; j++) {
        for (int j = pow(2, i); j < inSize; j++) {
            auto joinRow1 = joinOutput->read(j);
            int distance = jDistArray[j];
            int offset = (distance % (int)pow(2, i+1)); // start with 0 or 1 (and then 3 5 6 7), binning once for all, timing not much of an issue
            int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            //* swap
            auto joinRow2 = joinOutput->read(dest);
            if (offset > 0) {
                joinOutput->write(j, joinRow2);
                joinOutput->write(dest, joinRow1);
                distance -= offset;
                jDistArray[j] = 0;
                jDistArray[dest] = distance;
            } else {
                joinOutput->write(j, joinRow1);
                joinOutput->write(dest, joinRow2);
            }
        }
    }
    // implicily assuming that IO_BLOCKSIZE > noisyOutputSize
    if (noisyOutputSize >= 0) {
        joinOutput->resize(noisyOutputSize);
    }
    jDistArray.clear();
}

void ORCompact(int noisyOutputSize, TraceMem<tbytes> *data) {
    if (data == nullptr) {
        TightCompact_inner(0, joinOutput->size, selected_count);
        if (noisyOutputSize >= 0 && noisyOutputSize < joinOutput->size) {
            joinOutput->resize(noisyOutputSize);
        }
        jDistArray.clear();
    } else {
        TightCompact_inner(0, data->size, selected_count, data);
        if (noisyOutputSize >= 0 && noisyOutputSize < data->size) {
            data->resize(noisyOutputSize);
        }
    }
}

void swap(int i, int j, bool swap) {
    auto joinRow1 = joinOutput->read(i);
    auto joinRow2 = joinOutput->read(j);
    if (swap) {
        joinOutput->write(i, joinRow2);
        joinOutput->write(j, joinRow1);
    } else {
        joinOutput->write(i, joinRow1);
        joinOutput->write(j, joinRow2);
    }
}

void swap(int i, int j, bool swap, TraceMem<tbytes> *data) {
    auto r1 = data->read(i);
    auto r2 = data->read(j);
    if (swap) {
        data->write(i, r2);
        data->write(j, r1);
    } else {
        data->write(i, r1);
        data->write(j, r2);
    }
}

void TightCompact_inner(int idx, size_t N, uint32_t *selected_count_local, TraceMem<tbytes> *data) {
    if(N==0){
        return;
    }
    else if(N==1){
        return;
    }
    else if(N==2){
        bool selected0 = selected_count_local[1] - selected_count_local[0] == 1 ? true : false;
        bool selected1 = selected_count_local[2] - selected_count_local[1] == 1 ? true : false;
        bool swap_flag = (!selected0 & selected1);
        if (data == nullptr) {
            swap(idx, idx+1, swap_flag);
        } else {
            swap(idx, idx+1, swap_flag, data);
        }
        return;
    }

    size_t gt_pow2;
    size_t split_index;

    // Find largest power of 2 < N 
    gt_pow2 = pow2_lt(N);

    // For Order-preserving ORCompact
    // This will be right (R) of the recursion, and the leftover non-power of 2 left (L)
    split_index = N - gt_pow2;

    // Number of selected items in the non-power of 2 side (left)
    size_t mL;
    if (TC_PRECOMPUTE_COUNTS) {
        mL = selected_count_local[split_index] - selected_count_local[0];
    } 

    // unsigned char *L_ptr = buf;
    // unsigned char *R_ptr = buf + (split_index * block_size);

    int L_idx = idx;
    int R_idx = idx + split_index;

    //printf("Lsize = %ld, Rsize = %ld, Rside offset = %ld\n", split_index, gt_pow2, (gt_pow2 - split_index + mL));
    TightCompact_inner(L_idx, split_index, selected_count_local, data);
    TightCompact_2power_inner(R_idx, gt_pow2, (gt_pow2 - split_index + mL) % gt_pow2, selected_count_local+split_index, data);

    // For OP we CnS the first n_2 elements (split_size) against the suffix n_2 elements of the n_1 (2 power elements)
    // R_ptr = buf + (gt_pow2 * block_size); 
    R_idx = idx + gt_pow2;
    // Perform N-split_index oblivious swaps for this level
    for (size_t i=0; i<split_index; i++){
        // Oswap blocks at L_start, R_start conditional on marked_items
        bool swap_flag = i>=mL;
        if (data == nullptr) {
            swap(L_idx, R_idx, swap_flag);
        } else {
            swap(idx, idx+1, swap_flag, data);
        }
        L_idx+=1;
        R_idx+=1;
    }
}

void TightCompact_2power_inner(int idx, size_t N, size_t offset, uint32_t *selected_count_local, TraceMem<tbytes> *data) {
    if (N==1) {
        return;
    }
    if (N==2) {
        bool selected0 = selected_count_local[1] - selected_count_local[0] == 1 ? true : false;
        bool selected1 = selected_count_local[2] - selected_count_local[1] == 1 ? true : false;
        bool swap_flag = (!selected0 & selected1) ^ offset;
        // oswap_buffer<oswap_style>(buf, buf+block_size, block_size, swap);
        if (data == nullptr) {
            swap(idx, idx+1, swap_flag);
        } else {
            swap(idx, idx+1, swap_flag, data);
        }
        return;
    }

    // Number of selected items in left half
    size_t m1;
    if (TC_PRECOMPUTE_COUNTS) {
        m1 = selected_count_local[N/2] - selected_count_local[0];
    }

    size_t offset_mod = (offset & ((N/2)-1));
    size_t offset_m1_mod = (offset+m1) & ((N/2)-1);
    bool offset_right = (offset >= N/2);
    bool left_wrapped = ((offset_mod + m1) >= (N/2));

    TightCompact_2power_inner(idx, N/2, offset_mod, selected_count_local, data);
    TightCompact_2power_inner(idx + (N/2), N/2, offset_m1_mod, selected_count_local + N/2, data);

    // unsigned char *buf1_ptr = buf, *buf2_ptr = (buf + (N/2)*block_size);
    int L_idx = idx, R_idx = idx+(N/2);
    if (TC_OPT_SWAP_FLAG) {
        bool swap_flag = left_wrapped ^ offset_right;
        size_t num_swap = N/2;
        for(size_t i=0; i<num_swap; i++){
            swap_flag = swap_flag ^ (i == offset_m1_mod);
            // oswap_buffer<oswap_style>(buf1_ptr, buf2_ptr, block_size, swap_flag);
            if (data == nullptr) {
                swap(L_idx, R_idx, swap_flag);
            } else {
                swap(idx, idx+1, swap_flag, data);
            }
            L_idx++; R_idx++;
        }
    } else {
        for(size_t i=0; i<N/2; i++){
            bool swap_flag = (i>=offset_m1_mod) ^ left_wrapped ^ offset_right;
            if (data == nullptr) {
                swap(L_idx, R_idx, swap_flag);
            } else {
                swap(idx, idx+1, swap_flag, data);
            }
            L_idx++; R_idx++;
        }
    }
}