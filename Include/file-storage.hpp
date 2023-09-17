// #include "storage.hpp"
// #include <fstream>

// #pragma once
// /**
//  * @brief File system implementation of the storage adapter.
//  *
//  * Uses a binary file as the underlying storage.
//  */
// class FileSystemStorage : public AbsStorage
// {
//     private:
//     fstream file;
//     number locationCounter;

//     static const number EMPTY = 0;

//     void checkLocation(number location);

//     public:
//     FileSystemStorage(number blockSize, string filename, bool override);
//     ~FileSystemStorage();
    
//     void serializeData(char* sdata, bool exceptMeta=true) final;
//     void get(number location, bytes &response) final;
//     void set(number location, const bytes &data) final;
//     number malloc() final;

//     number empty() final;
//     number meta() final;

//     number size() final;
//     number nometaSize() final;
//     // number getBlockSize();
// };
