#include "Enclave-utility.hpp"
#include "oblivious-blocks.hpp"
#include <cstring>
vector<int> jDistArray;

int crossProduct(TraceMem<tbytes> *tWorkSpace0, TraceMem<tbytes> *tWorkSpace1, int tableID1, int tableID2, int nreal1, int nreal2, int writeID, bool useOCALL, int qs, int qe, int jattr) {
    // printf("id1 %d id2 %d nreal1 %d nreal2 %d writeID %d\n", tableID1, tableID2, nreal1, nreal2, writeID);
    char *res, *iter;
    int t1_rid_pos = textSizeVec[tableID1] - sizeof(int)*2;
    int t2_rid_pos = textSizeVec[tableID2] - sizeof(int)*2;
    int textSize = t1_rid_pos + t2_rid_pos + sizeof(int);
    char *plaintext = new char[textSize];
    char *encryptedMatch = new char[EDATA_BLOCKSIZE];
    int markDummy = INT_MAX;
    int counter = 0;

    // bool useOCALL = (joinOutput->count + nreal1*nreal2) > 2*(int)IO_BLOCKSIZE;
    // printf("test: %d %d\n", joinOutput->count, (int)IO_BLOCKSIZE - (int)OCALL_MAXSIZE);
    // useOCALL = true;
    if (useOCALL) {
        res = (char*)malloc(min(nreal1*nreal2, OCALL_MAXSIZE) * EDATA_BLOCKSIZE);
        iter = res;
    }

    vector<vector<int>> t2_key_array;
    vector<int> t2_rid_array;
    for (int i = 0; i < nreal1; i++) {
        auto rawr1 = tWorkSpace0->read(i);
        bytes r1Bytes;
        for (int k = 0; k < sensitive_dim*sizeof(int); k++) {
            r1Bytes.push_back(rawr1[k]);
        }
        vector<int> r1Key = deconstructIntegers(tableID1, r1Bytes, sensitive_dim*sizeof(int));
        uchar r1_rid_array[sizeof(int)];
        for (int k = t1_rid_pos; k < t1_rid_pos+sizeof(int); k++) {
            r1_rid_array[k-t1_rid_pos] = rawr1[k];
        }
        // copy(r1Bytes.begin()+t1_rid_pos, r1Bytes.begin()+t1_rid_pos+sizeof(int), r1_rid_array);
        int r1_rid = vector<int>((int*)r1_rid_array, (int*)r1_rid_array+1)[0];
        for (int j = 0; j < nreal2; j++) {
            auto rawr2 = tWorkSpace1->read(j);
            if (i == 0) {
                bytes r2Bytes;
                for (int k = 0; k < sensitive_dim*sizeof(int); k++) {
                    r2Bytes.push_back(rawr2[k]);
                }
                vector<int> r2Key = deconstructIntegers(tableID2, r2Bytes, sensitive_dim*sizeof(int));
                t2_key_array.push_back(r2Key);
                uchar r2_rid_array[sizeof(int)];
                for (int k = t2_rid_pos; k < t2_rid_pos+sizeof(int); k++) {
                    r2_rid_array[k-t2_rid_pos] = rawr2[k];
                }
                int r2_rid = vector<int>((int*)r2_rid_array, (int*)r2_rid_array+1)[0];
                t2_rid_array.push_back(r2_rid);
            }
            if (r1_rid != INT_MAX && t2_rid_array[j] != INT_MAX && r1Key[0] == t2_key_array[j][0]) {
                //* r1Key[0] == t2_key_array[j][0] assume there is only one join attribute
                //* match
                int piter = 0;
                memcpy(plaintext, &realMatches, sizeof(int)); piter += sizeof(int);
                for (int k = 0; k < t1_rid_pos; k++) {
                    plaintext[piter] = rawr1[k];
                    piter++;
                }
                for (int k = 0; k < t2_rid_pos; k++) {
                    plaintext[piter] = rawr2[k]; 
                    piter++;
                }
                if (!useOCALL) {
                    int distance = jDistArray.size() - realMatches;
                    jDistArray.push_back(distance);
                    selected_count[selected_count_idx] = realMatches;
                    // nReal++; 
                    selected_count_idx++;
                }
                if (jattr == 1 && (r1Key[0] < qs || r1Key[0] > qe)) {
                    continue;
                } else if (jattr == 0 && (t2_key_array[j][1] < qs || t2_key_array[j][1] > qe)) {
                    continue;
                }
                realMatches++;
            } else {
                memcpy(plaintext, &markDummy, sizeof(int));
                memset(plaintext+sizeof(int), '\0', textSize-sizeof(int));
                if (!useOCALL) {
                    jDistArray.push_back(0);
                    selected_count[selected_count_idx] = realMatches;
                    selected_count_idx++;
                }
            }
            if (useOCALL) {
                encryptRecord(textSize, plaintext, encryptedMatch);
                memcpy(iter, encryptedMatch, EDATA_BLOCKSIZE);
                iter += EDATA_BLOCKSIZE;
                counter++;
                if (counter == OCALL_MAXSIZE) {
                    write_join_output_OCALL(writeID, res, OCALL_MAXSIZE*EDATA_BLOCKSIZE);
                    iter = res;
                    counter = 0;
                }   
            } else {
                // auto address = joinOutput->malloc();
                cbytes joinRow;
                for (int k = 0; k < textSize; k++) {
                    joinRow[k] = plaintext[k];
                }
                joinOutput->write(joinRow);
            }
        }
    }
    if (counter > 0) {
        write_join_output_OCALL(writeID, res, counter*EDATA_BLOCKSIZE);
    }
    free(res);
    delete []plaintext;
    delete []encryptedMatch;
    return realMatches;
}

