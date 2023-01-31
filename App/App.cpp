/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <map>
#include <memory>
#include <iostream>
#include <algorithm>
# define MAX_PATH FILENAME_MAX

#include "App.h"
#include "CONFIG.hpp"
#include "type_def.hpp"
#include "app-storage.hpp"
#include "app-utility.hpp"
#include "ssl-server.hpp"
using namespace std;

vector<map<uint, shared_ptr<AbsStorage>>> untrustedTableStorage;
map<uint, shared_ptr<AbsStorage>> untrustedBinStorage;
vector<vector<int>> test_data, binned_data;
int selectIdx = 0;
vector<pair<int, int>> test_queries;
// string logfile_name = "log/log-createPDS-time-storage.txt";
float avg_res_blowup = 0;
vector<int> ansBins;
int ansTable = 0;

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;
typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int initialize_enclave(void)
{
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;
    
    /* Step 1: try to retrieve the launch token saved by last transaction 
     *         if there is no token, then create a new one.
     */
    /* try to get the token saved in $HOME */
    const char *home_dir = getpwuid(getuid())->pw_dir;
    
    if (home_dir != NULL && 
        (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
    } else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }

    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        if (fp != NULL) fclose(fp);
        return -1;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);
    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}

int readDataBlock_OCALL(int tableID, int readBinID, int pos, char *dblock, int dsize) {
    int numData = dsize / EDATA_BLOCKSIZE;
    shared_ptr<AbsStorage> binStorage;
    if (tableID < 0) {
        binStorage = untrustedBinStorage[readBinID];
    } else {
        binStorage = untrustedTableStorage[tableID][readBinID];
    }
    number meta = binStorage->meta()+1;
    number storageSize = binStorage->size()/binStorage->getBlockSize();
    auto iter = dblock;
    int dataSize = (int)storageSize - 1;
    for (uint i = 0; i < numData; i++) {
        if (pos+meta+i > storageSize) {
            bytes record(binStorage->getBlockSize(), '\0');
            if (tableID < 0) {
                auto address = binStorage->malloc();
                binStorage->set(address, record); 
            } else {
                return dataSize;
            }
            copy(record.begin(), record.end(), iter);
            iter += EDATA_BLOCKSIZE; 
        } else {
            bytes response;
            binStorage->get(pos+meta+i, response);
            copy(response.begin(), response.end(), iter);
            iter += EDATA_BLOCKSIZE;
        }
    }
    return dataSize;
}

void writeDataBlock_OCALL(int tableID, int writeBinID, uint pos, char *dblock, uint dsize) {
    uint numData = dsize / EDATA_BLOCKSIZE;
    if (untrustedBinStorage.find(writeBinID) == untrustedBinStorage.end()) {
        untrustedBinStorage[writeBinID] = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE);
    }
    number meta = untrustedBinStorage[writeBinID]->meta()+1;
    // printf("byte size %d, block size %d\n", untrustedBinStorage[writeBinID]->size(), untrustedBinStorage[writeBinID]->getBlockSize());
    number storageSize = untrustedBinStorage[writeBinID]->size()/untrustedBinStorage[writeBinID]->getBlockSize();
    auto iter = dblock;
    for (uint i = 0; i < numData; i++) {
        if (pos+meta+i > storageSize) {
            // this is an error, the data is written back to the readBinID (inplace), so it can't be 
            // larger than the original size
            untrustedBinStorage[writeBinID]->malloc(); // break;
        }
        bytes response(iter, iter+EDATA_BLOCKSIZE);
        
        untrustedBinStorage[writeBinID]->set(pos+meta+i, response);
        iter += EDATA_BLOCKSIZE;
    }
}

void expand_table_OCALL(int tableID, uint noiseSum, char *noise, uint nsize) {
    uint originalSize = untrustedTableStorage[tableID][0]->nometaSize() / untrustedTableStorage[tableID][0]->getBlockSize();
    uint iter = 0;
    auto noise_iter = noise;
    bytes dummyRecord;
    // bytes record(untrustedTableStorage[tableID][0]->getBlockSize(), '\0');
    // record.resize(untrustedTableStorage[tableID][0]->getBlockSize());
    while (iter < noiseSum) {
        dummyRecord.clear();
        dummyRecord.insert(dummyRecord.end(), noise_iter, noise_iter+EDATA_BLOCKSIZE);
        noise_iter += EDATA_BLOCKSIZE;
        auto address = untrustedTableStorage[tableID][0]->malloc();
        untrustedTableStorage[tableID][0]->set(address, dummyRecord);
        iter++;
    }
    // cout << "expanded table size: " << untrustedTableStorage[tableID][0]->nometaSize() / untrustedTableStorage[tableID][0]->getBlockSize() << endl;
}

void copy_bin_OCALL(int tableID, int readBinID, int writeBinID, int startPos, int endPos) {
    untrustedBinStorage[writeBinID] = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE); //* create a new storage anyway
    number meta = untrustedBinStorage[readBinID]->meta()+1;
    int tmp_count = 0;
    for (int i = startPos; i < endPos; i++) {
        bytes record;
        untrustedBinStorage[readBinID]->get(meta+i, record);
        auto address = untrustedBinStorage[writeBinID]->malloc();
        untrustedBinStorage[writeBinID]->set(address, record);
        tmp_count++;
    }
}

