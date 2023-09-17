#include <assert.h>
#include <string.h>
#include <vector>
#pragma once


// #define OP_READ 0
// #define OP_WRITE 1


template <typename T>
class TraceMem {
    public:
        int count;
        T *mem;
        int size;

        TraceMem(int size) : size(size) {
            mem = (T *)calloc(size, sizeof(T));
            count = 0;
        }

        void resize(int new_size) {
            mem = (T *)realloc(mem, sizeof(T) * new_size);
            if (new_size > size) {
                memset((T *)mem + size, 0, sizeof(T) * (new_size - size));
            } else {
                count = new_size;
            }
            size = new_size;
        }

        void insert(int add_size, vector<T> &new_data) {
            int org_size = size;
            resize(size + add_size);
            for (int i = org_size; i < size; i++) {
                write(i, new_data[i - org_size]);
            }
        }

        void insert(int add_size, TraceMem<T> &new_data, int start) {
            int org_size = size;
            resize(size + add_size);
            memcpy((T *)mem + org_size, (T *)new_data.mem + start, add_size*sizeof(T));
        }

        void erase(int idx) {
            memmove(mem, mem+idx, (size-idx)*sizeof(T));
            size = size - idx;
            count = size;
        }

        T read(int i) {
            assert(i >= 0 && i < size);
            return mem[i];
        }

        T* readPtr(int i) {
            return &mem[i];
        }

        void write(int i, T elt) {
            assert(i >= 0 && i < size);
            mem[i] = elt;
        }
        
        void write(T elt) {
            assert(count >= 0 && count < size);
            mem[count] = elt;
            count++;
        }

        void freeSpace() {
            free(mem);
        }
};

