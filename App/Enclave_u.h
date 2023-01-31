#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */

#include "user_types.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _struct_foo_t
#define _struct_foo_t
typedef struct struct_foo_t {
	uint32_t struct_foo_0;
	uint64_t struct_foo_1;
} struct_foo_t;
#endif

typedef enum enum_foo_t {
	ENUM_FOO_0 = 0,
	ENUM_FOO_1 = 1,
} enum_foo_t;

#ifndef _union_foo_t
#define _union_foo_t
typedef union union_foo_t {
	uint32_t union_foo_0;
	uint32_t union_foo_1;
	uint64_t union_foo_3;
} union_foo_t;
#endif

#ifndef OCALL_PRINT_STRING_DEFINED__
#define OCALL_PRINT_STRING_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_string, (const char* str));
#endif
#ifndef EXPAND_TABLE_OCALL_DEFINED__
#define EXPAND_TABLE_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, expand_table_OCALL, (int tableID, uint noiseSum, char* noise, uint nsize));
#endif
#ifndef READDATABLOCK_OCALL_DEFINED__
#define READDATABLOCK_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_NOCONVENTION, readDataBlock_OCALL, (int tableID, int readBinID, int pos, char* dblock, int dsize));
#endif
#ifndef WRITEDATABLOCK_OCALL_DEFINED__
#define WRITEDATABLOCK_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, writeDataBlock_OCALL, (int tableID, int writeBinID, uint pos, char* dblock, uint dsize));
#endif
#ifndef COPY_BIN_OCALL_DEFINED__
#define COPY_BIN_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, copy_bin_OCALL, (int tableID, int readBinID, int writeBinID, int startPos, int endPos));
#endif
#ifndef MOVE_RESIZE_BIN_OCALL_DEFINED__
#define MOVE_RESIZE_BIN_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, move_resize_bin_OCALL, (int tableID, int srcBinID, int destBinID, int size));
#endif
#ifndef MOVE_RESIZE_BIN_TOTABLE_OCALL_DEFINED__
#define MOVE_RESIZE_BIN_TOTABLE_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, move_resize_bin_totable_OCALL, (int tableID, int srcBinID, int destBinID, int size));
#endif
#ifndef INITBINSTORAGE_OCALL_DEFINED__
#define INITBINSTORAGE_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, initBinStorage_OCALL, (int tableID, int binID));
#endif
#ifndef DELETE_BIN_OCALL_DEFINED__
#define DELETE_BIN_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, delete_bin_OCALL, (int tableID, int binID));
#endif
#ifndef DELETE_TABLE_OCALL_DEFINED__
#define DELETE_TABLE_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, delete_table_OCALL, (int tableID));
#endif
#ifndef COLLECT_RESIZE_BINS_OCALL_DEFINED__
#define COLLECT_RESIZE_BINS_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, collect_resize_bins_OCALL, (int tableID, char* leaves_id, uint dsize, char* binSizes, uint bsize));
#endif
#ifndef WRITE_INDEX_OCALL_DEFINED__
#define WRITE_INDEX_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, write_index_OCALL, (int structureID, char* s_idxBinStart, uint size1, char* s_idxBinEnd, uint size2, const char* str));
#endif
#ifndef WRITE_JOIN_OUTPUT_OCALL_DEFINED__
#define WRITE_JOIN_OUTPUT_OCALL_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, write_join_output_OCALL, (int writeID, char* res, uint dsize));
#endif
#ifndef OCALL_POINTER_USER_CHECK_DEFINED__
#define OCALL_POINTER_USER_CHECK_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_pointer_user_check, (int* val));
#endif
#ifndef OCALL_POINTER_IN_DEFINED__
#define OCALL_POINTER_IN_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_pointer_in, (int* val));
#endif
#ifndef OCALL_POINTER_OUT_DEFINED__
#define OCALL_POINTER_OUT_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_pointer_out, (int* val));
#endif
#ifndef OCALL_POINTER_IN_OUT_DEFINED__
#define OCALL_POINTER_IN_OUT_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_pointer_in_out, (int* val));
#endif
#ifndef MEMCCPY_DEFINED__
#define MEMCCPY_DEFINED__
SGX_DLLIMPORT void* SGX_UBRIDGE(SGX_CDECL, memccpy, (void* dest, const void* src, int val, size_t len));
#endif
#ifndef OCALL_FUNCTION_ALLOW_DEFINED__
#define OCALL_FUNCTION_ALLOW_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_function_allow, (void));
#endif
#ifndef SGX_OC_CPUIDEX_DEFINED__
#define SGX_OC_CPUIDEX_DEFINED__
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
#endif
#ifndef SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
#endif
#ifndef SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
#endif
#ifndef SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
#endif
#ifndef SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));
#endif

