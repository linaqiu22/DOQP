#include "app-utility.hpp"
#include <iostream>
#include <algorithm>

//* synthetic data
void loadTable(int tableID, shared_ptr<AbsStorage> storage, string source, vector<vector<int>> &test_data) {
    ifstream in_file;
    in_file.open(source);
    if (!in_file.is_open()) {
        cout << "Error opening table input file " << source << endl;
        exit(0);
    }
    int n_data, n_attr;
    in_file >> n_data >> n_attr;
    float attrVal;
    int textSize = textSizeVec[tableID];
    number cipherSize = computeCiphertextSize(textSize);
    uchar plaintext[cipherSize];
    uchar ciphertext[cipherSize];
    uchar tag[TAG_SIZE];

    for (int i = 0; i < n_data; i++) {
        uchar *iter = plaintext;
        vector<int> in_record;
        for (int j = 0; j < n_attr; j++) {
            in_file >> attrVal;
            int intAttrVal = (int)attrVal;
            in_record.push_back(intAttrVal);
            if (storage !=  nullptr) {
                memcpy(iter, &intAttrVal, sizeof(int));
                iter += sizeof(int);
            }
        }
        test_data.push_back(in_record);
        if (storage != nullptr) {
            memcpy(iter, &i, sizeof(int));
            //* cipherSize to textSize
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
}

//* tpc-h customer
void loadCustomer(int tableID, shared_ptr<AbsStorage> storage, string source) {
    ifstream in_file;
    in_file.open(source);
    if (!in_file.is_open()) {
        cout << "Error opening table input file " << source << endl;
        exit(0);
    }
    int textSize = textSizeVec[tableID];
    number cipherSize = computeCiphertextSize(textSize);
    uchar plaintext[cipherSize];
    uchar ciphertext[cipherSize];
    uchar tag[TAG_SIZE];
    int n_data = SF * 150000;
    string line, element;
    int C_NATIONKEY;
    int start, end;
    bytes values;
    for (int i = 0; i < n_data; i++) {
        auto iter = plaintext;
        getline(in_file, line);
        start = 0;
        values.clear();
        for (int j = 0; j < 8; j++) {
            end = line.find("|", start);
            element = line.substr(start, end-start);
            if (j == 3) {
                C_NATIONKEY = stoi(element);
            } else if (j == 0 || j == 5) {
                if (j == 0) {
                    int C_CUSTKEY = stoi(element);
                    uchar tmp[sizeof(int)];
                    memcpy(tmp, &C_CUSTKEY, sizeof(int));
                    values.insert(values.end(), tmp, tmp+sizeof(int));
                } else {
                    float C_ACCTBAL = stof(element);
                    uchar tmp[sizeof(float)];
                    memcpy(tmp, &C_ACCTBAL, sizeof(float));
                    copy(tmp, tmp+sizeof(float), values.end());
                }
            } else {
                values.insert(values.end(), element.begin(), element.end());
            }
            start = end + 1;
        }
        if (storage != nullptr) {
            memcpy(iter, &C_NATIONKEY, sizeof(int)); iter += sizeof(int);
            copy(values.begin(), values.end(), iter); iter += (textSizeVec[tableID]-3*sizeof(int));
            memcpy(iter, &i, sizeof(int));
            //* cipherSize to textSize
            int ciphertext_len = aes_gcm_128_encrypt(plaintext, cipherSize, nullptr, 0,
                                                 aes_key, hardcoded_iv, IV_LENGTH, ciphertext, tag);
            bytes record(ciphertext, ciphertext+cipherSize);
            record.insert(record.end(), tag, tag+TAG_SIZE);      
            record.resize(storage->getBlockSize());
            auto address = storage->malloc();
            storage->set(address, record);
        }
    }
    in_file.close();
}

void loadSupplier(int tableID, shared_ptr<AbsStorage> storage, string source) {
    ifstream in_file;
    in_file.open(source);
    if (!in_file.is_open()) {
        cout << "Error opening table input file " << source << endl;
        exit(0);
    }
    int textSize = textSizeVec[tableID];
    number cipherSize = computeCiphertextSize(textSize);
    uchar plaintext[cipherSize];
    uchar ciphertext[cipherSize];
    uchar tag[TAG_SIZE];
    int n_data = SF * 10000;
    string line, element;
    int S_NATIONKEY;
    int start, end;
    bytes values;
    for (int i = 0; i < n_data; i++) {
        auto iter = plaintext;
        getline(in_file, line);
        start = 0;
        values.clear();
        for (int j = 0; j < 7; j++) {
            end = line.find("|", start);
            element = line.substr(start, end-start);
            if (j == 3) {
                S_NATIONKEY = stoi(element);
            } else if (j == 0 || j == 5){
                if (j == 0) {
                    int S_SUPPKEY = stoi(element);
                    uchar tmp[sizeof(int)];
                    memcpy(tmp, &S_SUPPKEY, sizeof(int));
                    values.insert(values.end(), tmp, tmp+sizeof(int));
                } else {
                    float S_ACCTBAL = stof(element);
                    uchar tmp[sizeof(float)];
                    memcpy(tmp, &S_ACCTBAL, sizeof(float));
                    copy(tmp, tmp+sizeof(float), values.end());
                }
            } else {
                values.insert(values.end(), element.begin(), element.end());
            }
            start = end + 1;
        }
        if (storage != nullptr) {
            memcpy(iter, &S_NATIONKEY, sizeof(int)); iter += sizeof(int);
            copy(values.begin(), values.end(), iter); iter += (textSizeVec[tableID]-3*sizeof(int));
            memcpy(iter, &i, sizeof(int));
            //* cipherSize to textSize
            int ciphertext_len = aes_gcm_128_encrypt(plaintext, cipherSize, nullptr, 0,
                                                 aes_key, hardcoded_iv, IV_LENGTH, ciphertext, tag);
            bytes record(ciphertext, ciphertext+cipherSize);
            record.insert(record.end(), tag, tag+TAG_SIZE);      
            record.resize(storage->getBlockSize());
            auto address = storage->malloc();
            storage->set(address, record);
        }
    }
    in_file.close();
}

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}


int aes_gcm_128_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *aad, int aad_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext,
                unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();

    /*
     * Set IV length if default 12 bytes (96 bits) is not appropriate
     */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        handleErrors();

    /* Initialise key and IV */
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    /* if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors(); */

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Get the tag */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag))
        handleErrors();

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}


int aes_gcm_128_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *aad, int aad_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. */
    if(!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
        handleErrors();

    /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        handleErrors();

    /* Initialise key and IV */
    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    /* if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors(); */

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag))
        handleErrors();

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0) {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    } else {
        /* Verify failed */
        return -1;
    }
}

uint computeCiphertextSize(uint data_size){
    //Rounded up to nearest block size:
    uint encrypted_request_size;
    encrypted_request_size = ceil((float)data_size / (float)AES_GCM_BLOCK_SIZE_IN_BYTES);
    encrypted_request_size *= 16;
    return encrypted_request_size;
}

void concatIntegers(char *text, int count, ...) {
    va_list args;
    va_start(args, count);

    int numbers[count];

    bytes result;
    for (int i = 0; i < count; i++)
    {
        numbers[i] = va_arg(args, int);
    }
    va_end(args);
    memcpy(text, (char *)&numbers, count*sizeof(int));
}

bytes bytesFromNumber(number num)
{
    number material[1] = {num};
    return bytes((uchar *)material, (uchar *)material + sizeof(number));
}