void move_resize_bin_OCALL(int tableID, int srcBinID, int destBinID, int size) {
    untrustedBinStorage[destBinID] = untrustedBinStorage[srcBinID];
    untrustedBinStorage[srcBinID].reset();
    untrustedBinStorage.erase(srcBinID);
    int dataSize = (int)(untrustedBinStorage[destBinID]->size()/untrustedBinStorage[destBinID]->getBlockSize()) - 1;
    if (size > dataSize) {
        cout << "possible error" << endl;
    }
    untrustedBinStorage[destBinID]->resize(size);
    //* correctness check
    /* auto binStorage = untrustedBinStorage[destBinID];
    auto numRecords = binStorage->nometaSize() / binStorage->getBlockSize();
    auto location = binStorage->meta()+1;
    int textSize = 2 * sizeof(int);
    uchar plaintext[textSize];
    number cipherSize = computeCiphertextSize(textSize);
    uchar ciphertext[cipherSize];
    uchar tag[TAG_SIZE];
    int check_min = INT_MAX, check_max = INT_MIN;
    for (number i = 0; i < numRecords; i++, location++) {
        bytes record;
        binStorage->get(location, record);
        copy(record.begin(), record.begin()+cipherSize, ciphertext);
        copy(record.begin()+cipherSize, record.begin()+cipherSize+TAG_SIZE, tag);
        aes_gcm_128_decrypt(ciphertext, cipherSize, nullptr, 0, tag, aes_key, hardcoded_iv, IV_LENGTH, plaintext);
        vector<int> values((int *)plaintext, (int *)plaintext+2);
        if (values[0] != 0 && values[0] < check_min) {
            check_min = values[0];
        }
        if (values[0] != 0 && values[0] > check_max) {
            check_max = values[0];
        }
    }
    assert(check_min >= min && check_max <= max); */
}

void move_resize_bin_totable_OCALL(int tableID, int srcBinID, int destBinID, int size) {
    untrustedTableStorage[tableID][destBinID] = untrustedBinStorage[srcBinID];
    untrustedBinStorage[srcBinID].reset();
    untrustedBinStorage.erase(srcBinID);
    untrustedTableStorage[tableID][destBinID]->resize(size);

    //* correctness check
    /* uint textSize = textSizeVec[tableID];
    number cipherSize = computeCiphertextSize(textSize);
    uchar plaintext[textSize];
    uchar ciphertext[cipherSize];
    uchar tag[TAG_SIZE];
    auto binStorage = untrustedTableStorage[tableID][destBinID];
    auto numRecords = binStorage->nometaSize() / binStorage->getBlockSize();
    // cout << "actual size " << numRecords << endl;
    auto location = binStorage->meta()+1;
    // numRecords = numRecords < size ? numRecords : size;
    for (number i = 0; i < numRecords; i++, location++) {
        bytes record;
        binStorage->get(location, record);
        copy(record.begin(), record.begin()+cipherSize, ciphertext);
        copy(record.begin()+cipherSize, record.begin()+cipherSize+TAG_SIZE, tag);
        aes_gcm_128_decrypt(ciphertext, cipherSize, nullptr, 0, tag, aes_key, hardcoded_iv, IV_LENGTH, plaintext);
        vector<int> values((int *)plaintext, (int *)plaintext+(textSize/sizeof(int)));
        if (values.back() != destBinID) {
            cout << "current bin " << destBinID << " wrong bin id " << values.back() << endl;
            exit(0);
        }
    } */
}

void initBinStorage_OCALL(int tableID, int binID) {
    untrustedBinStorage.clear();
    untrustedBinStorage.insert({binID, untrustedTableStorage[tableID][binID]});
    untrustedTableStorage[tableID][binID].reset();
    untrustedTableStorage[tableID].erase(binID);
}

void delete_bin_OCALL(int tableID, int binID) {
    untrustedBinStorage[binID].reset();
    untrustedBinStorage.erase(binID);
}

void delete_table_OCALL(int tableID) {
    for (auto &&entry : untrustedTableStorage[tableID]) {
        entry.second.reset();
    }
    untrustedTableStorage[tableID].clear();
}

void collect_resize_bins_OCALL(int tableID, char *leaves_id, uint dsize, char *binSizes, uint bsize) {
    vector<int> leaves((int*)leaves_id, (int*)leaves_id + dsize/sizeof(int));
    vector<int> sizes((int*)binSizes, (int*)binSizes + bsize/sizeof(int));
    // if (leaves.size() > 1) {
    //     untrustedTableStorage[tableID][0].reset(); untrustedTableStorage[tableID].erase(0);
    //     untrustedBinStorage[0].reset(); untrustedBinStorage.erase(0);
    // }
    for (uint i = 0; i < leaves.size(); i++) {
        untrustedTableStorage[tableID][leaves[i]] = untrustedBinStorage[leaves[i]];
        untrustedTableStorage[tableID][leaves[i]]->resize(sizes[i]);
        untrustedBinStorage[leaves[i]].reset();
    }
    untrustedBinStorage.clear();
}

void write_index_OCALL(int structureID, char *s_idxBinStart, uint size1, char *s_idxBinEnd, uint size2, const char *str) {
    string source(str); 
    string filename = "binned-data/" + source + "_index.txt";
    ofstream outfile (filename);
    if (!outfile.is_open()) {
        cout << filename << " not open" << endl;
        return;
    }
    // vector<number> tmp1((number*)s_idxBinStart, (number*)s_idxBinStart+size1/sizeof(number));
    // vector<number> tmp2((number*)s_idxBinEnd, (number*)s_idxBinEnd+size2/sizeof(number));
    // uint nLeaves1 = size1 / (2*sizeof(number));
    // uint nLeaves2 = size2 / (2*sizeof(number));
    outfile << size1 << " " << size2 << endl;
    outfile.write(s_idxBinStart, size1);
    outfile.write(s_idxBinEnd, size2);
    outfile.close();
}