int sortMergeJoin(TraceMem<pair<int, tbytes>> *binpair) {
    int pkTableID = 0, fkTableID = 1;
    ObliSortingNet<pair<int, tbytes>> net1 = ObliSortingNet<pair<int, tbytes>>(binpair, [](pair<int, tbytes> &r1, pair<int, tbytes> &r2) {
       uchar tmpInt[sizeof(int)];
        for (int k = 0; k < sizeof(int); k++) {
            tmpInt[k] = r1.second[k];
        }
        vector<int> r1Key = deconstructNumbers<int>(tmpInt, sizeof(int));
        for (int k = 0; k < sizeof(int); k++) {
            tmpInt[k] = r2.second[k];
        }
        vector<int> r2Key = deconstructNumbers<int>(tmpInt, sizeof(int));
        if (r1Key[0] == r2Key[0]) {
            // bitonic sort should be stable
            // we assume that foreign key table is the 2nd input table/right table
            return r1.first < r2.first;
        } else {
            return r1Key[0] < r2Key[0];
        }
    });
    net1.obliSort(binpair->size, true);
    uchar tmpInt[sizeof(int)];
    int preKey = INT_MIN;
    tbytes primary;
    cbytes empty, joinRow;
    int t2_rid_pos = textSizeVec[1] - sizeof(int)*2;
    uchar r2_rid_array[sizeof(int)];
    selected_count_idx = 0;
    int nReal = 0;
    for (int i = 0; i < binpair->size; i++) {
        auto curRow = binpair->read(i);
        for (int k = 0; k < sizeof(int); k++) {
            tmpInt[k] = curRow.second[k];
        }
        vector<int> curKey = deconstructNumbers<int>(tmpInt, sizeof(int));
        //* if curRow is from foreign key table
        if (curRow.first == 1 && curKey[0] == preKey) {
            for (int k = 0; k < textSizeVec[pkTableID]; k++) {
                joinRow[k] = primary[k];
            }
            for (int k = 0; k < textSizeVec[fkTableID]; k++) {
                joinRow[textSizeVec[pkTableID]+k] = curRow.second[k];
            }
            joinOutput->write(i, joinRow);
            int distance = jDistArray.size() - realMatches;
            jDistArray.push_back(distance);
            selected_count[selected_count_idx] = nReal;
            selected_count_idx++;
            //* for correctness check, can be removed
            copy(curRow.second.begin()+t2_rid_pos, curRow.second.begin()+t2_rid_pos+sizeof(int), r2_rid_array);
            int r2_rid = vector<int>((int*)r2_rid_array, (int*)r2_rid_array+1)[0];
            if (r2_rid != INT_MAX) {
                realMatches++;
                nReal++;
            }
        } else {
            for (int k = 0; k < textSizeVec[pkTableID]; k++) {
                empty[k] = '\0';
            }
            for (int k = 0; k < textSizeVec[fkTableID]; k++) {
                empty[textSizeVec[pkTableID]+k] = '\0';
            }
            joinOutput->write(i, empty);
            jDistArray.push_back(0);
            selected_count[selected_count_idx] = nReal;
            selected_count_idx++;
            if (curRow.first == 0) {
                //* if curRow is from primary key table
                preKey = curKey[0];
                primary = curRow.second;
            }
        }
    }
    return realMatches;
}

