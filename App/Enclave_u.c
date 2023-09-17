#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_extractMeta_t {
	int ms_tableID;
	char* ms_encryptedData;
	int ms_inSize;
	char* ms_sensitiveAttr;
	uint ms_sattrSize;
} ms_extractMeta_t;

typedef struct ms_extractData_t {
	int ms_tableID;
	int ms_nRecords;
} ms_extractData_t;

typedef struct ms_createPDS_t {
	int ms_retval;
	int ms_tableID;
} ms_createPDS_t;

typedef struct ms_initJoinPDS_t {
	int ms_retval;
	int ms_pfJoin;
	int ms_selectJoin;
} ms_initJoinPDS_t;

typedef struct ms_initHTree_t {
	int ms_retval;
	int ms_pfJoin;
	float ms_constant;
	int ms_ind;
} ms_initHTree_t;

typedef struct ms_createJoinPDS_t {
	int ms_structureID;
	int ms_tableID1;
	int ms_tableID2;
} ms_createJoinPDS_t;

typedef struct ms_createSelectJoinPDS_t {
	int ms_structureID;
	int ms_tableID1;
	int ms_tableID2;
} ms_createSelectJoinPDS_t;

typedef struct ms_createJoinBuckets_t {
	int ms_structureID;
	int ms_tableID1;
	int ms_tableID2;
	int ms_pfJoin;
} ms_createJoinBuckets_t;

typedef struct ms_createCZSC_t {
	int ms_tableID1;
	char* ms_eData_t1;
	int ms_inSize_t1;
	int ms_join_idx1;
	int ms_tableID2;
	char* ms_eData_t2;
	int ms_inSize_t2;
	int ms_join_idx2;
	int ms_pfJoin;
} ms_createCZSC_t;

typedef struct ms_buildIndex_t {
	int ms_retval;
	int ms_structureID;
	int ms_idx_dim;
	const char* ms_str;
	size_t ms_str_len;
	join_type ms_jtype;
} ms_buildIndex_t;

typedef struct ms_restoreIndex_t {
	int ms_retval;
	int ms_structureID;
	int ms_idx_dim;
	char* ms_s_idxBinStart;
	uint ms_size1;
	char* ms_s_idxBinEnd;
	uint ms_size2;
} ms_restoreIndex_t;

typedef struct ms_restoreJoinMeta_t {
	int ms_tableID;
	int ms_binID;
	int ms_pos;
	int ms_join_idx;
	char* ms_encryptedBin;
	uint ms_dsize;
} ms_restoreJoinMeta_t;

typedef struct ms_idxSelect_t {
	int ms_retval;
	int ms_structureID;
	int ms_idx_dim;
	int ms_queryStart;
	int ms_queryEnd;
	char* ms_s_bins;
	uint ms_dsize;
	char* ms_s_records;
	uint ms_rdsize;
	join_type ms_jtype;
} ms_idxSelect_t;

typedef struct ms_innerJoin_t {
	int ms_retval;
	int ms_structureID;
	int ms_tableID1;
	int ms_tableID2;
	int ms_writeID;
	join_type ms_jtype;
	int ms_qs;
	int ms_qe;
	int ms_jattr;
} ms_innerJoin_t;

typedef struct ms_pfJoin_t {
	int ms_retval;
	int ms_structureID;
	int ms_tableID1;
	int ms_tableID2;
	int ms_writeID;
	join_type ms_jtype;
} ms_pfJoin_t;

typedef struct ms_czscJoin_t {
	int ms_retval;
	int ms_tableID1;
	int ms_tableID2;
	int ms_writeID;
	int ms_pfJoin;
} ms_czscJoin_t;

typedef struct ms_joinCompact_t {
	int ms_structureID;
	int ms_writeID;
	int ms_us_size;
	join_type ms_jtype;
} ms_joinCompact_t;

typedef struct ms_ecall_type_char_t {
	char ms_val;
} ms_ecall_type_char_t;

typedef struct ms_ecall_type_int_t {
	int ms_val;
} ms_ecall_type_int_t;

typedef struct ms_ecall_type_float_t {
	float ms_val;
} ms_ecall_type_float_t;

typedef struct ms_ecall_type_double_t {
	double ms_val;
} ms_ecall_type_double_t;

typedef struct ms_ecall_type_size_t_t {
	size_t ms_val;
} ms_ecall_type_size_t_t;