void write_join_output_OCALL(int writeID, char *res, uint dsize) {
    if (untrustedTableStorage.size() <= writeID) {
        untrustedTableStorage.resize(writeID+1);
        shared_ptr<AbsStorage> output = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE);
        untrustedTableStorage[writeID].insert({0, output});
    }
    auto binStorage  = untrustedTableStorage[writeID][0];
    uint nRecords = dsize / EDATA_BLOCKSIZE;
    uint iter = 0;
    bytes match;
    auto res_iter = res;
    while (iter < nRecords) {
        match.clear();
        match.insert(match.end(), res_iter, res_iter+EDATA_BLOCKSIZE);
        res_iter += EDATA_BLOCKSIZE;
        //*tmp
        auto address = binStorage->malloc();
        // auto address = binStorage->meta();
        binStorage->set(address, match);
        iter++;
    }
}

/* 
void read_pair_records_OCALL(uint tableID, uint pos1, char *r1, uint pos2, char *r2, uint rsize) {
    number meta = untrustedTableStorage[tableID]->meta()+1;
    bytes response;
    untrustedTableStorage[tableID]->get(pos1+meta, response);
    copy(response.begin(), response.end(), r1);
    untrustedTableStorage[tableID]->get(pos2+meta, response);
    copy(response.begin(), response.end(), r2);
}

void swap_OCALL(uint tableID, uint pos1, char* r1, uint pos2, char *r2, int rsize) {
    number meta = untrustedTableStorage[tableID]->meta()+1;
    bytes record1((uchar *)r1, (uchar *)r1+rsize);
    untrustedTableStorage[tableID]->set(pos1+meta, record1);
    bytes record2((uchar *)r2, (uchar *)r2+rsize);
    untrustedTableStorage[tableID]->set(pos2+meta, record2);
}

void init_bin_OCALL(int tableID, int binID, uint size) {//* no use of tableID
    auto binIter = untrustedBinStorage.find(binID);
    if (binIter == untrustedBinStorage.end()) {
        untrustedBinStorage[binID] = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE);
    } else {
        untrustedBinStorage[binID]->reset();
    }
    bytes record;
    record.resize(untrustedBinStorage[binID]->getBlockSize());
    for (uint i = 0; i < size; i++) {
        auto address = untrustedBinStorage[binID]->malloc();
        untrustedBinStorage[binID]->set(address, record);
    }
} */

/* helper functions */
/* void loadTable(shared_ptr<AbsStorage> storage, string source) {
    ifstream in_file;
    in_file.open(source);
    if (!in_file.is_open()) {
        cout << "Error opening table input file" << endl;
        exit(0);
    }
    int n_data, n_attr;
    in_file >> n_data >> n_attr;
    int attrVal;
    int textSize = (n_attr+1) * sizeof(int);
    uchar plaintext[textSize];
    number cipherSize = computeCiphertextSize(textSize);
    uchar ciphertext[cipherSize];
    uchar tag[TAG_SIZE];
    // uchar *decryptedtext = new uchar[textSize];

    for (int i = 0; i < n_data; i++) {
        uchar *iter = plaintext;
        vector<int> in_record;
        for (int j = 0; j < n_attr; j++) {
            in_file >> attrVal;
            in_record.push_back(attrVal);
            if (storage !=  nullptr) {
                memcpy(iter, &attrVal, sizeof(int));
                iter += sizeof(int);
            }
        }
        if (storage !=  nullptr) {
            memcpy(iter, &i, sizeof(int));
        }
        test_data.push_back(in_record);
        if (storage != nullptr) {
            int ciphertext_len = aes_gcm_128_encrypt(plaintext, cipherSize, nullptr, 0,
                                                 aes_key, hardcoded_iv, IV_LENGTH, ciphertext, tag);
            bytes record(ciphertext, ciphertext+cipherSize);
            record.insert(record.end(), tag, tag+TAG_SIZE);      
            record.resize(storage->getBlockSize());
            auto address = storage->malloc();
            storage->set(address, record);
        }
    }
    sort(test_data.begin(), test_data.end());
    in_file.close();
} */

void loadQueries(string source) {
    ifstream in_file;
    in_file.open(source);
    if (!in_file.is_open()) {
        cout << "Error opening query input file" << endl;
        exit(0);
    }
    int n_data;
    in_file >> n_data >> selectIdx;
    int attrVal;
    pair<int, int> query;
    for (int i = 0; i < n_data; i++) {
        for (int j = 0; j < 2; j++) {
            in_file >> attrVal;
            if (j % 2 == 0 ) {
                query.first = attrVal;
            } else {
                query.second = attrVal;
            }
        }
        test_queries.push_back(query);
    }
}

