// #include "CONFIG.hpp"
// #include "trace_mem.h"
// #include "type_def.hpp"
// #include <unordered_map>
// #pragma once

// #ifndef _key_list_entry
// #define _key_list_entry
// typedef struct Entry {
//     number n1; // noisy count of key multiplicity in table 1
//     number n2; // noisy count of key multiplicity in table 2
//     number key;
//     number r1; // real count of key multiplicity in table 1
//     number r2; // real count of key multiplicity in table 2
// } Entry;
// #endif

// class Client {
//     private:
//     number keyMax; //* used to divide valid/dummy keys
//     Predicate predicate;

//     public:
//     shared_ptr<Table> T1, T2;
//     vector<Entry> completeKeyList;
//     number matches = 0;
//     number inSize = 0;
//     char *output;
//     char *output_iter;

//     vector<vector<Sanitizer>> rangeSanitizer;
//     unordered_map<number, Sanitizer> *pointSanitizer;


//     Client(Predicate predicate);

//     number loadTables(string inputPath);
//     void preprocess();
//     void populatePointNoise(vector<Entry> &completeKeyList, int nAddedKeys);
//     void populateRangeNoise(int size);
//     number getInput(char *inputData);
//     number getKeyList(char *keyPos);
//     void getDPIdx(char *dpIdx, number &dpIdxSize, number &pSanitizerSize1, number &pSanitizerSize2);

// };