typedef struct ms_ecall_type_wchar_t_t {
	wchar_t ms_val;
} ms_ecall_type_wchar_t_t;

typedef struct ms_ecall_type_struct_t {
	struct struct_foo_t ms_val;
} ms_ecall_type_struct_t;

typedef struct ms_ecall_type_enum_union_t {
	enum enum_foo_t ms_val1;
	union union_foo_t* ms_val2;
} ms_ecall_type_enum_union_t;

typedef struct ms_ecall_pointer_user_check_t {
	size_t ms_retval;
	void* ms_val;
	size_t ms_sz;
} ms_ecall_pointer_user_check_t;

typedef struct ms_ecall_pointer_in_t {
	int* ms_val;
} ms_ecall_pointer_in_t;

typedef struct ms_ecall_pointer_out_t {
	int* ms_val;
} ms_ecall_pointer_out_t;

typedef struct ms_ecall_pointer_in_out_t {
	int* ms_val;
} ms_ecall_pointer_in_out_t;

typedef struct ms_ecall_pointer_string_t {
	char* ms_str;
	size_t ms_str_len;
} ms_ecall_pointer_string_t;

typedef struct ms_ecall_pointer_string_const_t {
	const char* ms_str;
	size_t ms_str_len;
} ms_ecall_pointer_string_const_t;

typedef struct ms_ecall_pointer_size_t {
	void* ms_ptr;
	size_t ms_len;
} ms_ecall_pointer_size_t;

typedef struct ms_ecall_pointer_count_t {
	int* ms_arr;
	int ms_cnt;
} ms_ecall_pointer_count_t;

typedef struct ms_ecall_pointer_isptr_readonly_t {
	buffer_t ms_buf;
	size_t ms_len;
} ms_ecall_pointer_isptr_readonly_t;

typedef struct ms_ecall_array_user_check_t {
	int* ms_arr;
} ms_ecall_array_user_check_t;

typedef struct ms_ecall_array_in_t {
	int* ms_arr;
} ms_ecall_array_in_t;

typedef struct ms_ecall_array_out_t {
	int* ms_arr;
} ms_ecall_array_out_t;

typedef struct ms_ecall_array_in_out_t {
	int* ms_arr;
} ms_ecall_array_in_out_t;

typedef struct ms_ecall_array_isary_t {
	array_t*  ms_arr;
} ms_ecall_array_isary_t;

typedef struct ms_ecall_function_private_t {
	int ms_retval;
} ms_ecall_function_private_t;

typedef struct ms_ecall_sgx_cpuid_t {
	int* ms_cpuinfo;
	int ms_leaf;
} ms_ecall_sgx_cpuid_t;

typedef struct ms_ecall_increase_counter_t {
	size_t ms_retval;
} ms_ecall_increase_counter_t;

typedef struct ms_ocall_print_string_t {
	const char* ms_str;
} ms_ocall_print_string_t;

typedef struct ms_expand_table_OCALL_t {
	int ms_tableID;
	uint ms_noiseSum;
	char* ms_noise;
	uint ms_nsize;
} ms_expand_table_OCALL_t;

typedef struct ms_readDataBlock_OCALL_t {
	int ms_retval;
	int ms_tableID;
	int ms_readBinID;
	int ms_pos;
	char* ms_dblock;
	int ms_dsize;
} ms_readDataBlock_OCALL_t;

typedef struct ms_writeDataBlock_OCALL_t {
	int ms_tableID;
	int ms_writeBinID;
	uint ms_pos;
	char* ms_dblock;
	uint ms_dsize;
} ms_writeDataBlock_OCALL_t;

typedef struct ms_copy_bin_OCALL_t {
	int ms_tableID;
	int ms_readBinID;
	int ms_writeBinID;
	int ms_startPos;
	int ms_endPos;
} ms_copy_bin_OCALL_t;

typedef struct ms_move_resize_bin_OCALL_t {
	int ms_tableID;
	int ms_srcBinID;
	int ms_destBinID;
	int ms_size;
} ms_move_resize_bin_OCALL_t;

typedef struct ms_move_resize_bin_totable_OCALL_t {
	int ms_tableID;
	int ms_srcBinID;
	int ms_destBinID;
	int ms_size;
} ms_move_resize_bin_totable_OCALL_t;

typedef struct ms_initBinStorage_OCALL_t {
	int ms_tableID;
	int ms_binID;
} ms_initBinStorage_OCALL_t;