void writeBinnedData(int tableID, string source) {
    string filename = "binned-data/" + source + "_binned_data.txt";
    ofstream outfile(filename);
    // ofstream logfile;
    // logfile.open(logfile_name, ios_base::app);
    if (!outfile.is_open()) {
        cout << filename << " not open" << endl;
        return;
    }
    // if (!logfile.is_open()) {
    //     cout << logfile_name << " not open" << endl;
    //     return;
    // }
    number sumRecords = 0;
    outfile << untrustedTableStorage[tableID].size() << endl;
    for (auto &&map_pair : untrustedTableStorage[tableID]) {
        auto binID = map_pair.first;
        auto binStorage = map_pair.second;
        auto numRecords = binStorage->nometaSize() / binStorage->getBlockSize();
        sumRecords += numRecords;
        outfile << binID << " " << numRecords << endl;
        auto location = binStorage->meta()+1;
        char encrypted_data[binStorage->getBlockSize()];
        for (number i = 0; i < numRecords; i++, location++) {
            bytes record;
            binStorage->get(location, record);
            copy(record.begin(), record.end(), encrypted_data);
            // outfile << encrypted_data << endl;
            outfile.write(encrypted_data, binStorage->getBlockSize());
        }
        outfile << endl;
        binStorage.reset();
    }
    //* uncomment this if no checkSelectCorrectness
    // untrustedTableStorage[tableID].clear();
    // logfile << "size of pds " << sumRecords << " records" << endl;
    outfile.close();
}

void loadBinnedData(int tableID, string source, bool restore_join_meta, int join_idx) {
    untrustedTableStorage.resize(tableID+1);
    // string filename = "table" + to_string(tableID+1) + "_encrypted_binned_data.txt";
    string filename = "binned-data/" + source + "_binned_data.txt";
    ifstream infile (filename);
    if (!infile.is_open()) {
        cout << filename << " not open" << endl;
        return;
    }
    int textSize = textSizeVec[tableID];
    int numBins, binID, numRecords;
    infile >> numBins;
    for (int i = 0; i < numBins; i++) {
        infile >> binID >> numRecords;
        char *encryptedBin = (char*)malloc(numRecords*EDATA_BLOCKSIZE);
        string eol;
        getline(infile, eol);
        untrustedTableStorage[tableID][binID] = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE);
        auto eiter = encryptedBin;
        for (int j = 0; j < numRecords; j++) {
            char buffer[EDATA_BLOCKSIZE];
            infile.read(buffer, EDATA_BLOCKSIZE);
            if (restore_join_meta) {
                memcpy(eiter, buffer, EDATA_BLOCKSIZE);
                eiter += EDATA_BLOCKSIZE;
            }
            auto address = untrustedTableStorage[tableID][binID]->malloc();
            bytes record(buffer, buffer+EDATA_BLOCKSIZE);
            untrustedTableStorage[tableID][binID]->set(address, record);
        }
        if (restore_join_meta) {
            restoreJoinMeta(global_eid, tableID, binID, i, join_idx, encryptedBin, numRecords*EDATA_BLOCKSIZE);
        }
        free(encryptedBin);
        getline(infile, eol);
    }
    infile.close();
}

void loadIndex(int structureID, char *&s_idxBinStart, char *&s_idxBinEnd, uint &size1, uint &size2, string source) {
    string filename = "binned-data/" + source + "_index.txt";
    ifstream infile (filename);
    if (!infile.is_open()) {
        cout << filename << " not open" << endl;
        return;
    }
    infile >> size1 >> size2;
    string eol;
    getline(infile, eol);
    s_idxBinStart = (char*)malloc(size1);
    s_idxBinEnd = (char*)malloc(size2);
    infile.read(s_idxBinStart, size1);
    infile.read(s_idxBinEnd, size2);
    // vector<number> tmp1((number*)s_idxBinStart, (number*)s_idxBinStart+size1/sizeof(number));
    // vector<number> tmp2((number*)s_idxBinEnd, (number*)s_idxBinEnd+size2/sizeof(number));
    infile.close();
}

int checkBlowup(int tableID, string data_source, int Rsize, int query_dim=0, int qstart=INT_MIN, int qend=INT_MAX) {
    if (test_data.empty()) {
        loadTable(tableID, nullptr, data_source, test_data);
    }
    vector<vector<int>> realAns;
    for (uint i = 0; i < test_data.size(); i++) {
        if (test_data[i][query_dim] >= qstart && test_data[i][query_dim] <= qend) {
            realAns.push_back(test_data[i]);
        }
    }
    sort(realAns.begin(), realAns.end());
    if (realAns.size() > 0) {
        float blowup = (float)Rsize / (float)realAns.size();
        avg_res_blowup += blowup;
        return 1;
    } else {
        return 0;
    }
}