// Returns largest power of two less than N
uint64_t pow2_lt(uint64_t N) {
  uint64_t N1 = 1;
  while (N1 < N) {
    N1 <<= 1;
  }
  N1 >>= 1;
  return N1;
}


// Returns largest power of two greater than N
uint64_t pow2_gt(uint64_t N) {
  uint64_t N1 = 1;
  while (N1 < N) {
    N1 <<= 1;
  }
  return N1;
}

void parseInput(vector<metaRecord> &metaData, char *inputData, int inputSize, int tableID, vector<int> &attrIdx, vector<pair<int, int>> &initDomain) {
    int numData = inputSize / EDATA_BLOCKSIZE;
    // printf("numData %d\n", numData);
    if (initDomain.empty()) {
        initDomain.resize(attrIdx.size(), {INT_MAX, INT_MIN});
    }
    char *iter = inputData;

    uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    uchar *plaintext = new uchar[cipherSize];
    uchar *ciphertext = new uchar[cipherSize];
    uchar tag_in[TAG_SIZE];
    for (int i = 0; i < numData; i++) {
        memcpy(ciphertext, iter, cipherSize);
        iter += cipherSize;
        memcpy(tag_in, iter, TAG_SIZE);
        iter += (EDATA_BLOCKSIZE - cipherSize);

        sgx_status_t status = SGX_SUCCESS;
        status = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *) aes_key, (const uint8_t *) ciphertext,
                        cipherSize, (uint8_t *) plaintext, (const uint8_t *) hardcoded_iv, IV_LENGTH,
                        NULL, 0, (const sgx_aes_gcm_128bit_tag_t*) tag_in);
        if(status != SGX_SUCCESS) {
	        printf("Decrypt failed\n");
        }
        vector<int> structuredRecord = deconstructNumbers<int>(plaintext, textSizeVec[tableID]);
        metaRecord mr;
        mr.rid = i;
        for (uint j = 0; j < attrIdx.size(); j++) {
            auto attr_idx = attrIdx[j];
            initDomain[j].first = min(structuredRecord[attr_idx], initDomain[j].first);
            initDomain[j].second = max(structuredRecord[attr_idx], initDomain[j].second);

            mr.values.push_back(structuredRecord[attr_idx]);
        }
        metaData.push_back(mr);
    }
    // for (auto &&pr : initDomain) {
    //     printf("min %d max %d\n", pr.first, pr.second);
    // }
    delete []plaintext;
    delete []ciphertext;
}