typedef struct ms_delete_bin_OCALL_t {
	int ms_tableID;
	int ms_binID;
} ms_delete_bin_OCALL_t;

typedef struct ms_delete_table_OCALL_t {
	int ms_tableID;
} ms_delete_table_OCALL_t;

typedef struct ms_collect_resize_bins_OCALL_t {
	int ms_tableID;
	char* ms_leaves_id;
	uint ms_dsize;
	char* ms_binSizes;
	uint ms_bsize;
} ms_collect_resize_bins_OCALL_t;

typedef struct ms_write_index_OCALL_t {
	int ms_structureID;
	char* ms_s_idxBinStart;
	uint ms_size1;
	char* ms_s_idxBinEnd;
	uint ms_size2;
	const char* ms_str;
} ms_write_index_OCALL_t;

typedef struct ms_write_join_output_OCALL_t {
	int ms_writeID;
	char* ms_res;
	uint ms_dsize;
} ms_write_join_output_OCALL_t;

typedef struct ms_ocall_pointer_user_check_t {
	int* ms_val;
} ms_ocall_pointer_user_check_t;

typedef struct ms_ocall_pointer_in_t {
	int* ms_val;
} ms_ocall_pointer_in_t;

typedef struct ms_ocall_pointer_out_t {
	int* ms_val;
} ms_ocall_pointer_out_t;

typedef struct ms_ocall_pointer_in_out_t {
	int* ms_val;
} ms_ocall_pointer_in_out_t;

typedef struct ms_memccpy_t {
	void* ms_retval;
	void* ms_dest;
	const void* ms_src;
	int ms_val;
	size_t ms_len;
} ms_memccpy_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	const void* ms_waiter;
	const void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	const void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