/* int checkSelectCorrectness(int tableID, string data_source, int query_dim=0, int qstart=INT_MIN, int qend=INT_MAX, vector<int> *sbinIDs=nullptr) {
    if (test_data.empty()) {
        loadTable(tableID, nullptr, data_source, test_data);
    }
    vector<vector<int>> realAns;
    binned_data.clear();
    int sbin_iter = 0;
    for (auto &&map_pair : untrustedTableStorage[tableID]) {
        auto binID = map_pair.first;
        if (!sbinIDs->empty()) {
            if (sbin_iter < sbinIDs->size()) {
                int sbinID = sbinIDs->at(sbin_iter);
                if (binID != sbinID) {
                    continue;
                }
            } else {
                continue;
            }
        } else {
            break;
        }
        auto binStorage = map_pair.second;
        auto numRecords = binStorage->nometaSize() / binStorage->getBlockSize();
        auto location = binStorage->meta()+1;
        int textSize = textSizeVec[tableID];
        uchar plaintext[textSize];
        number cipherSize = computeCiphertextSize(textSize);
        uchar ciphertext[cipherSize];
        uchar tag[TAG_SIZE];
        for (number i = 0; i < numRecords; i++, location++) {
            bytes record;
            binStorage->get(location, record);
            copy(record.begin(), record.begin()+cipherSize, ciphertext);
            copy(record.begin()+cipherSize, record.begin()+cipherSize+TAG_SIZE, tag);
            aes_gcm_128_decrypt(ciphertext, cipherSize, nullptr, 0, tag, aes_key, hardcoded_iv, IV_LENGTH, plaintext);
            vector<int> values((int *)plaintext, (int *)plaintext+(textSize/sizeof(int)-2));
            binned_data.push_back(values);
        }
        sbin_iter++;
    }
    sort(binned_data.begin(), binned_data.end());
    for (uint i = 0; i < test_data.size(); i++) {
        if (test_data[i][query_dim] >= qstart && test_data[i][query_dim] <= qend) {
            realAns.push_back(test_data[i]);
        }
    }
    sort(realAns.begin(), realAns.end());
    bool correctness = includes(binned_data.begin(), binned_data.end(), realAns.begin(), realAns.end());
    if (!correctness) {
        int tmp_count = 0;
        for (auto &&res_pair : binned_data) {
            cout << "(";
            for (auto &&val : res_pair) {
                cout << "%d " << val;
            }
            cout << ")";
            tmp_count++;
            if (tmp_count % 10 == 0) {
                cout << endl;
                tmp_count = 0;
            }
        }
        cout << "query dim " << query_dim << " start " << qstart << " end " << qend << " incorrect" << endl;
        exit(0);
    }
    if (realAns.size() > 0) {
        float blowup = (float)binned_data.size() / realAns.size();
        avg_res_blowup += blowup;
        return 1;
    } else {
        return 0;
    }
} */

void replyClient(SSL *ssl) {
    int reVolume = 0;
    for (int i = 0; i < ansBins.size(); i++) {
        // auto binStorage = untrustedTableStorage[ansTable][ansBins[i]];
        auto binStorage = untrustedBinStorage[ansBins[i]];
        int dataSize = (int)(binStorage->size()/binStorage->getBlockSize()) - 1;
        // printf("ansBins[%d]=%d, data size %d\n", i, ansBins[i], dataSize);
        number meta = binStorage->meta()+1;
        char reply[binStorage->getBlockSize()];
        for (int j = meta; j < meta+dataSize; j++) {
            bytes response;
            binStorage->get(j, response);
            copy(response.begin(), response.end(), reply);
            SSL_write(ssl, reply, strlen(reply));
        }
        reVolume += dataSize;
    }
    // printf("%d records returned to client\n", reVolume);
}