int parseDataBlockPair(int tableID, TraceMem<pair<int, tbytes>> *decBlock, char *dblock, int start, int readDataSize, int dkeyMin) {
    uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    uchar *plaintext = new uchar[textSizeVec[tableID]];
    uchar *ciphertext = new uchar[cipherSize];
    uchar tag_in[TAG_SIZE];
    auto iter = dblock; int tmpcount = 0;
    int binid_pos = textSizeVec[tableID]-sizeof(int);
    pair<int, tbytes> record;
    uchar tmpInt[sizeof(int)];
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
        for (int j = 0; j < textSizeVec[tableID]; j++) {
            if (j < sizeof(int)) {
                tmpInt[j] = plaintext[j];
            }
            record.second[j] = plaintext[j];
        }
        vector<int> key = deconstructNumbers<int>(tmpInt, sizeof(int));
        record.first = key[0] - dkeyMin;
        decBlock->write(i, record);
    }
    delete []plaintext;
    delete []ciphertext;
    return tmpcount;
}

int readDataBlockPair_Wrapper(int ocallID, int tableID, int readBinID, int pos, TraceMem<pair<int,tbytes>> *plainBlock, int parsePos, int dnum, int dKeyMin) {
    int readFlag;
    char *dblock;
    if (plainBlock->size < parsePos+dnum) {
        plainBlock->resize(parsePos+dnum);
    }
    if (dnum <= OCALL_MAXSIZE) {
        dblock = (char*)malloc(dnum * EDATA_BLOCKSIZE);
        readDataBlock_OCALL(&readFlag, ocallID, readBinID, pos, dblock, dnum*EDATA_BLOCKSIZE);
        parseDataBlockPair(tableID, plainBlock, dblock, parsePos, dnum, dKeyMin);

    } else {
        int start = 0;
        dblock = (char*)malloc(OCALL_MAXSIZE * EDATA_BLOCKSIZE);
        while (start < dnum) {
            int readNum = min(dnum-start, (int)OCALL_MAXSIZE);
            readDataBlock_OCALL(&readFlag, ocallID, readBinID, pos+start, dblock, readNum*EDATA_BLOCKSIZE);
            parseDataBlockPair(tableID, plainBlock, dblock, parsePos+start, readNum, dKeyMin);
            start += readNum;
            // printf("size %d dnum %d start %d\n", plainBlock->size, dnum, start);
        }
    }
    free(dblock);
    return readFlag;
}

/* void encryptDataBlock(int tableID, TraceMem<tbytes> *plainBlock, char *dblock, int start, int writeDataSize) {
    uint cipherSize = computeCiphertextSize(textSizeVec[tableID]);
    // uchar emptytext[cipherSize];
    // memset(emptytext, '\0', cipherSize);
    uchar *plaintext = new uchar[cipherSize];
    uchar *reencrypted_text = new uchar[cipherSize];
    uchar tag_out[TAG_SIZE];
    // char *dblock_copy = new char[writeDataSize*EDATA_BLOCKSIZE];
    // auto iter_copy = dblock_copy;
    auto iter = dblock;
    sgx_status_t status = SGX_SUCCESS;
    for (int i = start; i < start+writeDataSize; i++) {
        auto record = plainBlock->read(i);
        // copy(plainBlock[i].begin(), plainBlock[i].begin()+cipherSize, plaintext);
        for (int j = 0; j < record.size(); j++) {
            plaintext[j] = record[j];
        }
        status = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *)aes_key, (const uint8_t *)plaintext, cipherSize,
				  (uint8_t *)reencrypted_text, (const uint8_t *)hardcoded_iv, IV_LENGTH, NULL, 0,
				  (sgx_aes_gcm_128bit_tag_t *)tag_out);
        if (status != SGX_SUCCESS) {
            printf("Encryption failed\n");
        }
        memcpy(iter, reencrypted_text, cipherSize);
        // memcpy(iter_copy, reencrypted_text, cipherSize);
        iter += cipherSize; //iter_copy += cipherSize;
        memcpy(iter, tag_out, TAG_SIZE);
        // memcpy(iter_copy, tag_out, TAG_SIZE);
        iter += (EDATA_BLOCKSIZE-cipherSize); //iter_copy += (EDATA_BLOCKSIZE-cipherSize);
    }
    // printf("check encrypt block size %d %d\n", iter-dblock, writeDataSize*EDATA_BLOCKSIZE);
    delete []plaintext;
    delete []reencrypted_text;
} */