sgx_status_t extractMeta(sgx_enclave_id_t eid, int tableID, char* encryptedData, int inSize, char* sensitiveAttr, uint sattrSize);
sgx_status_t extractData(sgx_enclave_id_t eid, int tableID, int nRecords);
sgx_status_t createPDS(sgx_enclave_id_t eid, int* retval, int tableID);
sgx_status_t initJoinPDS(sgx_enclave_id_t eid, int* retval, int pfJoin, int selectJoin);
sgx_status_t initHTree(sgx_enclave_id_t eid, int* retval, int pfJoin, float constant, int ind);
sgx_status_t createJoinPDS(sgx_enclave_id_t eid, int structureID, int tableID1, int tableID2);
sgx_status_t createSelectJoinPDS(sgx_enclave_id_t eid, int structureID, int tableID1, int tableID2);
sgx_status_t createJoinBuckets(sgx_enclave_id_t eid, int structureID, int tableID1, int tableID2, int pfJoin);
sgx_status_t createCZSC(sgx_enclave_id_t eid, int tableID1, char* eData_t1, int inSize_t1, int join_idx1, int tableID2, char* eData_t2, int inSize_t2, int join_idx2, int pfJoin);
sgx_status_t buildIndex(sgx_enclave_id_t eid, int* retval, int structureID, int idx_dim, const char* str, join_type jtype);
sgx_status_t restoreIndex(sgx_enclave_id_t eid, int* retval, int structureID, int idx_dim, char* s_idxBinStart, uint size1, char* s_idxBinEnd, uint size2);
sgx_status_t restoreJoinMeta(sgx_enclave_id_t eid, int tableID, int binID, int pos, int join_idx, char* encryptedBin, uint dsize);
sgx_status_t idxSelect(sgx_enclave_id_t eid, int* retval, int structureID, int idx_dim, int queryStart, int queryEnd, char* s_bins, uint dsize, char* s_records, uint rdsize, join_type jtype);
sgx_status_t innerJoin(sgx_enclave_id_t eid, int* retval, int structureID, int tableID1, int tableID2, int writeID, join_type jtype, int qs, int qe, int jattr);
sgx_status_t pfJoin(sgx_enclave_id_t eid, int* retval, int structureID, int tableID1, int tableID2, int writeID, join_type jtype);
sgx_status_t czscJoin(sgx_enclave_id_t eid, int* retval, int tableID1, int tableID2, int writeID, int pfJoin);
sgx_status_t joinCompact(sgx_enclave_id_t eid, int structureID, int writeID, int us_size, join_type jtype);
sgx_status_t ecall_type_char(sgx_enclave_id_t eid, char val);
sgx_status_t ecall_type_int(sgx_enclave_id_t eid, int val);
sgx_status_t ecall_type_float(sgx_enclave_id_t eid, float val);
sgx_status_t ecall_type_double(sgx_enclave_id_t eid, double val);
sgx_status_t ecall_type_size_t(sgx_enclave_id_t eid, size_t val);
sgx_status_t ecall_type_wchar_t(sgx_enclave_id_t eid, wchar_t val);
sgx_status_t ecall_type_struct(sgx_enclave_id_t eid, struct struct_foo_t val);
sgx_status_t ecall_type_enum_union(sgx_enclave_id_t eid, enum enum_foo_t val1, union union_foo_t* val2);
sgx_status_t ecall_pointer_user_check(sgx_enclave_id_t eid, size_t* retval, void* val, size_t sz);
sgx_status_t ecall_pointer_in(sgx_enclave_id_t eid, int* val);
sgx_status_t ecall_pointer_out(sgx_enclave_id_t eid, int* val);
sgx_status_t ecall_pointer_in_out(sgx_enclave_id_t eid, int* val);
sgx_status_t ecall_pointer_string(sgx_enclave_id_t eid, char* str);
sgx_status_t ecall_pointer_string_const(sgx_enclave_id_t eid, const char* str);
sgx_status_t ecall_pointer_size(sgx_enclave_id_t eid, void* ptr, size_t len);
sgx_status_t ecall_pointer_count(sgx_enclave_id_t eid, int* arr, int cnt);
sgx_status_t ecall_pointer_isptr_readonly(sgx_enclave_id_t eid, buffer_t buf, size_t len);
sgx_status_t ocall_pointer_attr(sgx_enclave_id_t eid);
sgx_status_t ecall_array_user_check(sgx_enclave_id_t eid, int arr[4]);
sgx_status_t ecall_array_in(sgx_enclave_id_t eid, int arr[4]);
sgx_status_t ecall_array_out(sgx_enclave_id_t eid, int arr[4]);
sgx_status_t ecall_array_in_out(sgx_enclave_id_t eid, int arr[4]);
sgx_status_t ecall_array_isary(sgx_enclave_id_t eid, array_t arr);
sgx_status_t ecall_function_calling_convs(sgx_enclave_id_t eid);
sgx_status_t ecall_function_public(sgx_enclave_id_t eid);
sgx_status_t ecall_function_private(sgx_enclave_id_t eid, int* retval);
sgx_status_t ecall_malloc_free(sgx_enclave_id_t eid);
sgx_status_t ecall_sgx_cpuid(sgx_enclave_id_t eid, int cpuinfo[4], int leaf);
sgx_status_t ecall_exception(sgx_enclave_id_t eid);
sgx_status_t ecall_map(sgx_enclave_id_t eid);
sgx_status_t ecall_increase_counter(sgx_enclave_id_t eid, size_t* retval);
sgx_status_t ecall_producer(sgx_enclave_id_t eid);
sgx_status_t ecall_consumer(sgx_enclave_id_t eid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
