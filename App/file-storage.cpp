// #include "file-storage.hpp"


// FileSystemStorage::FileSystemStorage(number blockSize, string filename, bool override) :
//     AbsStorage(blockSize)
// {
//     auto flags = fstream::in | fstream::out | fstream::binary | fstream::ate;
//     if (override)
//     {
//         flags |= fstream::trunc;
//     }

//     file.open(filename, flags);
//     if (!file)
//     {
//         printf("cannot open %s\n", filename.c_str());
//         throw Exception("runtime error.");
//     }

//     locationCounter = override ? (2 * blockSize) : (number)file.tellg();

//     if (override)
//     {
//         // auto emptyBlock = bytesFromNumber(empty());
//         number material[1] = {empty()};
//         auto emptyBlock = bytes((uchar *)material, (uchar *)material + sizeof(number));
//         emptyBlock.resize(blockSize);
//         set(meta(), emptyBlock);
//     }
// }

// FileSystemStorage::~FileSystemStorage()
// {
//     file.close();
// }

// void FileSystemStorage::serializeData(char* sdata, bool exceptMeta) {

// }

// void FileSystemStorage::get(number location, bytes &response)
// {
//     checkLocation(location);

//     uchar placeholder[blockSize];
//     file.seekg(location, file.beg);
//     file.read((char *)placeholder, blockSize);

//     response.insert(response.begin(), placeholder, placeholder + blockSize);
// }

// void FileSystemStorage::set(number location, const bytes &data)
// {
//     if (data.size() != blockSize)
//     {
//         printf("data size %lu does not match block size %llu\n", data.size(), blockSize);
//         throw Exception("runtime error.");
//     }

//     checkLocation(location);

//     uchar placeholder[blockSize];
//     copy(data.begin(), data.end(), placeholder);

//     file.seekp(location, file.beg);
//     file.write((const char *)placeholder, blockSize);
// }

// number FileSystemStorage::malloc()
// {
//     return locationCounter += blockSize;
// }

// number FileSystemStorage::empty()
// {
//     return EMPTY;
// }

// number FileSystemStorage::meta()
// {
//     return blockSize;
// }

// number FileSystemStorage::size()
// {
//     return locationCounter - blockSize;
// }

// number FileSystemStorage::nometaSize() {
//     return locationCounter - 2 * blockSize;
// }

// void FileSystemStorage::checkLocation(number location)
// {
//     if (location > locationCounter || location % blockSize != 0)
//     {
//         printf("attempt to access memory that was not malloced %llu\n", location);
//         throw Exception("runtime error.");
//     }
// }