void encryptRecord(uint textSize, char *input, char *&output) {
    uint cipherSize = computeCiphertextSize(textSize);
    uchar plaintext[cipherSize];
    memcpy(plaintext, input, textSize);
    // uchar *ciphertext = new uchar[cipherSize];
    // uchar tag_in[TAG_SIZE]; 
    uchar tag_out[TAG_SIZE];
    uchar reencrypted_text[cipherSize];
    // char *iter = input;
    // memcpy(ciphertext, iter, cipherSize);
    // iter += cipherSize;
    // memcpy(tag_in, iter, TAG_SIZE);
    // sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *) aes_key, (const uint8_t *) ciphertext,
    //                     cipherSize, (uint8_t *) plaintext, (const uint8_t *) hardcoded_iv, IV_LENGTH,
    //                     NULL, 0, (const sgx_aes_gcm_128bit_tag_t*) tag_in);
    sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *) aes_key, plaintext, cipherSize,
				  (uint8_t *) reencrypted_text, (const uint8_t *) hardcoded_iv, IV_LENGTH, NULL, 0,
				  (sgx_aes_gcm_128bit_tag_t *) tag_out);
    auto iter = output;
    memcpy(iter, reencrypted_text, cipherSize);
    iter += cipherSize;
    memcpy(iter, tag_out, TAG_SIZE);
}   

uint computeCiphertextSize(uint data_size){
    //Rounded up to nearest block size:
    uint encrypted_request_size;
    encrypted_request_size = ceil((float)data_size / (float)AES_GCM_BLOCK_SIZE_IN_BYTES);
    encrypted_request_size *= AES_GCM_BLOCK_SIZE_IN_BYTES;
    return encrypted_request_size;
}

double Lap(float eps, float sensitivity) {
    double lambda = (double)sensitivity/eps;
    int max = lambda*10;
    double* x = new double[4*max];
    double* pdf = new double[4*max];
    double start = -max;
    double step = 1.0 / 2;
    for (int i = 0; i < 4*max; i++) {
        x[i] = start;
        start += step;
    }
    for (int i = 0; i < 4*max; i++) {
        pdf[i] = 1/(2*lambda) * exp(-abs(x[i])/lambda);
    }

    double check = 0;
    for (int i = 0; i < 2*max; i++) {
        check += pdf[i];
    }
    for (int i = 2*max; i < 4*max; i++) {
        check += pdf[i];
    }

    // random_device rd;
    // default_random_engine eng(0);
    uniform_real_distribution<double> distr(0, check);
    double noise = 0;
    double r = distr(generator);
    double c = 0.0;
    for (int i = 0; i < 4*max; i++) {
        c += pdf[i];
        if (c >= r) {
            noise = x[i];
            break;
        }
    }
    if (c < r) {
        noise = x[4*max-1];
    }
    delete []x;
    delete []pdf;
    return noise;
}

