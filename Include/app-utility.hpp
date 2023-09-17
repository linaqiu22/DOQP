#include "CONFIG.hpp"
#include "type_def.hpp"
#include "app-storage.hpp"
#include <memory>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
using namespace std;
#pragma once

// load and encrypt
void loadTable(int tableID, shared_ptr<AbsStorage> storage, string source, vector<vector<int>> &test_data);
void loadCustomer(int tableID, shared_ptr<AbsStorage> storage, string source);
void loadSupplier(int tableID, shared_ptr<AbsStorage> storage, string source);

//* cryptography 
void handleErrors(void);
int aes_gcm_128_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *aad, int aad_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext,
                unsigned char *tag);
int aes_gcm_128_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *aad, int aad_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext);

uint computeCiphertextSize(uint data_size);

//* others
void concatIntegers(char *text, int count, ...);
bytes bytesFromNumber(number num);

