
#include "CONFIG.hpp"
#include "type_def.hpp"
#include "Enclave-utility.hpp"
#include "Enclave_t.h"
#include <math.h>
#include <functional>
#pragma once


extern TraceMem<tbytes>* tWorkSpace[2];
extern TraceMem<cbytes>* jWorkSpace[2];
extern vector<int> jDistArray;

static int compactBlockSize;
int prev_pow_two(int x);

template <typename T>
void expand(vector<pair<int, T>> &data, bool fill=true) {
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
    if (fill) {
        pair<int, T> fillIn;
        for (int i = 0; i < m; i++) {
            auto e = data[i];
            if (e.first == i) {
                fillIn = e;
            } 
            data[i] = fillIn;
        }
    }
}

template <typename T>
void expand(TraceMem<pair<int, T>> *data, bool fill=true) {
    int m = data->size; 
    for (int j = prev_pow_two(m); j >= 1; j /= 2) {
        for (int i = m - j - 1; i >= 0; i--) {
            auto e = data->read(i);
            int dest_i = e.first;
            auto e1 = data->read(i+j);
            if (dest_i >= i + j) {
                // assert(ind_func(e1) == -1);
                data->write(i, e1);
                data->write(i+j, e);
            } else {
                data->write(i, e);
                data->write(i+j, e1);
            }
        }
    }
    if (fill) {
        pair<int, T> fillIn;
        for (int i = 0; i < m; i++) {
            auto e = data->read(i);
            if (e.first == i) {
                fillIn = e;
            } 
            data->write(i, fillIn);
        }
    }
}

template <typename T>
class CompactNet {
    public:
    TraceMem<T> *workSpace0;
    TraceMem<T> *workSpace1;
    CompactNet(TraceMem<T> *workSpace0, TraceMem<T> *workSpace1) : workSpace0(workSpace0), workSpace1(workSpace1){}
    
    void compact(int tableID, int readBinID, uint dataSize, int writeBinID, function<bool(vector<int>&)> func, bool inplace=false) {
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
    }

    void consolidateData(int tableID, int readBinID, uint dataSize, vector<bool> &distinguishedStatus, function<bool(vector<int>&)> func, bool inplace) {
        compactBlockSize = IO_BLOCKSIZE;
        // if (inplace) {
        //     compactBlockSize = sqrt(dataSize);
        //     compactBlockSize = IO_BLOCKSIZE;
        // }
        uint numBlocks = ceil(dataSize / (double)compactBlockSize);
        int lastBlockDataSize = dataSize % compactBlockSize == 0 ? compactBlockSize : dataSize % compactBlockSize;
        uint startPos = 0;
        int lastBlockStatus = -1; int rcounter = -1; int readFlag;
        if (numBlocks < 2) { 
            readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, 0, workSpace0, 0, lastBlockDataSize);
            workSpaceCompact(tableID, readBinID, startPos, 0, lastBlockStatus, func, lastBlockDataSize, rcounter, inplace);
            return;
        }
        int idx = 1;
        int workSpaceStart = startPos;
       
        readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, startPos, workSpace0, 0, compactBlockSize);
        for (; idx < numBlocks; idx++) {
            int wsDataSize = compactBlockSize;
            if (idx == numBlocks-1) {
                readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, workSpaceStart+compactBlockSize, workSpace1, 0, lastBlockDataSize);
                wsDataSize += lastBlockDataSize;
            } else {
                readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, workSpaceStart+compactBlockSize, workSpace1, 0, compactBlockSize);
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
        }
        writeDataBlock_Wrapper(tableID, readBinID, workSpaceStart, workSpace0, lastBlockDataSize);
    }

    bool workSpaceCompact(int tableID, int readBinID, uint absStart, uint relativeStart, int &lastBlockStatus, function<bool(vector<int>&)> func, uint wsDataSize, int &rcounter, bool inplace) {
        bytes empty_r(textSizeVec[tableID], '\0');
        // bytes empty_r(sizeof(int), '\0');
        auto in = workSpace0;
        if (wsDataSize > 2*compactBlockSize) {
            printf("workSpace has invalid number of data\n");
        }
        bool flipped = true;
        int counter = 0, flippedCounter = 0;
        int nempty = 0;
        vector<int> distArray(wsDataSize, 0);
        if (wsDataSize <= compactBlockSize) {
            workSpace0->resize(wsDataSize);
            workSpace1->resize(0);
            flipped = false;
            for (uint i = relativeStart; i < relativeStart+wsDataSize; i++) {
                auto record = in->read(i-relativeStart);
                bytes recordBytes;
                for (int j = 0; j < textSizeVec[tableID]; j++) {
                    recordBytes.push_back(record[j]);
                }
                vector<int> recordInts = deconstructIntegers(tableID, recordBytes);
                /* if (!inplace) {
                    //! this might be removable
                    uchar bin_id_array[sizeof(int)];
                    copy(recordBytes.begin()+textSizeVec[tableID]-sizeof(int), recordBytes.begin()+textSizeVec[tableID], bin_id_array);
                    int bin_id = vector<int>((int*)bin_id_array, (int*)bin_id_array+1)[0];
                    recordInts.back() = bin_id;
                } */
                
                int distance = 0; 
                // if (((empty_r == recordBytes || func(recordInts))&&!flipped) || (!(empty_r == recordBytes || func(recordInts))&&flipped)) {
                if (((empty_r == recordBytes || func(recordInts))&&!flipped)) {
                    distance = i - relativeStart - counter;
                    counter++;
                } else {
                    distance = 0;
                }
                distArray[i-relativeStart] = distance;
            } 
        } else {
            workSpace1->resize(wsDataSize-compactBlockSize);

            //* compare number of distinguished elements (n1) and that of non-distinguished (n2)
            //* n1 >= IO_BLOCKSIZE flipped = false; otherwise, flipped = true
            vector<int> distArrayFlipped(wsDataSize, 0);
            for (uint i = relativeStart; i < relativeStart+wsDataSize; i++) {
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

                /* if (!inplace) {
                    //! this might be removable but still questionable
                    uchar bin_id_array[sizeof(int)];
                    copy(recordBytes.begin()+textSizeVec[tableID]-sizeof(int), recordBytes.begin()+textSizeVec[tableID], bin_id_array);
                    int bin_id = vector<int>((int*)bin_id_array, (int*)bin_id_array+1)[0];
                    recordInts.back() = bin_id;
                } */

                if (empty_r == recordBytes || func(recordInts)) {
                    if (empty_r == recordBytes) {
                        nempty++;
                    }
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
            if (wsDataSize < 2*compactBlockSize) {
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
        // printf("counter %d flippedcounter %d flipped %d, nempty %d\n", counter, flippedCounter, (int)flipped, nempty);
        //* original j = 0
        auto dest_in = workSpace0;
        for (uint i = 0; i < log2(wsDataSize); i++) {
            for (uint j = pow(2, i); j < wsDataSize; j++) {
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
        if (wsDataSize <= compactBlockSize) {
            if (!inplace) {
                int addSize = workSpace0->size - counter;
                int orgSize = workSpace1->size;
                workSpace1->resize(orgSize+addSize);
                int tmpCounter = counter;
                for (int j = orgSize; j < orgSize+addSize; j++) {
                    workSpace1->write(j, workSpace0->read(tmpCounter));
                    tmpCounter++;
                }
            }
            workSpace0->resize(counter);
            return flipped;
        }
        int writeDataSize = compactBlockSize;
        writeDataBlock_Wrapper(tableID, readBinID, absStart, workSpace0, writeDataSize);
        if (absStart == 0 || absStart == compactBlockSize) {
            workSpace0->resize(0);
        } else {
            workSpace0->freeSpace();
        }
        workSpace0 = workSpace1;
        workSpace1 = new TraceMem<T>(0);

        // workSpace0->erase(writeDataSize);
        // int addSize = workSpace1->size;
        // int orgSize = workSpace0->size;
        // workSpace0->resize(orgSize+addSize);
        // for (int j = orgSize; j < orgSize+addSize; j++) {
        //     workSpace0->write(j, workSpace1->read(j-orgSize));
        // }
        return flipped;
    }

    void blockCompact(int tableID, int readBinID, int writeBinID, vector<bool> &distinguishedStatus, bool inplace) {
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
            printf("total blocks %d, distinguished blocks %d\n", inSize, counter);
        }
        int readFlag;
        for (uint i = 0; i < log2(inSize); i++) {
            for (uint j = pow(2, i); j < inSize; j++) {
                int distance = blockDistArray[j];
                int offset = (distance % (int)pow(2, i+1));
                int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
                //* tmp
                readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, j*compactBlockSize, workSpace0, 0, compactBlockSize);
                bool swap = false;
                if (offset == 0) {
                    writeDataBlock_Wrapper(tableID, readBinID, j*compactBlockSize, workSpace0, compactBlockSize);
                    // workSpace0->resize(0);
                    readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, dest*compactBlockSize, workSpace1, 0, compactBlockSize);
                    writeDataBlock_Wrapper(tableID, readBinID, dest*compactBlockSize, workSpace1, compactBlockSize);
                    // workSpace1->resize(0);
                } else {
                    swap = true;
                    distance -= offset;
                    blockDistArray[j] = 0;
                    blockDistArray[dest] = distance;

                    writeDataBlock_Wrapper(tableID, readBinID, dest*compactBlockSize, workSpace0, compactBlockSize);
                    // workSpace0->resize(0);
                    readFlag = readDataBlock_Wrapper(-1, tableID, readBinID, dest*compactBlockSize, workSpace1, 0, compactBlockSize);
                    writeDataBlock_Wrapper(tableID, readBinID, j*compactBlockSize, workSpace1, compactBlockSize);
                    // workSpace1->resize(0);
                }
            }
        }
        if (!inplace) {
            copy_bin_OCALL(tableID, readBinID, writeBinID, 0, counter*compactBlockSize);
            copy_bin_OCALL(tableID, readBinID, writeBinID+1, counter*compactBlockSize, inSize*compactBlockSize);
        }
    }
};

void enclaveCompact(int noisyOutputSize);
void ORCompact(int noisyOutputSize, TraceMem<tbytes> *data=nullptr);
void swap(int i, int j, bool swap);
void swap(int i, int j, bool swap, TraceMem<tbytes> *data);
void TightCompact_inner(int idx, size_t N, uint32_t *selected_count, TraceMem<tbytes> *data=nullptr);
void TightCompact_2power_inner(int idx, size_t N, size_t offset, uint32_t *selected_count, TraceMem<tbytes> *data);


template <typename T>
void compactMeta(TraceMem<T> *in, TraceMem<T> *out1, TraceMem<T> *out2, function<bool(T&)> func, int domainSize=-1) {
    uint inSize = in->size;
    // printf("in size %d\n", in->size);
    vector<int> distArray(inSize, 0);
    int counter = 0;
    for (uint i = 0; i < inSize; i++) {
        // printf("%d ", i);
        auto record = in->read(i);
        int distance = 0;
        if (func(record)) {
            distance = i - counter;
            counter++;
        } else {
            distance = 0;
        }
        distArray[i] = distance;
    }
    // printf("compactMeta have distance, counter %d\n", counter);
    for (uint i = 0; i < log2(inSize); i++) {
        for (uint j = pow(2, i); j < inSize; j++) {
            auto record = in->read(j);
            int distance = distArray[j];
            int offset = (distance % (int)pow(2, i+1)); // start with 0 or 1 (and then 3 5 6 7), binning once for all, timing not much of an issue
            int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            //* swap
            auto toSwap = in->read(dest);
            if (offset > 0) {
                in->write(j, toSwap);
                in->write(dest, record);
                distance -= offset;
                distArray[j] = 0;
                distArray[dest] = distance;
            } else {
                in->write(j, record);
                in->write(dest, toSwap);
            }
        }
    }
    //construct out1 and out2
    // printf("construct out1 and out2\n");
    if (out1 != nullptr) {
        out1->resize(counter);
        for (uint i = 0; i < counter; i++) {
            out1->write(i, in->read(i));
        }
    }
    if (out2 != nullptr) {
        out2->resize(inSize-counter);
        for (uint i = counter; i < inSize; i++) {
            out2->write(i-counter, in->read(i));
        }
    }
    if (out1 == nullptr && out2 == nullptr) {
        if (domainSize > 0) {
            in->resize(domainSize);
        } else {
            in->resize(counter);
        }
    }
}

template <typename T>
void compactMetaVector(vector<T> *in, vector<T> *out1, vector<T> *out2, function<bool(T&)> func, int domainSize=-1) {
    uint inSize = in->size();
    vector<int> distArray(inSize, 0);
    int counter = 0;
    for (uint i = 0; i < inSize; i++) {
        auto record = in->at(i);
        int distance = 0;
        if (func(record)) {
            distance = i - counter;
            counter++;
        } else {
            distance = 0;
        }
        distArray[i] = distance;
    }

    for (uint i = 0; i < log2(inSize); i++) {
        for (uint j = pow(2, i); j < inSize; j++) {
            auto record = in->at(j);
            int distance = distArray[j];
            int offset = (distance % (int)pow(2, i+1)); // start with 0 or 1 (and then 3 5 6 7), binning once for all, timing not much of an issue
            int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            //* swap
            auto toSwap = in->at(dest);
            if (offset > 0) {
                in->at(j) = toSwap;
                in->at(dest) = record;
                distance -= offset;
                distArray[j] = 0;
                distArray[dest] = distance;
            } else {
                in->at(j) = record;
                in->at(dest) = toSwap;
            }
        }
    }
    //construct out1 and out2
    if (out1 != nullptr) {
        out1->insert(out1->end(), in->begin(), in->begin()+counter);
    } else {
        if (domainSize > 0) {
            in->resize(domainSize);
        } else {
            in->resize(counter);
        }
    }
    if (out2 != nullptr) {
        out2->insert(out2->end(), in->begin()+counter, in->end());
    }
}

// void compactRecord(int tableID, int dim, int pos, TraceMem<tbytes> *in, TraceMem<tbytes> *&out1, TraceMem<tbytes> *&out2, vector<int> &keyHelperList, function<bool(int&)> func);

template <typename T>
class ObliSortingNet { 
    protected:
    vector<T> *dataVec;
    TraceMem<T> *tmDataVec;
    function<bool(T&, T&)> func;

    void oddeven_merge(size_t lo, size_t hi, size_t r, size_t len, bool tm) {
        size_t step = r * 2;
        if(step < hi - lo) {
            oddeven_merge(lo, hi, step, len, tm);
            oddeven_merge(lo + r, hi, step, len, tm);
            for(size_t i = lo + r; i < hi - r; i += step) {
                if(i + r < len) {
                    if (!tm) {
                        compare_and_swap_fast(true, i, i + r);
                    } else {
                        compare_and_swap_tm_fast(true, i, i + r);
                    }
                }
            }
        }
        else {
            if(lo + r < len) {
                if (!tm) {
                    compare_and_swap_fast(true, lo, lo + r);
                } else {
                    compare_and_swap_tm_fast(true, lo, lo + r);
                }
            }
        }
    }

    void oddeven_merge_sort_range(size_t lo, size_t hi, size_t len, bool tm) {
        if(hi - lo >= 1) {
            size_t mid = lo + (hi - lo) / 2;
            oddeven_merge_sort_range(lo, mid, len, tm);
            oddeven_merge_sort_range(mid + 1, hi, len, tm);
            oddeven_merge(lo, hi, 1, len, tm);
        }
    }


    // bool compare_func_fast(T& entry1, T& entry2);

    void compare_and_swap_fast(bool ascend, size_t a, size_t b) {
        T entry1 = dataVec->at(a);
        T entry2 = dataVec->at(b);
        if (!func(entry1, entry2) == ascend) {
            dataVec->at(a) = entry2;
            dataVec->at(b) = entry1;
        } else {
            dataVec->at(a) = entry1;
            dataVec->at(b) = entry2;
        }
    }

    void compare_and_swap_tm_fast(bool ascend, size_t a, size_t b) {
        T entry1 = tmDataVec->read(a);
        T entry2 = tmDataVec->read(b);
        if (!func(entry1, entry2) == ascend) {
            tmDataVec->write(a, entry2);
            tmDataVec->write(b, entry1);
        } else {
            tmDataVec->write(a, entry1);
            tmDataVec->write(b, entry2);
        }
    }

    public:
    ObliSortingNet(vector<T> *input, function<bool(T&, T&)> func) {
        this->dataVec = input;
        this->func = func;
    }

    ObliSortingNet(TraceMem<T> *input, function<bool(T&, T&)> func) {
        this->tmDataVec = input;
        this->func = func;
    }

    /**
     * @brief batcher sort (TraceMem)
     * @param len number of input blocks
     * @param tm is TraceMem (vector otherwise)
     */
    void obliSort(size_t len, bool tm=false) {
        size_t test = len;
        size_t upper = 0;
        while(test != 0) {
            test >>= 1;
            upper ++;
        }
        oddeven_merge_sort_range(0, (1 << upper) - 1, len, tm);
    }
};