#include "CONFIG.hpp"
#include "type_def.hpp"
#include "storage.hpp"
#include "trace_mem.h"
#include "Enclave_t.h"  /* print string */
#include <assert.h> /* assert */
#include <sgx_tcrypto.h>
#pragma once

void printf(const char *fmt, ...);

//* join cross product
extern TraceMem<cbytes> *joinOutput;
extern uint32_t *selected_count;
extern int selected_count_idx;
extern int realMatches;
int crossProduct(TraceMem<tbytes> *tWorkSpace0, TraceMem<tbytes> *tWorkSpace1, int tableID1, int tableID2, int nreal1, int nreal2, int writeID, bool useOCALL, int qs=INT_MIN, int qe=INT_MAX, int jattr=1);
int sortMergeJoin(TraceMem<pair<int, tbytes>> *binpair);

//* ORCompact
// Returns largest power of two less than N
uint64_t pow2_lt(uint64_t N);
// Returns largest power of two greater than N
uint64_t pow2_gt(uint64_t N);

//* cryptography
uint computeCiphertextSize(uint data_size);
void parseInput(vector<metaRecord> &metaData, char *inputData, int inputSize, int tableID, vector<int> &attrIdx, vector<pair<int, int>> &initDomain);

template<typename T>
vector<T> deconstructNumbers(uchar *data, int dataSize);

int parseDataBlockPair(int tableID, TraceMem<pair<int, tbytes>> *decBlock, char *dblock, int start, int readDataSize, int dkeyMin);

template <typename T>
int parseDataBlock(int tableID, TraceMem<T> *decBlock, char *dblock, int start, int readDataSize, vector<KeyBin> *keyBinList=nullptr) {
    uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    uchar *plaintext = new uchar[textSizeVec[tableID]];
    uchar *ciphertext = new uchar[cipherSize];
    uchar tag_in[TAG_SIZE];
    auto iter = dblock; int tmpcount = 0;
    int binid_pos = textSizeVec[tableID]-sizeof(int);
    for (int i = start; i < start+readDataSize; i++) {
        memcpy(ciphertext, iter, cipherSize);
        iter += cipherSize;
        memcpy(tag_in, iter, TAG_SIZE);
        iter += (EDATA_BLOCKSIZE - cipherSize);

        sgx_status_t status = SGX_SUCCESS;
        status = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *) aes_key, (const uint8_t *) ciphertext,
                        cipherSize, (uint8_t *) plaintext, (const uint8_t *) hardcoded_iv, IV_LENGTH,
                        NULL, 0, (const sgx_aes_gcm_128bit_tag_t*) tag_in);
        if (status != SGX_SUCCESS) {
            tmpcount++;
            memset(plaintext, '\0', textSizeVec[tableID]);
        } 
        T record;
        for (int j = 0; j < textSizeVec[tableID]; j++) {
            record[j] = plaintext[j];
        }
        if (keyBinList != nullptr) {
            uchar tmpInt[sizeof(int)];
            vector<int> key = deconstructNumbers<int>(plaintext, sensitive_dim*sizeof(int));
            for (int j = 0; j < keyBinList->size(); j++) {
                if (keyBinList->at(j).key == key) {
                    memcpy(tmpInt, &keyBinList->at(j).bin_id, sizeof(int));
                    for (int k = 0; k < sizeof(int); k++) {
                        record[binid_pos+k] = tmpInt[k];
                    }
                }
            }
        }
        decBlock->write(i, record);
    }
    delete []plaintext;
    delete []ciphertext;
    return tmpcount;
}

template <typename T>
void encryptDataBlock(int tableID, TraceMem<T> *plainBlock, char *dblock, int start, int writeDataSize=IO_BLOCKSIZE) {
    uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    uchar *plaintext = new uchar[cipherSize];
    uchar *reencrypted_text = new uchar[cipherSize];
    uchar tag_out[TAG_SIZE];
    auto iter = dblock;
    sgx_status_t status = SGX_SUCCESS;
    for (int i = start; i < start+writeDataSize; i++) {
        auto record = plainBlock->read(i);
        for (int j = 0; j < textSizeVec[tableID]; j++) {
            plaintext[j] = record[j];
        }
        status = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *)aes_key, (const uint8_t *)plaintext, cipherSize,
				  (uint8_t *)reencrypted_text, (const uint8_t *)hardcoded_iv, IV_LENGTH, NULL, 0,
				  (sgx_aes_gcm_128bit_tag_t *)tag_out);
        if (status != SGX_SUCCESS) {
            printf("Encryption failed\n");
        }
        memcpy(iter, reencrypted_text, cipherSize);
        iter += cipherSize; 
        memcpy(iter, tag_out, TAG_SIZE);
        iter += (EDATA_BLOCKSIZE-cipherSize);
    }
    delete []plaintext;
    delete []reencrypted_text;
}

