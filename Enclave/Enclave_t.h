#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */

#include "user_types.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NO_HARDEN_EXT_WRITES
#define MEMCPY_S memcpy_s
#define MEMSET memset
#else
#define MEMCPY_S memcpy_verw_s
#define MEMSET memset_verw
#endif /* NO_HARDEN_EXT_WRITES */

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

void extractMeta(int tableID, char* encryptedData, int inSize, char* sensitiveAttr, uint sattrSize);
void extractData(int tableID, int nRecords);
int createPDS(int tableID);
int initJoinPDS(int pfJoin, int selectJoin);
int initHTree(int pfJoin, float constant, int ind);
void createJoinPDS(int structureID, int tableID1, int tableID2);
void createSelectJoinPDS(int structureID, int tableID1, int tableID2);
void createJoinBuckets(int structureID, int tableID1, int tableID2, int pfJoin);
void createCZSC(int tableID1, char* eData_t1, int inSize_t1, int join_idx1, int tableID2, char* eData_t2, int inSize_t2, int join_idx2, int pfJoin);
int buildIndex(int structureID, int idx_dim, const char* str, join_type jtype);
int restoreIndex(int structureID, int idx_dim, char* s_idxBinStart, uint size1, char* s_idxBinEnd, uint size2);
void restoreJoinMeta(int tableID, int binID, int pos, int join_idx, char* encryptedBin, uint dsize);
int idxSelect(int structureID, int idx_dim, int queryStart, int queryEnd, char* s_bins, uint dsize, char* s_records, uint rdsize, join_type jtype);
int innerJoin(int structureID, int tableID1, int tableID2, int writeID, join_type jtype, int qs, int qe, int jattr);
int pfJoin(int structureID, int tableID1, int tableID2, int writeID, join_type jtype);
int czscJoin(int tableID1, int tableID2, int writeID, int pfJoin);
void joinCompact(int structureID, int writeID, int us_size, join_type jtype);
void ecall_type_char(char val);
void ecall_type_int(int val);
void ecall_type_float(float val);
void ecall_type_double(double val);
void ecall_type_size_t(size_t val);
void ecall_type_wchar_t(wchar_t val);
void ecall_type_struct(struct struct_foo_t val);
void ecall_type_enum_union(enum enum_foo_t val1, union union_foo_t* val2);
size_t ecall_pointer_user_check(void* val, size_t sz);
void ecall_pointer_in(int* val);
void ecall_pointer_out(int* val);
void ecall_pointer_in_out(int* val);
void ecall_pointer_string(char* str);
void ecall_pointer_string_const(const char* str);
void ecall_pointer_size(void* ptr, size_t len);
void ecall_pointer_count(int* arr, int cnt);
void ecall_pointer_isptr_readonly(buffer_t buf, size_t len);
void ocall_pointer_attr(void);
void ecall_array_user_check(int arr[4]);
void ecall_array_in(int arr[4]);
void ecall_array_out(int arr[4]);
void ecall_array_in_out(int arr[4]);
void ecall_array_isary(array_t arr);
void ecall_function_calling_convs(void);
void ecall_function_public(void);
int ecall_function_private(void);
void ecall_malloc_free(void);
void ecall_sgx_cpuid(int cpuinfo[4], int leaf);
void ecall_exception(void);
void ecall_map(void);
size_t ecall_increase_counter(void);
void ecall_producer(void);
void ecall_consumer(void);

sgx_status_t SGX_CDECL ocall_print_string(const char* str);
sgx_status_t SGX_CDECL expand_table_OCALL(int tableID, uint noiseSum, char* noise, uint nsize);
sgx_status_t SGX_CDECL readDataBlock_OCALL(int* retval, int tableID, int readBinID, int pos, char* dblock, int dsize);
sgx_status_t SGX_CDECL writeDataBlock_OCALL(int tableID, int writeBinID, uint pos, char* dblock, uint dsize);
sgx_status_t SGX_CDECL copy_bin_OCALL(int tableID, int readBinID, int writeBinID, int startPos, int endPos);
sgx_status_t SGX_CDECL move_resize_bin_OCALL(int tableID, int srcBinID, int destBinID, int size);
sgx_status_t SGX_CDECL move_resize_bin_totable_OCALL(int tableID, int srcBinID, int destBinID, int size);
sgx_status_t SGX_CDECL initBinStorage_OCALL(int tableID, int binID);
sgx_status_t SGX_CDECL delete_bin_OCALL(int tableID, int binID);
sgx_status_t SGX_CDECL delete_table_OCALL(int tableID);
sgx_status_t SGX_CDECL collect_resize_bins_OCALL(int tableID, char* leaves_id, uint dsize, char* binSizes, uint bsize);
sgx_status_t SGX_CDECL write_index_OCALL(int structureID, char* s_idxBinStart, uint size1, char* s_idxBinEnd, uint size2, const char* str);
sgx_status_t SGX_CDECL write_join_output_OCALL(int writeID, char* res, uint dsize);
sgx_status_t SGX_CDECL ocall_pointer_user_check(int* val);
sgx_status_t SGX_CDECL ocall_pointer_in(int* val);
sgx_status_t SGX_CDECL ocall_pointer_out(int* val);
sgx_status_t SGX_CDECL ocall_pointer_in_out(int* val);
sgx_status_t SGX_CDECL memccpy(void** retval, void* dest, const void* src, int val, size_t len);
sgx_status_t SGX_CDECL ocall_function_allow(void);
sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf);
sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter);
sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