int Geom(double eps, int rmax, int sensitivity) {
    int max = rmax+5;
    uniform_real_distribution<double> distr(0, 1);
    int noise = 0;
    double r = distr(generator);
    double alpha = exp(eps);

    // int* x = new int[max];
    // double* pdf = new double[max];
    // for (int i = 0; i < max; i++) {
    //     pdf[i] = (alpha-1)/(alpha+1) * pow(alpha, -abs(x[i]));
    // }
    // double c = 0.0;
    // for (int i = 0; i < 2*max; i++) {
    //     c += pdf[i];
    //     if (c >= r) {
    //         noise = x[i];
    //         break;
    //     }
    // }
    // if (c < r) {
    //     for (int i = max; i < 2*max; i++) {
    //         x[i-max] = i - max;
    //     }
    //     for (int i = 0; i < max; i++) {
    //         pdf[i] = (alpha-1)/(alpha+1) * pow(alpha, -abs(x[i]));
    //     }
    //     for (int i = 0; i < max; i++) {
    //         c += pdf[i];
    //         if (c >= r) {
    //             noise = x[i];
    //             break;
    //         }
    //     }
    // }
    // if (c < r) {
    //     noise = x[2*max-1];
    // }

    // int* x = new int[2*max];
    // double* pdf = new double[2*max];
    // for (int i = max; i > 0; i--) {
    //     x[max-i] = -i;
    // }
    // for (int i = max; i < 2*max; i++) {
    //     x[i] = i - max;
    // }
    // for (int i = 0; i < 2*max; i++) {
    //     int x = i - max;
    //     pdf[i] = (alpha-1)/(alpha+1) * pow(alpha, -abs(x));
    //     // pdf[i] = (alpha-1)/(alpha+1) * pow(alpha, -abs(x[i]));
    // }

    double c = 0.0;
    for (int i = 0; i < 2*max; i++) {
        int x = i - max;
        double pdf = (alpha-1)/(alpha+1) * pow(alpha, -abs(x));
        // c += pdf[i];
        c += pdf;
        if (c >= r) {
            // noise = x[i];
            noise = i - max;
            break;
        }
    }
    if (c < r) {
        // noise = x[2*max-1];
        noise = max - 1;
    }
    // delete []x;
    // delete []pdf;
    return noise;
}

int TGeom(double eps, double delta, int sensitivity, bool shifted) {
    int k0 = sensitivity/eps * log(2.0/delta);
    int mean = k0 + sensitivity - 1;
    int noise = Geom(eps, mean, sensitivity);
    int tnoise;
    if (shifted) {
        tnoise = min(max(0, mean+noise), 2*mean);
    } else {
        tnoise = min(max(-mean, noise), mean);
    }
    return tnoise;
}

void sortedVecDifference(vector<int> &parentSet, vector<int> &subset, vector<int> &resultSet) {
    auto it1 = parentSet.begin(); auto it2 = subset.begin();
    while (it1 != parentSet.end() && it2 != subset.end()) {
        if (*it1 == *it2) {
            it1++;
            it2++;
        } else if (*it1 < *it2) {
            resultSet.push_back(*it1);
            it1++;
        } else {
            printf("%d %d\n", *it1, *it2);
            break;
        }
    }
    while (it1 != parentSet.end()) {
        resultSet.push_back(*it1);
        it1++;
    }
}

/* int readDataBlock_Wrapper(int ocallID, int tableID, int readBinID, uint pos, TraceMem<tbytes> *plainBlock, int dnum) {
    int readFlag;
    char *dblock;
    plainBlock->resize(dnum);
    if (dnum <= OCALL_MAXSIZE) {
        dblock = (char*)malloc(dnum * EDATA_BLOCKSIZE);
        readDataBlock_OCALL(&readFlag, ocallID, readBinID, pos, dblock, dnum*EDATA_BLOCKSIZE);
        parseDataBlock(tableID, plainBlock, dblock, 0, dnum);
    } else {
        int start = 0;
        dblock = (char*)malloc(OCALL_MAXSIZE * EDATA_BLOCKSIZE);
        // char *tmp_dblock = dblock;
        while (start < dnum) {
            int readNum = min(dnum-start, (int)OCALL_MAXSIZE);
            readDataBlock_OCALL(&readFlag, ocallID, readBinID, pos+start, dblock, readNum*EDATA_BLOCKSIZE);
            parseDataBlock(tableID, plainBlock, dblock, start, readNum);
            start += readNum;
            // tmp_dblock += (readNum*EDATA_BLOCKSIZE);
        }
    }
    free(dblock);
    return readFlag;
} */

