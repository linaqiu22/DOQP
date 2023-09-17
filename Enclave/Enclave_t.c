#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


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

static sgx_status_t SGX_CDECL sgx_extractMeta(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_extractMeta_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_extractMeta_t* ms = SGX_CAST(ms_extractMeta_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_encryptedData = ms->ms_encryptedData;
	int _tmp_inSize = ms->ms_inSize;
	size_t _len_encryptedData = _tmp_inSize;
	char* _in_encryptedData = NULL;
	char* _tmp_sensitiveAttr = ms->ms_sensitiveAttr;
	uint _tmp_sattrSize = ms->ms_sattrSize;
	size_t _len_sensitiveAttr = _tmp_sattrSize;
	char* _in_sensitiveAttr = NULL;

	CHECK_UNIQUE_POINTER(_tmp_encryptedData, _len_encryptedData);
	CHECK_UNIQUE_POINTER(_tmp_sensitiveAttr, _len_sensitiveAttr);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_encryptedData != NULL && _len_encryptedData != 0) {
		if ( _len_encryptedData % sizeof(*_tmp_encryptedData) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_encryptedData = (char*)malloc(_len_encryptedData);
		if (_in_encryptedData == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_encryptedData, _len_encryptedData, _tmp_encryptedData, _len_encryptedData)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_sensitiveAttr != NULL && _len_sensitiveAttr != 0) {
		if ( _len_sensitiveAttr % sizeof(*_tmp_sensitiveAttr) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_sensitiveAttr = (char*)malloc(_len_sensitiveAttr);
		if (_in_sensitiveAttr == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_sensitiveAttr, _len_sensitiveAttr, _tmp_sensitiveAttr, _len_sensitiveAttr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	extractMeta(ms->ms_tableID, _in_encryptedData, _tmp_inSize, _in_sensitiveAttr, _tmp_sattrSize);

err:
	if (_in_encryptedData) free(_in_encryptedData);
	if (_in_sensitiveAttr) free(_in_sensitiveAttr);
	return status;
}

static sgx_status_t SGX_CDECL sgx_extractData(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_extractData_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_extractData_t* ms = SGX_CAST(ms_extractData_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	extractData(ms->ms_tableID, ms->ms_nRecords);


	return status;
}

static sgx_status_t SGX_CDECL sgx_createPDS(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_createPDS_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_createPDS_t* ms = SGX_CAST(ms_createPDS_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = createPDS(ms->ms_tableID);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_initJoinPDS(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_initJoinPDS_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_initJoinPDS_t* ms = SGX_CAST(ms_initJoinPDS_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = initJoinPDS(ms->ms_pfJoin, ms->ms_selectJoin);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_initHTree(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_initHTree_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_initHTree_t* ms = SGX_CAST(ms_initHTree_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = initHTree(ms->ms_pfJoin, ms->ms_constant, ms->ms_ind);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_createJoinPDS(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_createJoinPDS_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_createJoinPDS_t* ms = SGX_CAST(ms_createJoinPDS_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	createJoinPDS(ms->ms_structureID, ms->ms_tableID1, ms->ms_tableID2);


	return status;
}

static sgx_status_t SGX_CDECL sgx_createSelectJoinPDS(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_createSelectJoinPDS_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_createSelectJoinPDS_t* ms = SGX_CAST(ms_createSelectJoinPDS_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	createSelectJoinPDS(ms->ms_structureID, ms->ms_tableID1, ms->ms_tableID2);


	return status;
}

static sgx_status_t SGX_CDECL sgx_createJoinBuckets(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_createJoinBuckets_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_createJoinBuckets_t* ms = SGX_CAST(ms_createJoinBuckets_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	createJoinBuckets(ms->ms_structureID, ms->ms_tableID1, ms->ms_tableID2, ms->ms_pfJoin);


	return status;
}

static sgx_status_t SGX_CDECL sgx_createCZSC(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_createCZSC_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_createCZSC_t* ms = SGX_CAST(ms_createCZSC_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_eData_t1 = ms->ms_eData_t1;
	int _tmp_inSize_t1 = ms->ms_inSize_t1;
	size_t _len_eData_t1 = _tmp_inSize_t1;
	char* _in_eData_t1 = NULL;
	char* _tmp_eData_t2 = ms->ms_eData_t2;
	int _tmp_inSize_t2 = ms->ms_inSize_t2;
	size_t _len_eData_t2 = _tmp_inSize_t2;
	char* _in_eData_t2 = NULL;

	CHECK_UNIQUE_POINTER(_tmp_eData_t1, _len_eData_t1);
	CHECK_UNIQUE_POINTER(_tmp_eData_t2, _len_eData_t2);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_eData_t1 != NULL && _len_eData_t1 != 0) {
		if ( _len_eData_t1 % sizeof(*_tmp_eData_t1) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_eData_t1 = (char*)malloc(_len_eData_t1);
		if (_in_eData_t1 == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_eData_t1, _len_eData_t1, _tmp_eData_t1, _len_eData_t1)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_eData_t2 != NULL && _len_eData_t2 != 0) {
		if ( _len_eData_t2 % sizeof(*_tmp_eData_t2) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_eData_t2 = (char*)malloc(_len_eData_t2);
		if (_in_eData_t2 == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_eData_t2, _len_eData_t2, _tmp_eData_t2, _len_eData_t2)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	createCZSC(ms->ms_tableID1, _in_eData_t1, _tmp_inSize_t1, ms->ms_join_idx1, ms->ms_tableID2, _in_eData_t2, _tmp_inSize_t2, ms->ms_join_idx2, ms->ms_pfJoin);

err:
	if (_in_eData_t1) free(_in_eData_t1);
	if (_in_eData_t2) free(_in_eData_t2);
	return status;
}

static sgx_status_t SGX_CDECL sgx_buildIndex(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_buildIndex_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_buildIndex_t* ms = SGX_CAST(ms_buildIndex_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_str = ms->ms_str;
	size_t _len_str = ms->ms_str_len ;
	char* _in_str = NULL;
	int _in_retval;

	CHECK_UNIQUE_POINTER(_tmp_str, _len_str);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_str != NULL && _len_str != 0) {
		_in_str = (char*)malloc(_len_str);
		if (_in_str == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_str, _len_str, _tmp_str, _len_str)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_str[_len_str - 1] = '\0';
		if (_len_str != strlen(_in_str) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	_in_retval = buildIndex(ms->ms_structureID, ms->ms_idx_dim, (const char*)_in_str, ms->ms_jtype);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	if (_in_str) free(_in_str);
	return status;
}

static sgx_status_t SGX_CDECL sgx_restoreIndex(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_restoreIndex_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_restoreIndex_t* ms = SGX_CAST(ms_restoreIndex_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_s_idxBinStart = ms->ms_s_idxBinStart;
	uint _tmp_size1 = ms->ms_size1;
	size_t _len_s_idxBinStart = _tmp_size1;
	char* _in_s_idxBinStart = NULL;
	char* _tmp_s_idxBinEnd = ms->ms_s_idxBinEnd;
	uint _tmp_size2 = ms->ms_size2;
	size_t _len_s_idxBinEnd = _tmp_size2;
	char* _in_s_idxBinEnd = NULL;
	int _in_retval;

	CHECK_UNIQUE_POINTER(_tmp_s_idxBinStart, _len_s_idxBinStart);
	CHECK_UNIQUE_POINTER(_tmp_s_idxBinEnd, _len_s_idxBinEnd);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_s_idxBinStart != NULL && _len_s_idxBinStart != 0) {
		if ( _len_s_idxBinStart % sizeof(*_tmp_s_idxBinStart) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_s_idxBinStart = (char*)malloc(_len_s_idxBinStart);
		if (_in_s_idxBinStart == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_s_idxBinStart, _len_s_idxBinStart, _tmp_s_idxBinStart, _len_s_idxBinStart)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_s_idxBinEnd != NULL && _len_s_idxBinEnd != 0) {
		if ( _len_s_idxBinEnd % sizeof(*_tmp_s_idxBinEnd) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_s_idxBinEnd = (char*)malloc(_len_s_idxBinEnd);
		if (_in_s_idxBinEnd == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_s_idxBinEnd, _len_s_idxBinEnd, _tmp_s_idxBinEnd, _len_s_idxBinEnd)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	_in_retval = restoreIndex(ms->ms_structureID, ms->ms_idx_dim, _in_s_idxBinStart, _tmp_size1, _in_s_idxBinEnd, _tmp_size2);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	if (_in_s_idxBinStart) free(_in_s_idxBinStart);
	if (_in_s_idxBinEnd) free(_in_s_idxBinEnd);
	return status;
}

static sgx_status_t SGX_CDECL sgx_restoreJoinMeta(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_restoreJoinMeta_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_restoreJoinMeta_t* ms = SGX_CAST(ms_restoreJoinMeta_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_encryptedBin = ms->ms_encryptedBin;
	uint _tmp_dsize = ms->ms_dsize;
	size_t _len_encryptedBin = _tmp_dsize;
	char* _in_encryptedBin = NULL;

	CHECK_UNIQUE_POINTER(_tmp_encryptedBin, _len_encryptedBin);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_encryptedBin != NULL && _len_encryptedBin != 0) {
		if ( _len_encryptedBin % sizeof(*_tmp_encryptedBin) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_encryptedBin = (char*)malloc(_len_encryptedBin);
		if (_in_encryptedBin == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_encryptedBin, _len_encryptedBin, _tmp_encryptedBin, _len_encryptedBin)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	restoreJoinMeta(ms->ms_tableID, ms->ms_binID, ms->ms_pos, ms->ms_join_idx, _in_encryptedBin, _tmp_dsize);

err:
	if (_in_encryptedBin) free(_in_encryptedBin);
	return status;
}

static sgx_status_t SGX_CDECL sgx_idxSelect(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_idxSelect_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_idxSelect_t* ms = SGX_CAST(ms_idxSelect_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_s_bins = ms->ms_s_bins;
	uint _tmp_dsize = ms->ms_dsize;
	size_t _len_s_bins = _tmp_dsize;
	char* _in_s_bins = NULL;
	char* _tmp_s_records = ms->ms_s_records;
	uint _tmp_rdsize = ms->ms_rdsize;
	size_t _len_s_records = _tmp_rdsize;
	char* _in_s_records = NULL;
	int _in_retval;

	CHECK_UNIQUE_POINTER(_tmp_s_bins, _len_s_bins);
	CHECK_UNIQUE_POINTER(_tmp_s_records, _len_s_records);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_s_bins != NULL && _len_s_bins != 0) {
		if ( _len_s_bins % sizeof(*_tmp_s_bins) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_s_bins = (char*)malloc(_len_s_bins)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_s_bins, 0, _len_s_bins);
	}
	if (_tmp_s_records != NULL && _len_s_records != 0) {
		if ( _len_s_records % sizeof(*_tmp_s_records) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_s_records = (char*)malloc(_len_s_records)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_s_records, 0, _len_s_records);
	}
	_in_retval = idxSelect(ms->ms_structureID, ms->ms_idx_dim, ms->ms_queryStart, ms->ms_queryEnd, _in_s_bins, _tmp_dsize, _in_s_records, _tmp_rdsize, ms->ms_jtype);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}
	if (_in_s_bins) {
		if (MEMCPY_S(_tmp_s_bins, _len_s_bins, _in_s_bins, _len_s_bins)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_s_records) {
		if (MEMCPY_S(_tmp_s_records, _len_s_records, _in_s_records, _len_s_records)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_s_bins) free(_in_s_bins);
	if (_in_s_records) free(_in_s_records);
	return status;
}

static sgx_status_t SGX_CDECL sgx_innerJoin(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_innerJoin_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_innerJoin_t* ms = SGX_CAST(ms_innerJoin_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = innerJoin(ms->ms_structureID, ms->ms_tableID1, ms->ms_tableID2, ms->ms_writeID, ms->ms_jtype, ms->ms_qs, ms->ms_qe, ms->ms_jattr);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_pfJoin(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_pfJoin_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_pfJoin_t* ms = SGX_CAST(ms_pfJoin_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = pfJoin(ms->ms_structureID, ms->ms_tableID1, ms->ms_tableID2, ms->ms_writeID, ms->ms_jtype);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_czscJoin(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_czscJoin_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_czscJoin_t* ms = SGX_CAST(ms_czscJoin_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = czscJoin(ms->ms_tableID1, ms->ms_tableID2, ms->ms_writeID, ms->ms_pfJoin);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_joinCompact(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_joinCompact_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_joinCompact_t* ms = SGX_CAST(ms_joinCompact_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	joinCompact(ms->ms_structureID, ms->ms_writeID, ms->ms_us_size, ms->ms_jtype);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_char(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_char_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_char_t* ms = SGX_CAST(ms_ecall_type_char_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_char(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_int(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_int_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_int_t* ms = SGX_CAST(ms_ecall_type_int_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_int(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_float(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_float_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_float_t* ms = SGX_CAST(ms_ecall_type_float_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_float(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_double(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_double_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_double_t* ms = SGX_CAST(ms_ecall_type_double_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_double(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_size_t(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_size_t_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_size_t_t* ms = SGX_CAST(ms_ecall_type_size_t_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_size_t(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_wchar_t(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_wchar_t_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_wchar_t_t* ms = SGX_CAST(ms_ecall_type_wchar_t_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_wchar_t(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_struct(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_struct_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_struct_t* ms = SGX_CAST(ms_ecall_type_struct_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_type_struct(ms->ms_val);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_type_enum_union(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_type_enum_union_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_type_enum_union_t* ms = SGX_CAST(ms_ecall_type_enum_union_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	union union_foo_t* _tmp_val2 = ms->ms_val2;


	ecall_type_enum_union(ms->ms_val1, _tmp_val2);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_user_check(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_user_check_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_user_check_t* ms = SGX_CAST(ms_ecall_pointer_user_check_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_val = ms->ms_val;
	size_t _in_retval;


	_in_retval = ecall_pointer_user_check(_tmp_val, ms->ms_sz);
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_in(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_in_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_in_t* ms = SGX_CAST(ms_ecall_pointer_in_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_val = ms->ms_val;
	size_t _len_val = sizeof(int);
	int* _in_val = NULL;

	CHECK_UNIQUE_POINTER(_tmp_val, _len_val);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_val != NULL && _len_val != 0) {
		if ( _len_val % sizeof(*_tmp_val) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_val = (int*)malloc(_len_val);
		if (_in_val == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_val, _len_val, _tmp_val, _len_val)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_pointer_in(_in_val);

err:
	if (_in_val) free(_in_val);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_out(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_out_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_out_t* ms = SGX_CAST(ms_ecall_pointer_out_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_val = ms->ms_val;
	size_t _len_val = sizeof(int);
	int* _in_val = NULL;

	CHECK_UNIQUE_POINTER(_tmp_val, _len_val);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_val != NULL && _len_val != 0) {
		if ( _len_val % sizeof(*_tmp_val) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_val = (int*)malloc(_len_val)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_val, 0, _len_val);
	}
	ecall_pointer_out(_in_val);
	if (_in_val) {
		if (MEMCPY_S(_tmp_val, _len_val, _in_val, _len_val)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_val) free(_in_val);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_in_out(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_in_out_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_in_out_t* ms = SGX_CAST(ms_ecall_pointer_in_out_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_val = ms->ms_val;
	size_t _len_val = sizeof(int);
	int* _in_val = NULL;

	CHECK_UNIQUE_POINTER(_tmp_val, _len_val);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_val != NULL && _len_val != 0) {
		if ( _len_val % sizeof(*_tmp_val) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_val = (int*)malloc(_len_val);
		if (_in_val == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_val, _len_val, _tmp_val, _len_val)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_pointer_in_out(_in_val);
	if (_in_val) {
		if (MEMCPY_S(_tmp_val, _len_val, _in_val, _len_val)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_val) free(_in_val);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_string(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_string_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_string_t* ms = SGX_CAST(ms_ecall_pointer_string_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_str = ms->ms_str;
	size_t _len_str = ms->ms_str_len ;
	char* _in_str = NULL;

	CHECK_UNIQUE_POINTER(_tmp_str, _len_str);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_str != NULL && _len_str != 0) {
		_in_str = (char*)malloc(_len_str);
		if (_in_str == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_str, _len_str, _tmp_str, _len_str)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_str[_len_str - 1] = '\0';
		if (_len_str != strlen(_in_str) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	ecall_pointer_string(_in_str);
	if (_in_str)
	{
		_in_str[_len_str - 1] = '\0';
		_len_str = strlen(_in_str) + 1;
		if (MEMCPY_S((void*)_tmp_str, _len_str, _in_str, _len_str)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_str) free(_in_str);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_string_const(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_string_const_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_string_const_t* ms = SGX_CAST(ms_ecall_pointer_string_const_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_str = ms->ms_str;
	size_t _len_str = ms->ms_str_len ;
	char* _in_str = NULL;

	CHECK_UNIQUE_POINTER(_tmp_str, _len_str);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_str != NULL && _len_str != 0) {
		_in_str = (char*)malloc(_len_str);
		if (_in_str == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_str, _len_str, _tmp_str, _len_str)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_str[_len_str - 1] = '\0';
		if (_len_str != strlen(_in_str) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	ecall_pointer_string_const((const char*)_in_str);

err:
	if (_in_str) free(_in_str);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_size(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_size_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_size_t* ms = SGX_CAST(ms_ecall_pointer_size_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_ptr = ms->ms_ptr;
	size_t _tmp_len = ms->ms_len;
	size_t _len_ptr = _tmp_len;
	void* _in_ptr = NULL;

	CHECK_UNIQUE_POINTER(_tmp_ptr, _len_ptr);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_ptr != NULL && _len_ptr != 0) {
		_in_ptr = (void*)malloc(_len_ptr);
		if (_in_ptr == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_ptr, _len_ptr, _tmp_ptr, _len_ptr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_pointer_size(_in_ptr, _tmp_len);
	if (_in_ptr) {
		if (MEMCPY_S(_tmp_ptr, _len_ptr, _in_ptr, _len_ptr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_ptr) free(_in_ptr);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_count(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_count_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_count_t* ms = SGX_CAST(ms_ecall_pointer_count_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_arr = ms->ms_arr;
	int _tmp_cnt = ms->ms_cnt;
	size_t _len_arr = _tmp_cnt * sizeof(int);
	int* _in_arr = NULL;

	if (sizeof(*_tmp_arr) != 0 &&
		(size_t)_tmp_cnt > (SIZE_MAX / sizeof(*_tmp_arr))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_arr, _len_arr);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_arr != NULL && _len_arr != 0) {
		if ( _len_arr % sizeof(*_tmp_arr) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_arr = (int*)malloc(_len_arr);
		if (_in_arr == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_arr, _len_arr, _tmp_arr, _len_arr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_pointer_count(_in_arr, _tmp_cnt);
	if (_in_arr) {
		if (MEMCPY_S(_tmp_arr, _len_arr, _in_arr, _len_arr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_arr) free(_in_arr);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_pointer_isptr_readonly(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_pointer_isptr_readonly_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_pointer_isptr_readonly_t* ms = SGX_CAST(ms_ecall_pointer_isptr_readonly_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	buffer_t _tmp_buf = ms->ms_buf;
	size_t _tmp_len = ms->ms_len;
	size_t _len_buf = _tmp_len;
	buffer_t _in_buf = NULL;

	CHECK_UNIQUE_POINTER(_tmp_buf, _len_buf);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_buf != NULL && _len_buf != 0) {
		_in_buf = (buffer_t)malloc(_len_buf);
		if (_in_buf == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s((void*)_in_buf, _len_buf, _tmp_buf, _len_buf)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_pointer_isptr_readonly(_in_buf, _tmp_len);

err:
	if (_in_buf) free((void*)_in_buf);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ocall_pointer_attr(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ocall_pointer_attr();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_array_user_check(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_array_user_check_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_array_user_check_t* ms = SGX_CAST(ms_ecall_array_user_check_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_arr = ms->ms_arr;


	ecall_array_user_check(_tmp_arr);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_array_in(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_array_in_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_array_in_t* ms = SGX_CAST(ms_ecall_array_in_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_arr = ms->ms_arr;
	size_t _len_arr = 4 * sizeof(int);
	int* _in_arr = NULL;

	CHECK_UNIQUE_POINTER(_tmp_arr, _len_arr);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_arr != NULL && _len_arr != 0) {
		if ( _len_arr % sizeof(*_tmp_arr) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_arr = (int*)malloc(_len_arr);
		if (_in_arr == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_arr, _len_arr, _tmp_arr, _len_arr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_array_in(_in_arr);

err:
	if (_in_arr) free(_in_arr);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_array_out(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_array_out_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_array_out_t* ms = SGX_CAST(ms_ecall_array_out_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_arr = ms->ms_arr;
	size_t _len_arr = 4 * sizeof(int);
	int* _in_arr = NULL;

	CHECK_UNIQUE_POINTER(_tmp_arr, _len_arr);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_arr != NULL && _len_arr != 0) {
		if ( _len_arr % sizeof(*_tmp_arr) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_arr = (int*)malloc(_len_arr)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_arr, 0, _len_arr);
	}
	ecall_array_out(_in_arr);
	if (_in_arr) {
		if (MEMCPY_S(_tmp_arr, _len_arr, _in_arr, _len_arr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_arr) free(_in_arr);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_array_in_out(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_array_in_out_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_array_in_out_t* ms = SGX_CAST(ms_ecall_array_in_out_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_arr = ms->ms_arr;
	size_t _len_arr = 4 * sizeof(int);
	int* _in_arr = NULL;

	CHECK_UNIQUE_POINTER(_tmp_arr, _len_arr);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_arr != NULL && _len_arr != 0) {
		if ( _len_arr % sizeof(*_tmp_arr) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_arr = (int*)malloc(_len_arr);
		if (_in_arr == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_arr, _len_arr, _tmp_arr, _len_arr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	ecall_array_in_out(_in_arr);
	if (_in_arr) {
		if (MEMCPY_S(_tmp_arr, _len_arr, _in_arr, _len_arr)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_arr) free(_in_arr);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_array_isary(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_array_isary_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_array_isary_t* ms = SGX_CAST(ms_ecall_array_isary_t*, pms);
	sgx_status_t status = SGX_SUCCESS;


	ecall_array_isary((ms->ms_arr != NULL) ? (*ms->ms_arr) : NULL);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_function_calling_convs(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_function_calling_convs();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_function_public(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_function_public();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_function_private(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_function_private_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_function_private_t* ms = SGX_CAST(ms_ecall_function_private_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int _in_retval;


	_in_retval = ecall_function_private();
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_malloc_free(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_malloc_free();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_sgx_cpuid(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_sgx_cpuid_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_sgx_cpuid_t* ms = SGX_CAST(ms_ecall_sgx_cpuid_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_cpuinfo = ms->ms_cpuinfo;
	size_t _len_cpuinfo = 4 * sizeof(int);
	int* _in_cpuinfo = NULL;

	CHECK_UNIQUE_POINTER(_tmp_cpuinfo, _len_cpuinfo);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_cpuinfo != NULL && _len_cpuinfo != 0) {
		if ( _len_cpuinfo % sizeof(*_tmp_cpuinfo) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_cpuinfo = (int*)malloc(_len_cpuinfo)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_cpuinfo, 0, _len_cpuinfo);
	}
	ecall_sgx_cpuid(_in_cpuinfo, ms->ms_leaf);
	if (_in_cpuinfo) {
		if (MEMCPY_S(_tmp_cpuinfo, _len_cpuinfo, _in_cpuinfo, _len_cpuinfo)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_cpuinfo) free(_in_cpuinfo);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_exception(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_exception();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_map(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_map();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_increase_counter(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_increase_counter_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_increase_counter_t* ms = SGX_CAST(ms_ecall_increase_counter_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	size_t _in_retval;


	_in_retval = ecall_increase_counter();
	if (MEMCPY_S(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_producer(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_producer();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_consumer(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ecall_consumer();
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[50];
} g_ecall_table = {
	50,
	{
		{(void*)(uintptr_t)sgx_extractMeta, 0, 0},
		{(void*)(uintptr_t)sgx_extractData, 0, 0},
		{(void*)(uintptr_t)sgx_createPDS, 0, 0},
		{(void*)(uintptr_t)sgx_initJoinPDS, 0, 0},
		{(void*)(uintptr_t)sgx_initHTree, 0, 0},
		{(void*)(uintptr_t)sgx_createJoinPDS, 0, 0},
		{(void*)(uintptr_t)sgx_createSelectJoinPDS, 0, 0},
		{(void*)(uintptr_t)sgx_createJoinBuckets, 0, 0},
		{(void*)(uintptr_t)sgx_createCZSC, 0, 0},
		{(void*)(uintptr_t)sgx_buildIndex, 0, 0},
		{(void*)(uintptr_t)sgx_restoreIndex, 0, 0},
		{(void*)(uintptr_t)sgx_restoreJoinMeta, 0, 0},
		{(void*)(uintptr_t)sgx_idxSelect, 0, 0},
		{(void*)(uintptr_t)sgx_innerJoin, 0, 0},
		{(void*)(uintptr_t)sgx_pfJoin, 0, 0},
		{(void*)(uintptr_t)sgx_czscJoin, 0, 0},
		{(void*)(uintptr_t)sgx_joinCompact, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_char, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_int, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_float, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_double, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_size_t, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_wchar_t, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_struct, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_type_enum_union, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_user_check, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_in, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_out, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_in_out, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_string, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_string_const, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_size, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_count, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_pointer_isptr_readonly, 0, 0},
		{(void*)(uintptr_t)sgx_ocall_pointer_attr, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_array_user_check, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_array_in, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_array_out, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_array_in_out, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_array_isary, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_function_calling_convs, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_function_public, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_function_private, 1, 0},
		{(void*)(uintptr_t)sgx_ecall_malloc_free, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_sgx_cpuid, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_exception, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_map, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_increase_counter, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_producer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_consumer, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[24][50];
} g_dyn_entry_table = {
	24,
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_print_string(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_print_string_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_string_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(str, _len_str);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (str != NULL) ? _len_str : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_string_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_string_t));
	ocalloc_size -= sizeof(ms_ocall_print_string_t);

	if (str != NULL) {
		if (MEMCPY_S(&ms->ms_str, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_str % sizeof(*str) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, str, _len_str)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_str);
		ocalloc_size -= _len_str;
	} else {
		ms->ms_str = NULL;
	}

	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL expand_table_OCALL(int tableID, uint noiseSum, char* noise, uint nsize)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_noise = nsize;

	ms_expand_table_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_expand_table_OCALL_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(noise, _len_noise);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (noise != NULL) ? _len_noise : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_expand_table_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_expand_table_OCALL_t));
	ocalloc_size -= sizeof(ms_expand_table_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_noiseSum, sizeof(ms->ms_noiseSum), &noiseSum, sizeof(noiseSum))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (noise != NULL) {
		if (MEMCPY_S(&ms->ms_noise, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_noise % sizeof(*noise) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, noise, _len_noise)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_noise);
		ocalloc_size -= _len_noise;
	} else {
		ms->ms_noise = NULL;
	}

	if (MEMCPY_S(&ms->ms_nsize, sizeof(ms->ms_nsize), &nsize, sizeof(nsize))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(1, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL readDataBlock_OCALL(int* retval, int tableID, int readBinID, int pos, char* dblock, int dsize)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_dblock = dsize;

	ms_readDataBlock_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_readDataBlock_OCALL_t);
	void *__tmp = NULL;

	void *__tmp_dblock = NULL;

	CHECK_ENCLAVE_POINTER(dblock, _len_dblock);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (dblock != NULL) ? _len_dblock : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_readDataBlock_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_readDataBlock_OCALL_t));
	ocalloc_size -= sizeof(ms_readDataBlock_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_readBinID, sizeof(ms->ms_readBinID), &readBinID, sizeof(readBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_pos, sizeof(ms->ms_pos), &pos, sizeof(pos))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (dblock != NULL) {
		if (MEMCPY_S(&ms->ms_dblock, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_dblock = __tmp;
		if (_len_dblock % sizeof(*dblock) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		MEMSET(__tmp_dblock, 0, _len_dblock);
		__tmp = (void *)((size_t)__tmp + _len_dblock);
		ocalloc_size -= _len_dblock;
	} else {
		ms->ms_dblock = NULL;
	}

	if (MEMCPY_S(&ms->ms_dsize, sizeof(ms->ms_dsize), &dsize, sizeof(dsize))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(2, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
		if (dblock) {
			if (memcpy_s((void*)dblock, _len_dblock, __tmp_dblock, _len_dblock)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL writeDataBlock_OCALL(int tableID, int writeBinID, uint pos, char* dblock, uint dsize)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_dblock = dsize;

	ms_writeDataBlock_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_writeDataBlock_OCALL_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(dblock, _len_dblock);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (dblock != NULL) ? _len_dblock : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_writeDataBlock_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_writeDataBlock_OCALL_t));
	ocalloc_size -= sizeof(ms_writeDataBlock_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_writeBinID, sizeof(ms->ms_writeBinID), &writeBinID, sizeof(writeBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_pos, sizeof(ms->ms_pos), &pos, sizeof(pos))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (dblock != NULL) {
		if (MEMCPY_S(&ms->ms_dblock, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_dblock % sizeof(*dblock) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, dblock, _len_dblock)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_dblock);
		ocalloc_size -= _len_dblock;
	} else {
		ms->ms_dblock = NULL;
	}

	if (MEMCPY_S(&ms->ms_dsize, sizeof(ms->ms_dsize), &dsize, sizeof(dsize))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(3, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL copy_bin_OCALL(int tableID, int readBinID, int writeBinID, int startPos, int endPos)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_copy_bin_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_copy_bin_OCALL_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_copy_bin_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_copy_bin_OCALL_t));
	ocalloc_size -= sizeof(ms_copy_bin_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_readBinID, sizeof(ms->ms_readBinID), &readBinID, sizeof(readBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_writeBinID, sizeof(ms->ms_writeBinID), &writeBinID, sizeof(writeBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_startPos, sizeof(ms->ms_startPos), &startPos, sizeof(startPos))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_endPos, sizeof(ms->ms_endPos), &endPos, sizeof(endPos))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(4, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL move_resize_bin_OCALL(int tableID, int srcBinID, int destBinID, int size)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_move_resize_bin_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_move_resize_bin_OCALL_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_move_resize_bin_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_move_resize_bin_OCALL_t));
	ocalloc_size -= sizeof(ms_move_resize_bin_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_srcBinID, sizeof(ms->ms_srcBinID), &srcBinID, sizeof(srcBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_destBinID, sizeof(ms->ms_destBinID), &destBinID, sizeof(destBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_size, sizeof(ms->ms_size), &size, sizeof(size))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(5, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL move_resize_bin_totable_OCALL(int tableID, int srcBinID, int destBinID, int size)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_move_resize_bin_totable_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_move_resize_bin_totable_OCALL_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_move_resize_bin_totable_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_move_resize_bin_totable_OCALL_t));
	ocalloc_size -= sizeof(ms_move_resize_bin_totable_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_srcBinID, sizeof(ms->ms_srcBinID), &srcBinID, sizeof(srcBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_destBinID, sizeof(ms->ms_destBinID), &destBinID, sizeof(destBinID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_size, sizeof(ms->ms_size), &size, sizeof(size))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(6, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL initBinStorage_OCALL(int tableID, int binID)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_initBinStorage_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_initBinStorage_OCALL_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_initBinStorage_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_initBinStorage_OCALL_t));
	ocalloc_size -= sizeof(ms_initBinStorage_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_binID, sizeof(ms->ms_binID), &binID, sizeof(binID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(7, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL delete_bin_OCALL(int tableID, int binID)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_delete_bin_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_delete_bin_OCALL_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_delete_bin_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_delete_bin_OCALL_t));
	ocalloc_size -= sizeof(ms_delete_bin_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_binID, sizeof(ms->ms_binID), &binID, sizeof(binID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(8, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL delete_table_OCALL(int tableID)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_delete_table_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_delete_table_OCALL_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_delete_table_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_delete_table_OCALL_t));
	ocalloc_size -= sizeof(ms_delete_table_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(9, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL collect_resize_bins_OCALL(int tableID, char* leaves_id, uint dsize, char* binSizes, uint bsize)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_leaves_id = dsize;
	size_t _len_binSizes = bsize;

	ms_collect_resize_bins_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_collect_resize_bins_OCALL_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(leaves_id, _len_leaves_id);
	CHECK_ENCLAVE_POINTER(binSizes, _len_binSizes);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (leaves_id != NULL) ? _len_leaves_id : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (binSizes != NULL) ? _len_binSizes : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_collect_resize_bins_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_collect_resize_bins_OCALL_t));
	ocalloc_size -= sizeof(ms_collect_resize_bins_OCALL_t);

	if (MEMCPY_S(&ms->ms_tableID, sizeof(ms->ms_tableID), &tableID, sizeof(tableID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (leaves_id != NULL) {
		if (MEMCPY_S(&ms->ms_leaves_id, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_leaves_id % sizeof(*leaves_id) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, leaves_id, _len_leaves_id)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_leaves_id);
		ocalloc_size -= _len_leaves_id;
	} else {
		ms->ms_leaves_id = NULL;
	}

	if (MEMCPY_S(&ms->ms_dsize, sizeof(ms->ms_dsize), &dsize, sizeof(dsize))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (binSizes != NULL) {
		if (MEMCPY_S(&ms->ms_binSizes, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_binSizes % sizeof(*binSizes) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, binSizes, _len_binSizes)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_binSizes);
		ocalloc_size -= _len_binSizes;
	} else {
		ms->ms_binSizes = NULL;
	}

	if (MEMCPY_S(&ms->ms_bsize, sizeof(ms->ms_bsize), &bsize, sizeof(bsize))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(10, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL write_index_OCALL(int structureID, char* s_idxBinStart, uint size1, char* s_idxBinEnd, uint size2, const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_s_idxBinStart = size1;
	size_t _len_s_idxBinEnd = size2;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_write_index_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_write_index_OCALL_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(s_idxBinStart, _len_s_idxBinStart);
	CHECK_ENCLAVE_POINTER(s_idxBinEnd, _len_s_idxBinEnd);
	CHECK_ENCLAVE_POINTER(str, _len_str);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (s_idxBinStart != NULL) ? _len_s_idxBinStart : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (s_idxBinEnd != NULL) ? _len_s_idxBinEnd : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (str != NULL) ? _len_str : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_write_index_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_write_index_OCALL_t));
	ocalloc_size -= sizeof(ms_write_index_OCALL_t);

	if (MEMCPY_S(&ms->ms_structureID, sizeof(ms->ms_structureID), &structureID, sizeof(structureID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (s_idxBinStart != NULL) {
		if (MEMCPY_S(&ms->ms_s_idxBinStart, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_s_idxBinStart % sizeof(*s_idxBinStart) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, s_idxBinStart, _len_s_idxBinStart)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_s_idxBinStart);
		ocalloc_size -= _len_s_idxBinStart;
	} else {
		ms->ms_s_idxBinStart = NULL;
	}

	if (MEMCPY_S(&ms->ms_size1, sizeof(ms->ms_size1), &size1, sizeof(size1))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (s_idxBinEnd != NULL) {
		if (MEMCPY_S(&ms->ms_s_idxBinEnd, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_s_idxBinEnd % sizeof(*s_idxBinEnd) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, s_idxBinEnd, _len_s_idxBinEnd)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_s_idxBinEnd);
		ocalloc_size -= _len_s_idxBinEnd;
	} else {
		ms->ms_s_idxBinEnd = NULL;
	}

	if (MEMCPY_S(&ms->ms_size2, sizeof(ms->ms_size2), &size2, sizeof(size2))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (str != NULL) {
		if (MEMCPY_S(&ms->ms_str, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_str % sizeof(*str) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, str, _len_str)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_str);
		ocalloc_size -= _len_str;
	} else {
		ms->ms_str = NULL;
	}

	status = sgx_ocall(11, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL write_join_output_OCALL(int writeID, char* res, uint dsize)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_res = dsize;

	ms_write_join_output_OCALL_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_write_join_output_OCALL_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(res, _len_res);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (res != NULL) ? _len_res : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_write_join_output_OCALL_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_write_join_output_OCALL_t));
	ocalloc_size -= sizeof(ms_write_join_output_OCALL_t);

	if (MEMCPY_S(&ms->ms_writeID, sizeof(ms->ms_writeID), &writeID, sizeof(writeID))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (res != NULL) {
		if (MEMCPY_S(&ms->ms_res, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_res % sizeof(*res) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, res, _len_res)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_res);
		ocalloc_size -= _len_res;
	} else {
		ms->ms_res = NULL;
	}

	if (MEMCPY_S(&ms->ms_dsize, sizeof(ms->ms_dsize), &dsize, sizeof(dsize))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(12, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_pointer_user_check(int* val)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_ocall_pointer_user_check_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_pointer_user_check_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_pointer_user_check_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_pointer_user_check_t));
	ocalloc_size -= sizeof(ms_ocall_pointer_user_check_t);

	if (MEMCPY_S(&ms->ms_val, sizeof(ms->ms_val), &val, sizeof(val))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(13, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_pointer_in(int* val)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_val = sizeof(int);

	ms_ocall_pointer_in_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_pointer_in_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(val, _len_val);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (val != NULL) ? _len_val : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_pointer_in_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_pointer_in_t));
	ocalloc_size -= sizeof(ms_ocall_pointer_in_t);

	if (val != NULL) {
		if (MEMCPY_S(&ms->ms_val, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_val % sizeof(*val) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, val, _len_val)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_val);
		ocalloc_size -= _len_val;
	} else {
		ms->ms_val = NULL;
	}

	status = sgx_ocall(14, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_pointer_out(int* val)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_val = sizeof(int);

	ms_ocall_pointer_out_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_pointer_out_t);
	void *__tmp = NULL;

	void *__tmp_val = NULL;

	CHECK_ENCLAVE_POINTER(val, _len_val);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (val != NULL) ? _len_val : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_pointer_out_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_pointer_out_t));
	ocalloc_size -= sizeof(ms_ocall_pointer_out_t);

	if (val != NULL) {
		if (MEMCPY_S(&ms->ms_val, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_val = __tmp;
		if (_len_val % sizeof(*val) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		MEMSET(__tmp_val, 0, _len_val);
		__tmp = (void *)((size_t)__tmp + _len_val);
		ocalloc_size -= _len_val;
	} else {
		ms->ms_val = NULL;
	}

	status = sgx_ocall(15, ms);

	if (status == SGX_SUCCESS) {
		if (val) {
			if (memcpy_s((void*)val, _len_val, __tmp_val, _len_val)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_pointer_in_out(int* val)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_val = sizeof(int);

	ms_ocall_pointer_in_out_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_pointer_in_out_t);
	void *__tmp = NULL;

	void *__tmp_val = NULL;

	CHECK_ENCLAVE_POINTER(val, _len_val);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (val != NULL) ? _len_val : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_pointer_in_out_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_pointer_in_out_t));
	ocalloc_size -= sizeof(ms_ocall_pointer_in_out_t);

	if (val != NULL) {
		if (MEMCPY_S(&ms->ms_val, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_val = __tmp;
		if (_len_val % sizeof(*val) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, val, _len_val)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_val);
		ocalloc_size -= _len_val;
	} else {
		ms->ms_val = NULL;
	}

	status = sgx_ocall(16, ms);

	if (status == SGX_SUCCESS) {
		if (val) {
			if (memcpy_s((void*)val, _len_val, __tmp_val, _len_val)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL memccpy(void** retval, void* dest, const void* src, int val, size_t len)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_dest = len;
	size_t _len_src = len;

	ms_memccpy_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_memccpy_t);
	void *__tmp = NULL;

	void *__tmp_dest = NULL;

	CHECK_ENCLAVE_POINTER(dest, _len_dest);
	CHECK_ENCLAVE_POINTER(src, _len_src);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (dest != NULL) ? _len_dest : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (src != NULL) ? _len_src : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_memccpy_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_memccpy_t));
	ocalloc_size -= sizeof(ms_memccpy_t);

	if (dest != NULL) {
		if (MEMCPY_S(&ms->ms_dest, sizeof(void*), &__tmp, sizeof(void*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_dest = __tmp;
		if (MEMCPY_S(__tmp, ocalloc_size, dest, _len_dest)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_dest);
		ocalloc_size -= _len_dest;
	} else {
		ms->ms_dest = NULL;
	}

	if (src != NULL) {
		if (MEMCPY_S(&ms->ms_src, sizeof(const void*), &__tmp, sizeof(const void*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, src, _len_src)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_src);
		ocalloc_size -= _len_src;
	} else {
		ms->ms_src = NULL;
	}

	if (MEMCPY_S(&ms->ms_val, sizeof(ms->ms_val), &val, sizeof(val))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_len, sizeof(ms->ms_len), &len, sizeof(len))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(17, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
		if (dest) {
			if (memcpy_s((void*)dest, _len_dest, __tmp_dest, _len_dest)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_function_allow(void)
{
	sgx_status_t status = SGX_SUCCESS;
	status = sgx_ocall(18, NULL);

	return status;
}
sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(int);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	void *__tmp_cpuinfo = NULL;

	CHECK_ENCLAVE_POINTER(cpuinfo, _len_cpuinfo);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (cpuinfo != NULL) ? _len_cpuinfo : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));
	ocalloc_size -= sizeof(ms_sgx_oc_cpuidex_t);

	if (cpuinfo != NULL) {
		if (MEMCPY_S(&ms->ms_cpuinfo, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_cpuinfo = __tmp;
		if (_len_cpuinfo % sizeof(*cpuinfo) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		MEMSET(__tmp_cpuinfo, 0, _len_cpuinfo);
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
		ocalloc_size -= _len_cpuinfo;
	} else {
		ms->ms_cpuinfo = NULL;
	}

	if (MEMCPY_S(&ms->ms_leaf, sizeof(ms->ms_leaf), &leaf, sizeof(leaf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_subleaf, sizeof(ms->ms_subleaf), &subleaf, sizeof(subleaf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(19, ms);

	if (status == SGX_SUCCESS) {
		if (cpuinfo) {
			if (memcpy_s((void*)cpuinfo, _len_cpuinfo, __tmp_cpuinfo, _len_cpuinfo)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);

	if (MEMCPY_S(&ms->ms_self, sizeof(ms->ms_self), &self, sizeof(self))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(20, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);

	if (MEMCPY_S(&ms->ms_waiter, sizeof(ms->ms_waiter), &waiter, sizeof(waiter))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(21, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);

	if (MEMCPY_S(&ms->ms_waiter, sizeof(ms->ms_waiter), &waiter, sizeof(waiter))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (MEMCPY_S(&ms->ms_self, sizeof(ms->ms_self), &self, sizeof(self))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(22, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(void*);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(waiters, _len_waiters);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (waiters != NULL) ? _len_waiters : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);

	if (waiters != NULL) {
		if (MEMCPY_S(&ms->ms_waiters, sizeof(const void**), &__tmp, sizeof(const void**))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_waiters % sizeof(*waiters) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (MEMCPY_S(__tmp, ocalloc_size, waiters, _len_waiters)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_waiters);
		ocalloc_size -= _len_waiters;
	} else {
		ms->ms_waiters = NULL;
	}

	if (MEMCPY_S(&ms->ms_total, sizeof(ms->ms_total), &total, sizeof(total))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(23, ms);

	if (status == SGX_SUCCESS) {
		if (retval) *retval = ms->ms_retval;
	}
	sgx_ocfree();
	return status;
}

