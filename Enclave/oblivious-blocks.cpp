#include "oblivious-blocks.hpp"
// #include "do-algorithms.hpp"

int prev_pow_two(int x) {
    int y = 1;
    while (y < x) y <<= 1;
    return y >>= 1;
}

void enclaveCompact(int noisyOutputSize) {
    int inSize = joinOutput->size;
    for (int i = 0; i < log2(inSize); i++) {
        for (int j = pow(2, i); j < inSize; j++) {
            auto joinRow1 = joinOutput->read(j);
            int distance = jDistArray[j];
            int offset = (distance % (int)pow(2, i+1)); // start with 0 or 1 (and then 3 5 6 7), binning once for all, timing not much of an issue
            int dest = j - pow(2, i) < 0 ? 0 : j - pow(2, i);
            //* swap
            auto joinRow2 = joinOutput->read(dest);
            if (offset > 0) {
                joinOutput->write(j, joinRow2);
                joinOutput->write(dest, joinRow1);
                distance -= offset;
                jDistArray[j] = 0;
                jDistArray[dest] = distance;
            } else {
                joinOutput->write(j, joinRow1);
                joinOutput->write(dest, joinRow2);
            }
        }
    }
    // implicily assuming that IO_BLOCKSIZE > noisyOutputSize
    if (noisyOutputSize >= 0) {
        joinOutput->resize(noisyOutputSize);
    }
    jDistArray.clear();
}

void ORCompact(int noisyOutputSize, TraceMem<tbytes> *data) {
    if (data == nullptr) {
        TightCompact_inner(0, joinOutput->size, selected_count);
        if (noisyOutputSize >= 0 && noisyOutputSize < joinOutput->size) {
            joinOutput->resize(noisyOutputSize);
        }
        jDistArray.clear();
    } else {
        TightCompact_inner(0, data->size, selected_count, data);
        if (noisyOutputSize >= 0 && noisyOutputSize < data->size) {
            data->resize(noisyOutputSize);
        }
    }
}

void swap(int i, int j, bool swap) {
    auto joinRow1 = joinOutput->read(i);
    auto joinRow2 = joinOutput->read(j);
    if (swap) {
        joinOutput->write(i, joinRow2);
        joinOutput->write(j, joinRow1);
    } else {
        joinOutput->write(i, joinRow1);
        joinOutput->write(j, joinRow2);
    }
}

void swap(int i, int j, bool swap, TraceMem<tbytes> *data) {
    auto r1 = data->read(i);
    auto r2 = data->read(j);
    if (swap) {
        data->write(i, r2);
        data->write(j, r1);
    } else {
        data->write(i, r1);
        data->write(j, r2);
    }
}

void TightCompact_inner(int idx, size_t N, uint32_t *selected_count_local, TraceMem<tbytes> *data) {
    if(N==0){
        return;
    }
    else if(N==1){
        return;
    }
    else if(N==2){
        bool selected0 = selected_count_local[1] - selected_count_local[0] == 1 ? true : false;
        bool selected1 = selected_count_local[2] - selected_count_local[1] == 1 ? true : false;
        bool swap_flag = (!selected0 & selected1);
        if (data == nullptr) {
            swap(idx, idx+1, swap_flag);
        } else {
            swap(idx, idx+1, swap_flag, data);
        }
        return;
    }

    size_t gt_pow2;
    size_t split_index;

    // Find largest power of 2 < N 
    gt_pow2 = pow2_lt(N);

    // For Order-preserving ORCompact
    // This will be right (R) of the recursion, and the leftover non-power of 2 left (L)
    split_index = N - gt_pow2;

    // Number of selected items in the non-power of 2 side (left)
    size_t mL;
    if (TC_PRECOMPUTE_COUNTS) {
        mL = selected_count_local[split_index] - selected_count_local[0];
    } 

    // unsigned char *L_ptr = buf;
    // unsigned char *R_ptr = buf + (split_index * block_size);

    int L_idx = idx;
    int R_idx = idx + split_index;

    TightCompact_inner(L_idx, split_index, selected_count_local, data);
    TightCompact_2power_inner(R_idx, gt_pow2, (gt_pow2 - split_index + mL) % gt_pow2, selected_count_local+split_index, data);

    // For OP we CnS the first n_2 elements (split_size) against the suffix n_2 elements of the n_1 (2 power elements)
    // R_ptr = buf + (gt_pow2 * block_size); 
    R_idx = idx + gt_pow2;
    // Perform N-split_index oblivious swaps for this level
    for (size_t i=0; i<split_index; i++){
        // Oswap blocks at L_start, R_start conditional on marked_items
        bool swap_flag = i>=mL;
        if (data == nullptr) {
            swap(L_idx, R_idx, swap_flag);
        } else {
            swap(idx, idx+1, swap_flag, data);
        }
        L_idx+=1;
        R_idx+=1;
    }
}

void TightCompact_2power_inner(int idx, size_t N, size_t offset, uint32_t *selected_count_local, TraceMem<tbytes> *data) {
    if (N==1) {
        return;
    }
    if (N==2) {
        bool selected0 = selected_count_local[1] - selected_count_local[0] == 1 ? true : false;
        bool selected1 = selected_count_local[2] - selected_count_local[1] == 1 ? true : false;
        bool swap_flag = (!selected0 & selected1) ^ offset;
        // oswap_buffer<oswap_style>(buf, buf+block_size, block_size, swap);
        if (data == nullptr) {
            swap(idx, idx+1, swap_flag);
        } else {
            swap(idx, idx+1, swap_flag, data);
        }
        return;
    }

    // Number of selected items in left half
    size_t m1;
    if (TC_PRECOMPUTE_COUNTS) {
        m1 = selected_count_local[N/2] - selected_count_local[0];
    }

    size_t offset_mod = (offset & ((N/2)-1));
    size_t offset_m1_mod = (offset+m1) & ((N/2)-1);
    bool offset_right = (offset >= N/2);
    bool left_wrapped = ((offset_mod + m1) >= (N/2));

    TightCompact_2power_inner(idx, N/2, offset_mod, selected_count_local, data);
    TightCompact_2power_inner(idx + (N/2), N/2, offset_m1_mod, selected_count_local + N/2, data);

    // unsigned char *buf1_ptr = buf, *buf2_ptr = (buf + (N/2)*block_size);
    int L_idx = idx, R_idx = idx+(N/2);
    if (TC_OPT_SWAP_FLAG) {
        bool swap_flag = left_wrapped ^ offset_right;
        size_t num_swap = N/2;
        for(size_t i=0; i<num_swap; i++){
            swap_flag = swap_flag ^ (i == offset_m1_mod);
            // oswap_buffer<oswap_style>(buf1_ptr, buf2_ptr, block_size, swap_flag);
            if (data == nullptr) {
                swap(L_idx, R_idx, swap_flag);
            } else {
                swap(idx, idx+1, swap_flag, data);
            }
            L_idx++; R_idx++;
        }
    } else {
        for(size_t i=0; i<N/2; i++){
            bool swap_flag = (i>=offset_m1_mod) ^ left_wrapped ^ offset_right;
            if (data == nullptr) {
                swap(L_idx, R_idx, swap_flag);
            } else {
                swap(idx, idx+1, swap_flag, data);
            }
            L_idx++; R_idx++;
        }
    }
}