static sgx_status_t SGX_CDECL Enclave_ocall_print_string(void* pms)
{
	ms_ocall_print_string_t* ms = SGX_CAST(ms_ocall_print_string_t*, pms);
	ocall_print_string(ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_expand_table_OCALL(void* pms)
{
	ms_expand_table_OCALL_t* ms = SGX_CAST(ms_expand_table_OCALL_t*, pms);
	expand_table_OCALL(ms->ms_tableID, ms->ms_noiseSum, ms->ms_noise, ms->ms_nsize);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_readDataBlock_OCALL(void* pms)
{
	ms_readDataBlock_OCALL_t* ms = SGX_CAST(ms_readDataBlock_OCALL_t*, pms);
	ms->ms_retval = readDataBlock_OCALL(ms->ms_tableID, ms->ms_readBinID, ms->ms_pos, ms->ms_dblock, ms->ms_dsize);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_writeDataBlock_OCALL(void* pms)
{
	ms_writeDataBlock_OCALL_t* ms = SGX_CAST(ms_writeDataBlock_OCALL_t*, pms);
	writeDataBlock_OCALL(ms->ms_tableID, ms->ms_writeBinID, ms->ms_pos, ms->ms_dblock, ms->ms_dsize);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_copy_bin_OCALL(void* pms)
{
	ms_copy_bin_OCALL_t* ms = SGX_CAST(ms_copy_bin_OCALL_t*, pms);
	copy_bin_OCALL(ms->ms_tableID, ms->ms_readBinID, ms->ms_writeBinID, ms->ms_startPos, ms->ms_endPos);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_move_resize_bin_OCALL(void* pms)
{
	ms_move_resize_bin_OCALL_t* ms = SGX_CAST(ms_move_resize_bin_OCALL_t*, pms);
	move_resize_bin_OCALL(ms->ms_tableID, ms->ms_srcBinID, ms->ms_destBinID, ms->ms_size);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_move_resize_bin_totable_OCALL(void* pms)
{
	ms_move_resize_bin_totable_OCALL_t* ms = SGX_CAST(ms_move_resize_bin_totable_OCALL_t*, pms);
	move_resize_bin_totable_OCALL(ms->ms_tableID, ms->ms_srcBinID, ms->ms_destBinID, ms->ms_size);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_initBinStorage_OCALL(void* pms)
{
	ms_initBinStorage_OCALL_t* ms = SGX_CAST(ms_initBinStorage_OCALL_t*, pms);
	initBinStorage_OCALL(ms->ms_tableID, ms->ms_binID);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_delete_bin_OCALL(void* pms)
{
	ms_delete_bin_OCALL_t* ms = SGX_CAST(ms_delete_bin_OCALL_t*, pms);
	delete_bin_OCALL(ms->ms_tableID, ms->ms_binID);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_delete_table_OCALL(void* pms)
{
	ms_delete_table_OCALL_t* ms = SGX_CAST(ms_delete_table_OCALL_t*, pms);
	delete_table_OCALL(ms->ms_tableID);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_collect_resize_bins_OCALL(void* pms)
{
	ms_collect_resize_bins_OCALL_t* ms = SGX_CAST(ms_collect_resize_bins_OCALL_t*, pms);
	collect_resize_bins_OCALL(ms->ms_tableID, ms->ms_leaves_id, ms->ms_dsize, ms->ms_binSizes, ms->ms_bsize);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_write_index_OCALL(void* pms)
{
	ms_write_index_OCALL_t* ms = SGX_CAST(ms_write_index_OCALL_t*, pms);
	write_index_OCALL(ms->ms_structureID, ms->ms_s_idxBinStart, ms->ms_size1, ms->ms_s_idxBinEnd, ms->ms_size2, ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_write_join_output_OCALL(void* pms)
{
	ms_write_join_output_OCALL_t* ms = SGX_CAST(ms_write_join_output_OCALL_t*, pms);
	write_join_output_OCALL(ms->ms_writeID, ms->ms_res, ms->ms_dsize);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_pointer_user_check(void* pms)
{
	ms_ocall_pointer_user_check_t* ms = SGX_CAST(ms_ocall_pointer_user_check_t*, pms);
	ocall_pointer_user_check(ms->ms_val);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_pointer_in(void* pms)
{
	ms_ocall_pointer_in_t* ms = SGX_CAST(ms_ocall_pointer_in_t*, pms);
	ocall_pointer_in(ms->ms_val);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_pointer_out(void* pms)
{
	ms_ocall_pointer_out_t* ms = SGX_CAST(ms_ocall_pointer_out_t*, pms);
	ocall_pointer_out(ms->ms_val);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_pointer_in_out(void* pms)
{
	ms_ocall_pointer_in_out_t* ms = SGX_CAST(ms_ocall_pointer_in_out_t*, pms);
	ocall_pointer_in_out(ms->ms_val);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_memccpy(void* pms)
{
	ms_memccpy_t* ms = SGX_CAST(ms_memccpy_t*, pms);
	ms->ms_retval = memccpy(ms->ms_dest, ms->ms_src, ms->ms_val, ms->ms_len);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_function_allow(void* pms)
{
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ocall_function_allow();
	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall(ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall(ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall(ms->ms_waiter, ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall(ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[24];
} ocall_table_Enclave = {
	24,
	{
		(void*)Enclave_ocall_print_string,
		(void*)Enclave_expand_table_OCALL,
		(void*)Enclave_readDataBlock_OCALL,
		(void*)Enclave_writeDataBlock_OCALL,
		(void*)Enclave_copy_bin_OCALL,
		(void*)Enclave_move_resize_bin_OCALL,
		(void*)Enclave_move_resize_bin_totable_OCALL,
		(void*)Enclave_initBinStorage_OCALL,
		(void*)Enclave_delete_bin_OCALL,
		(void*)Enclave_delete_table_OCALL,
		(void*)Enclave_collect_resize_bins_OCALL,
		(void*)Enclave_write_index_OCALL,
		(void*)Enclave_write_join_output_OCALL,
		(void*)Enclave_ocall_pointer_user_check,
		(void*)Enclave_ocall_pointer_in,
		(void*)Enclave_ocall_pointer_out,
		(void*)Enclave_ocall_pointer_in_out,
		(void*)Enclave_memccpy,
		(void*)Enclave_ocall_function_allow,
		(void*)Enclave_sgx_oc_cpuidex,
		(void*)Enclave_sgx_thread_wait_untrusted_event_ocall,
		(void*)Enclave_sgx_thread_set_untrusted_event_ocall,
		(void*)Enclave_sgx_thread_setwait_untrusted_events_ocall,
		(void*)Enclave_sgx_thread_set_multiple_untrusted_events_ocall,
	}
};
sgx_status_t extractMeta(sgx_enclave_id_t eid, int tableID, char* encryptedData, int inSize, char* sensitiveAttr, uint sattrSize)
{
	sgx_status_t status;
	ms_extractMeta_t ms;
	ms.ms_tableID = tableID;
	ms.ms_encryptedData = encryptedData;
	ms.ms_inSize = inSize;
	ms.ms_sensitiveAttr = sensitiveAttr;
	ms.ms_sattrSize = sattrSize;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t extractData(sgx_enclave_id_t eid, int tableID, int nRecords)
{
	sgx_status_t status;
	ms_extractData_t ms;
	ms.ms_tableID = tableID;
	ms.ms_nRecords = nRecords;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t createPDS(sgx_enclave_id_t eid, int* retval, int tableID)
{
	sgx_status_t status;
	ms_createPDS_t ms;
	ms.ms_tableID = tableID;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t initJoinPDS(sgx_enclave_id_t eid, int* retval, int pfJoin, int selectJoin)
{
	sgx_status_t status;
	ms_initJoinPDS_t ms;
	ms.ms_pfJoin = pfJoin;
	ms.ms_selectJoin = selectJoin;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t initHTree(sgx_enclave_id_t eid, int* retval, int pfJoin, float constant, int ind)
{
	sgx_status_t status;
	ms_initHTree_t ms;
	ms.ms_pfJoin = pfJoin;
	ms.ms_constant = constant;
	ms.ms_ind = ind;
	status = sgx_ecall(eid, 4, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t createJoinPDS(sgx_enclave_id_t eid, int structureID, int tableID1, int tableID2)
{
	sgx_status_t status;
	ms_createJoinPDS_t ms;
	ms.ms_structureID = structureID;
	ms.ms_tableID1 = tableID1;
	ms.ms_tableID2 = tableID2;
	status = sgx_ecall(eid, 5, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t createSelectJoinPDS(sgx_enclave_id_t eid, int structureID, int tableID1, int tableID2)
{
	sgx_status_t status;
	ms_createSelectJoinPDS_t ms;
	ms.ms_structureID = structureID;
	ms.ms_tableID1 = tableID1;
	ms.ms_tableID2 = tableID2;
	status = sgx_ecall(eid, 6, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t createJoinBuckets(sgx_enclave_id_t eid, int structureID, int tableID1, int tableID2, int pfJoin)
{
	sgx_status_t status;
	ms_createJoinBuckets_t ms;
	ms.ms_structureID = structureID;
	ms.ms_tableID1 = tableID1;
	ms.ms_tableID2 = tableID2;
	ms.ms_pfJoin = pfJoin;
	status = sgx_ecall(eid, 7, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t createCZSC(sgx_enclave_id_t eid, int tableID1, char* eData_t1, int inSize_t1, int join_idx1, int tableID2, char* eData_t2, int inSize_t2, int join_idx2, int pfJoin)
{
	sgx_status_t status;
	ms_createCZSC_t ms;
	ms.ms_tableID1 = tableID1;
	ms.ms_eData_t1 = eData_t1;
	ms.ms_inSize_t1 = inSize_t1;
	ms.ms_join_idx1 = join_idx1;
	ms.ms_tableID2 = tableID2;
	ms.ms_eData_t2 = eData_t2;
	ms.ms_inSize_t2 = inSize_t2;
	ms.ms_join_idx2 = join_idx2;
	ms.ms_pfJoin = pfJoin;
	status = sgx_ecall(eid, 8, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t buildIndex(sgx_enclave_id_t eid, int* retval, int structureID, int idx_dim, const char* str, join_type jtype)
{
	sgx_status_t status;
	ms_buildIndex_t ms;
	ms.ms_structureID = structureID;
	ms.ms_idx_dim = idx_dim;
	ms.ms_str = str;
	ms.ms_str_len = str ? strlen(str) + 1 : 0;
	ms.ms_jtype = jtype;
	status = sgx_ecall(eid, 9, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t restoreIndex(sgx_enclave_id_t eid, int* retval, int structureID, int idx_dim, char* s_idxBinStart, uint size1, char* s_idxBinEnd, uint size2)
{
	sgx_status_t status;
	ms_restoreIndex_t ms;
	ms.ms_structureID = structureID;
	ms.ms_idx_dim = idx_dim;
	ms.ms_s_idxBinStart = s_idxBinStart;
	ms.ms_size1 = size1;
	ms.ms_s_idxBinEnd = s_idxBinEnd;
	ms.ms_size2 = size2;
	status = sgx_ecall(eid, 10, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t restoreJoinMeta(sgx_enclave_id_t eid, int tableID, int binID, int pos, int join_idx, char* encryptedBin, uint dsize)
{
	sgx_status_t status;
	ms_restoreJoinMeta_t ms;
	ms.ms_tableID = tableID;
	ms.ms_binID = binID;
	ms.ms_pos = pos;
	ms.ms_join_idx = join_idx;
	ms.ms_encryptedBin = encryptedBin;
	ms.ms_dsize = dsize;
	status = sgx_ecall(eid, 11, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t idxSelect(sgx_enclave_id_t eid, int* retval, int structureID, int idx_dim, int queryStart, int queryEnd, char* s_bins, uint dsize, char* s_records, uint rdsize, join_type jtype)
{
	sgx_status_t status;
	ms_idxSelect_t ms;
	ms.ms_structureID = structureID;
	ms.ms_idx_dim = idx_dim;
	ms.ms_queryStart = queryStart;
	ms.ms_queryEnd = queryEnd;
	ms.ms_s_bins = s_bins;
	ms.ms_dsize = dsize;
	ms.ms_s_records = s_records;
	ms.ms_rdsize = rdsize;
	ms.ms_jtype = jtype;
	status = sgx_ecall(eid, 12, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t innerJoin(sgx_enclave_id_t eid, int* retval, int structureID, int tableID1, int tableID2, int writeID, join_type jtype, int qs, int qe, int jattr)
{
	sgx_status_t status;
	ms_innerJoin_t ms;
	ms.ms_structureID = structureID;
	ms.ms_tableID1 = tableID1;
	ms.ms_tableID2 = tableID2;
	ms.ms_writeID = writeID;
	ms.ms_jtype = jtype;
	ms.ms_qs = qs;
	ms.ms_qe = qe;
	ms.ms_jattr = jattr;
	status = sgx_ecall(eid, 13, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t pfJoin(sgx_enclave_id_t eid, int* retval, int structureID, int tableID1, int tableID2, int writeID, join_type jtype)
{
	sgx_status_t status;
	ms_pfJoin_t ms;
	ms.ms_structureID = structureID;
	ms.ms_tableID1 = tableID1;
	ms.ms_tableID2 = tableID2;
	ms.ms_writeID = writeID;
	ms.ms_jtype = jtype;
	status = sgx_ecall(eid, 14, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t czscJoin(sgx_enclave_id_t eid, int* retval, int tableID1, int tableID2, int writeID, int pfJoin)
{
	sgx_status_t status;
	ms_czscJoin_t ms;
	ms.ms_tableID1 = tableID1;
	ms.ms_tableID2 = tableID2;
	ms.ms_writeID = writeID;
	ms.ms_pfJoin = pfJoin;
	status = sgx_ecall(eid, 15, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t joinCompact(sgx_enclave_id_t eid, int structureID, int writeID, int us_size, join_type jtype)
{
	sgx_status_t status;
	ms_joinCompact_t ms;
	ms.ms_structureID = structureID;
	ms.ms_writeID = writeID;
	ms.ms_us_size = us_size;
	ms.ms_jtype = jtype;
	status = sgx_ecall(eid, 16, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_char(sgx_enclave_id_t eid, char val)
{
	sgx_status_t status;
	ms_ecall_type_char_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 17, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_int(sgx_enclave_id_t eid, int val)
{
	sgx_status_t status;
	ms_ecall_type_int_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 18, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_float(sgx_enclave_id_t eid, float val)
{
	sgx_status_t status;
	ms_ecall_type_float_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 19, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_double(sgx_enclave_id_t eid, double val)
{
	sgx_status_t status;
	ms_ecall_type_double_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 20, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_size_t(sgx_enclave_id_t eid, size_t val)
{
	sgx_status_t status;
	ms_ecall_type_size_t_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 21, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_wchar_t(sgx_enclave_id_t eid, wchar_t val)
{
	sgx_status_t status;
	ms_ecall_type_wchar_t_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 22, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_struct(sgx_enclave_id_t eid, struct struct_foo_t val)
{
	sgx_status_t status;
	ms_ecall_type_struct_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 23, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_type_enum_union(sgx_enclave_id_t eid, enum enum_foo_t val1, union union_foo_t* val2)
{
	sgx_status_t status;
	ms_ecall_type_enum_union_t ms;
	ms.ms_val1 = val1;
	ms.ms_val2 = val2;
	status = sgx_ecall(eid, 24, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_user_check(sgx_enclave_id_t eid, size_t* retval, void* val, size_t sz)
{
	sgx_status_t status;
	ms_ecall_pointer_user_check_t ms;
	ms.ms_val = val;
	ms.ms_sz = sz;
	status = sgx_ecall(eid, 25, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_pointer_in(sgx_enclave_id_t eid, int* val)
{
	sgx_status_t status;
	ms_ecall_pointer_in_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 26, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_out(sgx_enclave_id_t eid, int* val)
{
	sgx_status_t status;
	ms_ecall_pointer_out_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 27, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_in_out(sgx_enclave_id_t eid, int* val)
{
	sgx_status_t status;
	ms_ecall_pointer_in_out_t ms;
	ms.ms_val = val;
	status = sgx_ecall(eid, 28, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_string(sgx_enclave_id_t eid, char* str)
{
	sgx_status_t status;
	ms_ecall_pointer_string_t ms;
	ms.ms_str = str;
	ms.ms_str_len = str ? strlen(str) + 1 : 0;
	status = sgx_ecall(eid, 29, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_string_const(sgx_enclave_id_t eid, const char* str)
{
	sgx_status_t status;
	ms_ecall_pointer_string_const_t ms;
	ms.ms_str = str;
	ms.ms_str_len = str ? strlen(str) + 1 : 0;
	status = sgx_ecall(eid, 30, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_size(sgx_enclave_id_t eid, void* ptr, size_t len)
{
	sgx_status_t status;
	ms_ecall_pointer_size_t ms;
	ms.ms_ptr = ptr;
	ms.ms_len = len;
	status = sgx_ecall(eid, 31, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_count(sgx_enclave_id_t eid, int* arr, int cnt)
{
	sgx_status_t status;
	ms_ecall_pointer_count_t ms;
	ms.ms_arr = arr;
	ms.ms_cnt = cnt;
	status = sgx_ecall(eid, 32, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_pointer_isptr_readonly(sgx_enclave_id_t eid, buffer_t buf, size_t len)
{
	sgx_status_t status;
	ms_ecall_pointer_isptr_readonly_t ms;
	ms.ms_buf = buf;
	ms.ms_len = len;
	status = sgx_ecall(eid, 33, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ocall_pointer_attr(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 34, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_array_user_check(sgx_enclave_id_t eid, int arr[4])
{
	sgx_status_t status;
	ms_ecall_array_user_check_t ms;
	ms.ms_arr = (int*)arr;
	status = sgx_ecall(eid, 35, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_array_in(sgx_enclave_id_t eid, int arr[4])
{
	sgx_status_t status;
	ms_ecall_array_in_t ms;
	ms.ms_arr = (int*)arr;
	status = sgx_ecall(eid, 36, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_array_out(sgx_enclave_id_t eid, int arr[4])
{
	sgx_status_t status;
	ms_ecall_array_out_t ms;
	ms.ms_arr = (int*)arr;
	status = sgx_ecall(eid, 37, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_array_in_out(sgx_enclave_id_t eid, int arr[4])
{
	sgx_status_t status;
	ms_ecall_array_in_out_t ms;
	ms.ms_arr = (int*)arr;
	status = sgx_ecall(eid, 38, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_array_isary(sgx_enclave_id_t eid, array_t arr)
{
	sgx_status_t status;
	ms_ecall_array_isary_t ms;
	ms.ms_arr = (array_t *)&arr[0];
	status = sgx_ecall(eid, 39, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_function_calling_convs(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 40, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_function_public(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 41, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_function_private(sgx_enclave_id_t eid, int* retval)
{
	sgx_status_t status;
	ms_ecall_function_private_t ms;
	status = sgx_ecall(eid, 42, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_malloc_free(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 43, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_sgx_cpuid(sgx_enclave_id_t eid, int cpuinfo[4], int leaf)
{
	sgx_status_t status;
	ms_ecall_sgx_cpuid_t ms;
	ms.ms_cpuinfo = (int*)cpuinfo;
	ms.ms_leaf = leaf;
	status = sgx_ecall(eid, 44, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_exception(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 45, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_map(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 46, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_increase_counter(sgx_enclave_id_t eid, size_t* retval)
{
	sgx_status_t status;
	ms_ecall_increase_counter_t ms;
	status = sgx_ecall(eid, 47, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_producer(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 48, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t ecall_consumer(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 49, &ocall_table_Enclave, NULL);
	return status;
}