/* Application entry */
int main(int argc, char *argv[]) {
    (void)(argc);
    (void)(argv);
    /* Main code */
    // The input file has two columns, the first one is join attribute, the second one is data attribute. Both attributes have type integer
    // The first row in the input file has two numbers. Indicate the size of table 1 and table 2 respectively
    //* INT_MAX is a reserved flag for dummy entries/noise
    //TODO 
    // if (!(argc == 7 || argc == 5)) {
    if (!(argc == 8 || argc == 7 || argc == 9)) {
        printf("Wrong number of input parameters: %s\n select tables/bins DATA_FILE QUERY_FILE HTree/DO constant\n\
                    join tables/bins DATA_FILE1 DATA_FILE2 HTree/DO/CZSC21 constant pf-join/general\n", argv[0]);
        exit(0);
    }
    query_type op;
    string queryPath;
    data_source ds;
    join_type jt;
    time_t start_time, end_time;
    double usedtime;
    bool pfJoinFlag = false;
    if (strcmp(argv[1], "join") == 0 || strcmp(argv[1], "select-join") == 0) {
        if (strcmp(argv[1], "join") == 0) {
            op = Join;
            if (argc != 8) {
                printf("Usage: %s join tables/bins DATA_FILE1 DATA_FILE2 HTree/DO/CZSC21 constant pf-join/general\n", argv[0]);
                exit(0);
            }
        }
        if (strcmp(argv[1], "select-join") == 0) {
            op = SelectJoin;
            if (argc != 9) {
                printf("Usage: %s select-join tables/bins DATA_FILE1 DATA_FILE2 HTree constant pf-join/general QUERY_FILE\n", argv[0]);
                exit(0);
            }
            queryPath = argv[argc-1];
            loadQueries(queryPath);
        }
        untrustedTableStorage.resize(2);
        if (strcmp(argv[7], "pf-join") == 0) {
            pfJoinFlag = true;
        }
    }
    if (strcmp(argv[1], "select") == 0) {
        if (argc != 7) {
            printf("Usage: %s select tables/bins DATA_FILE QUERY_FILE HTree/DO constant\n", argv[0]);
            exit(0);
        }
        op = Select;
        untrustedTableStorage.resize(1);
        queryPath = argv[4];
        // loadQueries("query/query_1d_100000_pert=20.txt");
        loadQueries(queryPath);
    }
    if (strcmp(argv[2], "tables") == 0) {
        ds = Tables;
    }
    if (strcmp(argv[2], "bins") == 0) {
        ds = Bins;
    }
    if (strcmp(argv[5], "CZSC21") == 0) {
        jt = CZSC21;
        if (strcmp(argv[2], "bins") == 0) {
            printf("CZSC21 join does not support bins\n");
            exit(0);
        }
    } else if (strcmp(argv[5], "DO") == 0) {
        jt = DO;
    } else if (strcmp(argv[5], "HTree") == 0) {
        jt = HTreeDO;
    } else {
        printf("Input error: invalid join type\n");
    }
    constant = stof(argv[6]);
    // int port = atoi(argv[argc-1]);
    
    /* ofstream logfile;
    logfile.open(logfile_name, ios_base::app);
    if (!logfile.is_open()) {
        cout << logfile_name << "is not open" << endl;
        exit(0);
    } */
    int tableID1 = 0, tableID2 = 1;
    int join_idx1 = 0, join_idx2 = 0;
    char *sdata1, *sdata2, *sensitiveAttr1, *sensitiveAttr2;
    uint sattrSize = sensitive_dim * sizeof(int); // both attr1 and attr2 are sensitive
    string tablePath1 = argv[3];
    string source1 = tablePath1.substr(0, tablePath1.find("."));
    source1 = source1.substr(source1.find("/")+1);
    string tablePath2, source2;
    if (op == Join || op == SelectJoin) {
        tablePath2 = argv[4];
        source2 = tablePath2.substr(0, tablePath2.find("."));
        source2 = source2.substr(source2.find("/")+1);
    }
    //* load and encrypt
    if (ds == Tables) {
        // cout << source1 << " " << source2 << endl;
        shared_ptr<AbsStorage> t1Storage = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE);
        if (source1 == "supplier") {
            loadSupplier(tableID1, t1Storage, tablePath1);
        } else if (source1 == "customer") {
            loadCustomer(tableID1, t1Storage, tablePath1);
        } else {
            loadTable(tableID1, t1Storage, tablePath1, test_data);
        }
        // cout << "load t1" << endl;
        untrustedTableStorage[0].insert({0, t1Storage});
        sdata1 = new char[t1Storage->nometaSize()];
        t1Storage->serializeData(sdata1);
        sensitiveAttr1 = new char[sattrSize];
        auto iter = sensitiveAttr1;
        for (int i = 0; i < sensitive_dim; i++) {
            memcpy(iter, &i, sizeof(int));
            iter += sizeof(int);
        }
        // cout << "serialize t1" << endl;
        if (op == Join || op == SelectJoin) {
            shared_ptr<AbsStorage> t2Storage = make_shared<InMemoryStorage>(EDATA_BLOCKSIZE);
            if (source2 == "customer") {
                loadCustomer(tableID2, t2Storage, tablePath2);
            } else if (source2 == "supplier") {
                loadSupplier(tableID2, t2Storage, tablePath2);
            } else {
                loadTable(tableID2, t2Storage, tablePath2, test_data);
            }
            // cout << "load t2" << endl;
            vector<vector<int>>().swap(test_data);
            untrustedTableStorage[1].insert({0, t2Storage});
            sdata2 = new char[t2Storage->nometaSize()];
            t2Storage->serializeData(sdata2);
            sensitiveAttr2 = new char[sattrSize];
            auto iter = sensitiveAttr2;
            for (int i = 0; i < sensitive_dim; i++) {
                memcpy(iter, &i, sizeof(int));
                iter += sizeof(int);
            }
        }
    }
    // toggle
    /* establish secure communication channel between client and server */
    /* int sock;
    SSL_CTX *ctx;
    ctx = create_context();
    configure_context(ctx);
    sock = create_socket(8000);
    struct sockaddr_in addr;
    unsigned int len = sizeof(addr);
    SSL *ssl;
    int client = accept(sock, (struct sockaddr*)&addr, &len);
    if (client < 0) {
        perror("Unable to accept");
        exit(EXIT_FAILURE);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client);

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } */
      
    //* encrypted data loaded in untrusted storage
    /* Initialize the enclave */
    if(initialize_enclave() < 0){
		printf("Enter a character before exit ...\n");
		getchar();
		return -1; 
    }
    /* Utilize edger8r attributes */
    edger8r_array_attributes();
    edger8r_pointer_attributes();
    edger8r_type_attributes();
    edger8r_function_attributes();
    /* Utilize trusted libraries */
    ecall_libc_functions();
    ecall_libcxx_functions();
    sgx_status_t sgx_return = SGX_SUCCESS;
    // out of date
    if (ds == Bins) {
        bool restore_join_meta = false;
        if (op == Join && join_simulation) {
            restore_join_meta = true;
        }
        loadBinnedData(tableID1, source1, restore_join_meta, join_idx1);
        if (op == Join) {
            loadBinnedData(tableID2, source2, restore_join_meta, join_idx2);
        }
    }
    int structureID = 0;
    if (op == Select) {
        int queryStart = INT_MIN, queryEnd = INT_MAX, queryDim = 0;
        int numBins = 0;
        if (ds == Tables) {
            //* privtree
            extractMeta(global_eid, tableID1, sdata1, untrustedTableStorage[tableID1][0]->nometaSize(), sensitiveAttr1, sattrSize);
            delete []sdata1; delete []sensitiveAttr1;
            start_time = clock();
            sgx_return = createPDS(global_eid, &structureID, tableID1);
            /* start_time = clock();
            extractData(global_eid, tableID1, untrustedTableStorage[tableID1][0]->nometaSize()/untrustedTableStorage[tableID1][0]->getBlockSize());
            sgx_return = initHTree(global_eid, &structureID, (int)pfJoinFlag, constant, 1);
            sgx_return = createJoinBuckets(global_eid, structureID, tableID1, -1, (int)pfJoinFlag); */
            end_time = clock();
            usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
            cout << "createPDS for table "<< tablePath1 << " done in " << usedtime << "s" << endl;
            /* if (!select_simulation) {
                writeBinnedData(tableID1, source1);
                cout << "write binned data into file" << endl;
            } */
            const char *str = source1.c_str();
            buildIndex(global_eid, &numBins, structureID, queryDim, str, jt);
            cout << "built index" << endl;
        }
        if (ds == Bins) {
            char *idxBinStart, *idxBinEnd;
            uint size1, size2;
            loadIndex(structureID, idxBinStart, idxBinEnd, size1, size2, source1);
            restoreIndex(global_eid, &numBins, structureID, queryDim, idxBinStart, size1, idxBinEnd, size2);
        }
        //* start index select 1d for now
        char *s_bins = (char *)malloc(numBins*sizeof(int));
        char *s_records = (char *)malloc(sizeof(int));
        int examine_queries = 0;
        ansTable = 0;

        start_time = clock();

        /* for (uint q = 0; q < 100; q++) {
            queryStart = test_queries[q].first;
            queryEnd = test_queries[q].second;
            int numSelectedBins = 0;
            idxSelect(global_eid, &numSelectedBins, structureID, queryDim, queryStart, queryEnd, s_bins, numBins*sizeof(int), s_records, sizeof(int), jt);
            vector<int> Rsize((int*)s_records, (int*)s_records+1);
            vector<int> selectedBins((int*)s_bins, (int*)s_bins+numSelectedBins);
            // examine_queries += checkBlowup(tableID1, tablePath1,  Rsize[0], queryDim, queryStart, queryEnd);
            ansBins.clear();
            ansBins.insert(ansBins.end(), selectedBins.begin(), selectedBins.end());
            // toggle
            replyClient(ssl);
        }
        end_time = clock();
        usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
        // cout << "avg select response blowup " << avg_res_blowup/examine_queries << endl;
        cout << "total query runtime " << usedtime << " s" << endl;
        cout << "avg query runtime " << usedtime/100 << " s" << endl;
        avg_res_blowup = 0; examine_queries = 0;
        start_time = clock(); */
        
        for (uint q = 0; q < test_queries.size(); q++) {
            queryStart = test_queries[q].first;
            queryEnd = test_queries[q].second;
            int numSelectedBins = 0;
            idxSelect(global_eid, &numSelectedBins, structureID, queryDim, queryStart, queryEnd, s_bins, numBins*sizeof(int), s_records, sizeof(int), jt);
            vector<int> Rsize((int*)s_records, (int*)s_records+1);
            vector<int> selectedBins((int*)s_bins, (int*)s_bins+numSelectedBins);
            // cout << queryStart << " " << queryEnd << " selected records " << Rsize[0] << endl; 
            // examine_queries += checkSelectCorrectness(tableID1, tablePath1, queryDim, queryStart, queryEnd, &selectedBins);
            examine_queries += checkBlowup(tableID1, tablePath1,  Rsize[0], queryDim, queryStart, queryEnd);
            /* ansBins.clear();
            ansBins.insert(ansBins.end(), selectedBins.begin(), selectedBins.end());
            // toggle
            replyClient(ssl); */
        }
        end_time = clock();
        usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
        free(s_bins);
        free(s_records);
        if (examine_queries > 0) {
            cout << "avg select response blowup " << avg_res_blowup/examine_queries << endl;
        }
        cout << "total query runtime " << usedtime << " s" << endl;
        cout << "avg query runtime " << usedtime/100 << " s" << endl;
        cout << "select done..." << endl;
    }
    if (op == Join || op == SelectJoin) {
        if (ds == Tables) {
            if (jt == DO || jt == HTreeDO) {
                //* independent HTree and DO extractMeta
                /* int iter = 0;
                number start = 0, end = 0;
                end = min(untrustedTableStorage[tableID1][0]->nometaSize(), (number)(iter+1)*OCALL_MAXSIZE*EDATA_BLOCKSIZE);
                auto diter = sdata1;
                while (end <= untrustedTableStorage[tableID1][0]->nometaSize()) {
                    extractMeta(global_eid, tableID1, diter, end-start, sensitiveAttr1, sattrSize);
                    if (end == untrustedTableStorage[tableID1][0]->nometaSize()) {
                        break;
                    }
                    start = end;
                    diter = sdata1 + start;
                    iter += 1;
                    end = min(untrustedTableStorage[tableID1][0]->nometaSize(), (number)(iter+1)*OCALL_MAXSIZE*EDATA_BLOCKSIZE);
                }
                
                start = 0; iter = 0;
                end = min(untrustedTableStorage[tableID2][0]->nometaSize(), (number)(iter+1)*OCALL_MAXSIZE*EDATA_BLOCKSIZE);
                diter = sdata2;
                while (end <= untrustedTableStorage[tableID2][0]->nometaSize()) {
                    extractMeta(global_eid, tableID2, diter, end-start, sensitiveAttr2, sattrSize);
                    if (end == untrustedTableStorage[tableID2][0]->nometaSize()) {
                        break;
                    }
                    start = end;
                    diter = sdata2 + start;
                    iter += 1;
                    end = min(untrustedTableStorage[tableID2][0]->nometaSize(), (number)(iter+1)*OCALL_MAXSIZE*EDATA_BLOCKSIZE);
                }
                delete []sdata1; delete []sdata2;
                delete []sensitiveAttr1; delete []sensitiveAttr2; */
                if (jt == DO) {
                    int isSelectJoin = 0;
                    if (op == SelectJoin) {
                        isSelectJoin = 1;
                    }
                    start_time = clock();
                    sgx_return = initJoinPDS(global_eid, &structureID, (int)pfJoinFlag, isSelectJoin);
                    // start_time = clock();
                    if (isSelectJoin == 0) {
                        sgx_return = createJoinPDS(global_eid, structureID, tableID1, tableID2);
                    } else {
                        sgx_return = createSelectJoinPDS(global_eid, structureID, tableID1, tableID2);
                    }
                    
                } else {
                    start_time = clock();
                    //* one-time construction
                    extractData(global_eid, tableID1, untrustedTableStorage[tableID1][0]->nometaSize()/untrustedTableStorage[tableID1][0]->getBlockSize());
                    extractData(global_eid, tableID2, untrustedTableStorage[tableID2][0]->nometaSize()/untrustedTableStorage[tableID2][0]->getBlockSize());
                    
                    sgx_return = initHTree(global_eid, &structureID, (int)pfJoinFlag, constant, 0);
                    // start_time = clock();
                    // end_time = clock();
                    // usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
                    // cout << "init htree " << usedtime << "s" << endl;
                    // start_time = clock();
                    sgx_return = createJoinBuckets(global_eid, structureID, tableID1, tableID2, (int)pfJoinFlag);
                }
                
            } else if (jt == CZSC21) {
                cout << "create czsc21" << endl;
                start_time = clock();
                sgx_return = createCZSC(global_eid, tableID1, sdata1, untrustedTableStorage[tableID1][0]->nometaSize(), join_idx1, tableID2, sdata2, untrustedTableStorage[tableID2][0]->nometaSize(), join_idx2, (int)pfJoinFlag);
                cout << "end czsc21" << endl;
            }
            // if (!join_simulation) {
            //     writeBinnedData(tableID1, source1);
            //     writeBinnedData(tableID2, source2);
            // }
        }
        end_time = clock();
        usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
        cout << "join pds time " << usedtime << "s" << endl;
        //* SelectJoin, assume that there is only one predicate
        if (op == Join) {
            test_queries.push_back({INT_MIN, INT_MAX});
            // test_queries.push_back({1, 500});
        }
        int usSize = 0, writeID = untrustedTableStorage.size();
        for (int q = 0; q < test_queries.size(); q++) {
            start_time = clock();
            if (jt == DO || jt == HTreeDO) {
                if (!pfJoinFlag) {
                    int selectJoinAttr = selectIdx == 0 ? 1 : 0;
                    // int selectJoinAttr = 1;
                    innerJoin(global_eid, &usSize, structureID, tableID1, tableID2, writeID, jt, test_queries[q].first, test_queries[q].second, selectJoinAttr);
                } else {
                    // does not support select-join
                    if (q > 0) {
                        break;
                    }
                    pfJoin(global_eid, &usSize, structureID, tableID1, tableID2, writeID, jt);
                }
            } else if (jt == CZSC21) {
                // does not support select-join
                if (q > 0) {
                    break;
                }
                czscJoin(global_eid, &usSize, tableID1, tableID2, writeID, (int)pfJoinFlag);
            }
            end_time = clock();
            usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
            cout << "join time " << usedtime << "s" << endl;
            if (!join_simulation && !pfJoinFlag) {
                start_time = clock();
                //TODO: in enclave and out of enclave combined join compact
                joinCompact(global_eid, structureID, writeID, usSize, jt);
                end_time = clock();
                usedtime = (double)(end_time - start_time)/(CLOCKS_PER_SEC);
                cout << "join compact time " << usedtime << "s" << endl;
                if (usSize > 0) {
                    untrustedTableStorage[writeID][1] = untrustedBinStorage[1];
                    untrustedBinStorage.clear();
                }
                // ansBins.clear();
                // ansBins.push_back(1);
                // ansTable = 2;
                // toggle
                // replyClient(ssl);
            }
        }
    }
    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);
    printf("Info: Enclave successfully returned.\n");

    /* int sock;
    SSL_CTX *ctx;
    ctx = create_context();
    configure_context(ctx);
    sock = create_socket(port);
    // Handle connections 
    while(1) {
        struct sockaddr_in addr;
        unsigned int len = sizeof(addr);
        
        SSL *ssl;
        // const char reply[] = "send this sentense to client\n";

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            // send query response to client
            if (op == Select) {
                ansTable = 0;
            } else if (op == Join) {
                ansTable = 2; //hard coded
            }
            int reVolume = 0;
            for (int i = 0; i < ansBins.size(); i++) {
                printf("ansBins size %d, ansBins[%d]=%d\n", ansBins.size(), i, ansBins[i]);
                auto binStorage = untrustedTableStorage[ansTable][ansBins[i]];
                int dataSize = (int)(binStorage->size()/binStorage->getBlockSize()) - 1;
                number meta = binStorage->meta()+1;
                char reply[binStorage->getBlockSize()];
                for (int j = meta; j < meta+dataSize; j++) {
                    bytes response;
                    binStorage->get(j, response);
                    copy(response.begin(), response.end(), reply);
                    SSL_write(ssl, reply, strlen(reply));
                }
                reVolume += dataSize;
            }
            printf("%d records returned to client\n", reVolume);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
        break;
    } */
    // toggle
    /* SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client);
    close(sock);
    SSL_CTX_free(ctx);
    return 0; */
}