/* void writeDataBlock_Wrapper(int tableID, int writeBinID, uint pos, TraceMem<tbytes> *plainBlock, int dnum) {
    char *dblock;
    if (dnum <= OCALL_MAXSIZE) {
        dblock = (char*)malloc(dnum * EDATA_BLOCKSIZE);
        encryptDataBlock(tableID, plainBlock, dblock, 0, dnum);
        writeDataBlock_OCALL(tableID, writeBinID, pos, dblock, dnum*EDATA_BLOCKSIZE);
    } else {
        int start = 0;
        dblock = (char*)malloc(OCALL_MAXSIZE * EDATA_BLOCKSIZE);
        // char *tmp_dblock = dblock;
        while (start < dnum) {
            int writeNum = min(dnum-start, (int)OCALL_MAXSIZE);
            encryptDataBlock(tableID, plainBlock, dblock, start, writeNum);
            writeDataBlock_OCALL(tableID, writeBinID, pos+start, dblock, writeNum*EDATA_BLOCKSIZE);
            start += writeNum;
            // tmp_dblock += (writeNum*EDATA_BLOCKSIZE);
        }
    }
    free(dblock);
} */

vector<number> deconstructNumbers(const bytes &data)
{
    auto count = data.size() / sizeof(number);

    uchar buffer[count * sizeof(number)];
    copy(data.begin(), data.end(), buffer);
    return vector<number>((number *)buffer, (number *)buffer + count);
}

//TODO deconstruct sensitive int values from a record (data)
vector<int> deconstructIntegers(int tableID, const bytes &data, int stop)
{
    if (stop == -1) {
        stop = data.size();
    }
    auto count = stop / sizeof(int);
    uchar buffer[stop];
    copy(data.begin(), data.begin()+stop, buffer);
    return vector<int>((int *)buffer, (int *)buffer + count); 
}

template<typename T>
vector<T> deconstructNumbers(uchar *data, int dataSize) {
    auto count = dataSize / sizeof(T);
    return vector<T>((T *)data, (T *)data + count);
}

bytes concat(int count, ...)
{
    va_list args;
    va_start(args, count);

    bytes result;
    for (int i = 0; i < count; i++)
    {
        bytes *vec = va_arg(args, bytes *);
        result.insert(result.end(), vec->begin(), vec->end());
    }
    va_end(args);

    return result;
}

bytes concatNumbers(int count, ...)
{
    va_list args;
    va_start(args, count);

    number numbers[count];

    bytes result;
    for (int i = 0; i < count; i++)
    {
        numbers[i] = va_arg(args, number);
    }
    va_end(args);

    return bytes((uchar *)numbers, (uchar *)numbers + count * sizeof(number));
}

bytes bytesFromNumber(number num)
{
    number material[1] = {num};
    return bytes((uchar *)material, (uchar *)material + sizeof(number));
}

bytes bytesFromInteger(int num) {
    int material[1] = {num};
    return bytes((uchar *)material, (uchar *)material + sizeof(int));
}

number numberFromBytes(const bytes &data)
{
    uchar buffer[sizeof(number)];
    copy(data.begin(), data.begin()+sizeof(number), buffer);
    return ((number *)buffer)[0];
}

vector<bytes> deconstruct(const bytes &data, vector<int> stops)
{
    vector<bytes> result;
    result.resize(stops.size() + 1);
    for (uint i = 0; i <= stops.size(); i++)
    {
        bytes buffer(
            data.begin() + (i == 0 ? 0 : stops[i - 1]),
            data.begin() + (i == stops.size() ? data.size() : stops[i]));

        result[i] = buffer;
    }

    return result;
}


void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void reverse(char *s) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char *s, int *len) {
    int i = 0, sign;

    if ((sign = n) < 0) {
        n = -n;
        i = 0;
    }
        
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    
    *len = i;
    
    reverse(s);
}