#include "CONFIG.hpp"
#include <vector>
#include <array>
using namespace std;
#pragma once

using number = unsigned long long;
using uchar  = unsigned char;
using uint   = unsigned int;
using bytes  = vector<uchar>;

using IO_BLOCK_BYTES = vector<bytes>;
// the maximum number of bytes of each record
using tbytes = array<uchar, 128*sizeof(int)>;
// the maximum number of bytes per record in compaction
using cbytes = array<uchar, 128*sizeof(int)>;

#ifndef _query_type
#define _query_type
typedef enum query_type {
    Select,
    Join,
    SelectJoin
} query_type;
#endif

#ifndef _data_source
#define _data_source
typedef enum data_source {
    Tables,
    Bins
} data_source;
#endif

#ifndef _join_type
#define _join_type
typedef enum join_type {
    CZSC21,
    DO,
    HTreeDO
} join_type;
#endif

#ifndef _key_bin
#define _key_bin
typedef struct {
    vector<int> key;
    int count1;
    int noise1;
    int count2;
    int noise2;
    int bin_id;
} KeyBin;
#endif

typedef struct {
    int rid;
    vector<int> values;
} metaRecord;

typedef vector<uint> Bin; // group of rid

