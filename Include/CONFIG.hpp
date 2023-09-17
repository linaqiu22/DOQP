#include <random>
#include <string>
#include <cstring>
using namespace std;
#pragma once

#define AES_GCM_BLOCK_SIZE_IN_BYTES 16
#define KEY_LENGTH 16
#define IV_LENGTH 12
#define TAG_SIZE 16
#define IO_BLOCKSIZE 200000000 
#define OCALL_MAXSIZE 15000
// #define COMPACT_BLOCKSIZE 9000

#define TC_PRECOMPUTE_COUNTS 1 // From CCS 22 ORCompact
#define TC_OPT_SWAP_FLAG 1 // From CCS 22 ORCompact

#define K 16 // HTree branching factor

static unsigned char aes_key[KEY_LENGTH] = {"AAAAAAAAAAAAAAA"};
static unsigned char hardcoded_iv[IV_LENGTH] = {"AAAAAAAAAAA"};

static bool join_simulation = false;
static bool select_simulation = false;
static bool czsc_optimized = true;
const int sensitive_dim = 4;


const double eps_d = 0.02;
const double eps_r = 0.26;
const double eps_c = 0.02;
static double delta_r;
static double delta_c;
const int M_BLOCKSIZE = 48; // index data block size in enclave
const float SF = 1; // tpc-h scale factor

//* synthetic data
const int EDATA_BLOCKSIZE = 512;
// const vector<unsigned int> textSizeVec{2*sizeof(int)+2*sizeof(int)}; //* synthetic data select 
// const vector<unsigned int> textSizeVec{2*sizeof(int)+2*sizeof(int), 2*sizeof(int)+2*sizeof(int), 5*sizeof(int)}; //* synthetic data join (key_atttr, val_attr, id, binID)

//* TPC-H
// const int EDATA_BLOCKSIZE = 512; // tpc-h
// const vector<unsigned int> textSizeVec{193+2*sizeof(int), 219+2*sizeof(int), sizeof(int)+412}; //* TPC-H join Supplier, Customer
const vector<unsigned int> textSizeVec{193+2*sizeof(int), 193+2*sizeof(int), sizeof(int)+386}; //* TPC-H join Supplier, Supplier
// const vector<unsigned int> textSizeVec{219+2*sizeof(int), 219+2*sizeof(int), sizeof(int)+438}; //* TPC-H join Customer, Customer

class Exception : public exception
{
    public:
    explicit Exception(const char* message) :
        msg_(message)
    {
    }

    explicit Exception(const string& message) :
        msg_(message)
    {
    }
    
    virtual ~Exception() throw() {}

    virtual const char* what() const throw()
    {
        return msg_.c_str();
    }

    protected:
    string msg_;
};
static random_device rd;
static default_random_engine generator(0); // fixed seed that removes randomness for debug purpose


// TODO REMOVE
static float constant;