void encryptRecord(uint textSize, char *input, char *&output);

//* others
double Lap(float eps, float sensitivity=1.0);
int Geom(double eps, int rmax, int sensitivity=1);
int TGeom(double eps, double delta, int sensitivity, bool shift=true);
void sortedVecDifference(vector<int> &parentSet, vector<int> &subset, vector<int> &resultSet);
int readDataBlockPair_Wrapper(int ocallID, int tableID, int readBinID, int pos, TraceMem<pair<int,tbytes>> *plainBlock, int parsePos, int dnum, int dkeyMin=0);

template <typename T>
int readDataBlock_Wrapper(int ocallID, int tableID, int readBinID, int pos, TraceMem<T> *plainBlock, int parsePos, int dnum, vector<KeyBin> *keyBinList=nullptr) {
    int readFlag;
    char *dblock;
    if (plainBlock->size < parsePos+dnum) {
        plainBlock->resize(parsePos+dnum);
    }
    if (dnum <= OCALL_MAXSIZE) {
        dblock = (char*)malloc(dnum * EDATA_BLOCKSIZE);
        readDataBlock_OCALL(&readFlag, ocallID, readBinID, pos, dblock, dnum*EDATA_BLOCKSIZE);
        parseDataBlock(tableID, plainBlock, dblock, parsePos, dnum, keyBinList);
        
    } else {
        int start = 0;
        dblock = (char*)malloc(OCALL_MAXSIZE * EDATA_BLOCKSIZE);
        while (start < dnum) {
            int readNum = min(dnum-start, (int)OCALL_MAXSIZE);
            readDataBlock_OCALL(&readFlag, ocallID, readBinID, pos+start, dblock, readNum*EDATA_BLOCKSIZE);
            parseDataBlock(tableID, plainBlock, dblock, parsePos+start, readNum, keyBinList);
            start += readNum;
        }
    }
    free(dblock);
    return readFlag;
}

template <typename T>
void writeDataBlock_Wrapper(int tableID, int writeBinID, int pos, TraceMem<T> *plainBlock, int dnum) {
    char *dblock;
    if (dnum <= OCALL_MAXSIZE) {
        dblock = (char*)malloc(dnum * EDATA_BLOCKSIZE);
        encryptDataBlock<T>(tableID, plainBlock, dblock, 0, dnum);
        writeDataBlock_OCALL(tableID, writeBinID, pos, dblock, dnum*EDATA_BLOCKSIZE);
    } else {
        int start = 0;
        dblock = (char*)malloc(OCALL_MAXSIZE * EDATA_BLOCKSIZE);
        while (start < dnum) {
            int writeNum = min(dnum-start, (int)OCALL_MAXSIZE);
            encryptDataBlock<T>(tableID, plainBlock, dblock, start, writeNum);
            writeDataBlock_OCALL(tableID, writeBinID, pos+start, dblock, writeNum*EDATA_BLOCKSIZE);
            start += writeNum;
        }
    }
    free(dblock);
}



/**
 * @brief concatenates the vectors of bytes to a single vector of bytes
 *
 * @param count the number of inputs
 * @param ... the POINTERS to the vectors
 * @return bytes the single concatenated vector
 */
bytes concat(int count, ...);

/**
 * @brief splits the vector of bytes into subvectors defines by split points
 *
 * @param data the vector to split
 * @param stops the offsets to use as split points
 * @return vector<bytes> the resulting sub-vectors
 */
vector<bytes> deconstruct(const bytes &data, vector<int> stops);

/**
 * @brief concatenates numbers into one vector of bytes
 *
 * @param count the number of inputs
 * @param ... the numbers to concat
 * @return bytes the resulting vector
 */
bytes concatNumbers(int count, ...);

/**
 * @brief deconstruct the vector of byte back to numbers
 *
 * @param data the vector of bytes (usually created with concatNumbers)
 * @return vector<number> the original numbers
 */
vector<number> deconstructNumbers(const bytes &data);

//TODO adapt this to more complicated input
vector<int> deconstructIntegers(int tableID, const bytes &data, int stop=-1);

/**
 * @brief converts a number to bytes
 *
 * @param num the number to convert
 * @return bytes the resulting bytes
 */
bytes bytesFromNumber(number num);
bytes bytesFromInteger(int num);

/**
 * @brief converts bytes back to number
 *
 * @param data the bytes to convert
 * @return number the resulting number
 */
number numberFromBytes(const bytes &data);
void reverse(char *s);
void itoa(int n, char *s, int *len);