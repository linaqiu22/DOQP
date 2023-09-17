#include "app-storage.hpp"

InMemoryStorage::InMemoryStorage(number blockSize) :
    AbsStorage(blockSize)
{
    number material[1] = {empty()};
    auto emptyBlock = bytes((uchar *)material, (uchar *)material + sizeof(number));
    emptyBlock.resize(blockSize);
    set(meta(), emptyBlock);
}

InMemoryStorage::~InMemoryStorage()
{
}

void InMemoryStorage::serializeData(char* sdata, bool exceptMeta) {
    auto iter = sdata;
    for (auto &&pr : memory) {
        if (exceptMeta && pr.first == meta()) {
            continue;
        }
        copy(pr.second.begin(), pr.second.end(), iter);
        iter += blockSize;
    }
}

void InMemoryStorage::get(number location, bytes &response)
{
    checkLocation(location);

    response.insert(response.begin(), memory[location].begin(), memory[location].end());
}

void InMemoryStorage::set(number location, const bytes &data)
{
    if (data.size() != blockSize)
    {
        printf("data size %lu does not match block size %llu\n", data.size(), blockSize);
        // printf("data size does not match block size\n");
        throw Exception("runtime error.");
    }

    checkLocation(location);

    memory[location] = data;
}

void InMemoryStorage::resize(uint size) {
    number location = META + 1 + size;
    auto iter = memory.find(location);
    if (iter != memory.end()) {
        memory.erase(iter, memory.end());
        locationCounter = location;
    }
}

number InMemoryStorage::malloc()
{
    return locationCounter++;
}

number InMemoryStorage::empty()
{
    return EMPTY;
}

number InMemoryStorage::meta()
{
    return META;
}

number InMemoryStorage::size()
{
    return (locationCounter - 1) * blockSize;
}

number InMemoryStorage::nometaSize() {
    return (locationCounter - 2) * blockSize;
}

void InMemoryStorage::checkLocation(number location)
{
    if (location >= locationCounter)
    {
        printf("attempt to access memory that was not malloced %llu\n", location);
        // printf("attempt to access memory that was not malloced\n");
        throw Exception("runtime error.");
    }
}

FileSystemStorage::FileSystemStorage(number blockSize, string filename, bool override) :
    AbsStorage(blockSize)
{
    auto flags = fstream::in | fstream::out | fstream::binary | fstream::ate;
    if (override)
    {
        flags |= fstream::trunc;
    }

    file.open(filename, flags);
    if (!file)
    {
        printf("cannot open %s\n", filename.c_str());
        throw Exception("runtime error.");
    }

    locationCounter = override ? (2 * blockSize) : (number)file.tellg();

    if (override)
    {
        // auto emptyBlock = bytesFromNumber(empty());
        number material[1] = {empty()};
        auto emptyBlock = bytes((uchar *)material, (uchar *)material + sizeof(number));
        emptyBlock.resize(blockSize);
        set(meta(), emptyBlock);
    }
}

FileSystemStorage::~FileSystemStorage()
{
    file.close();
}

void FileSystemStorage::get(number location, bytes &response)
{
    checkLocation(location);

    uchar placeholder[blockSize];
    file.seekg(location, file.beg);
    file.read((char *)placeholder, blockSize);

    response.insert(response.begin(), placeholder, placeholder + blockSize);
}

void FileSystemStorage::set(number location, const bytes &data)
{
    if (data.size() != blockSize)
    {
        printf("data size %lu does not match block size %llu\n", data.size(), blockSize);
        throw Exception("runtime error.");
    }

    checkLocation(location);

    uchar placeholder[blockSize];
    copy(data.begin(), data.end(), placeholder);

    file.seekp(location, file.beg);
    file.write((const char *)placeholder, blockSize);
}

number FileSystemStorage::malloc()
{
    return locationCounter += blockSize;
}

number FileSystemStorage::empty()
{
    return EMPTY;
}

number FileSystemStorage::meta()
{
    return blockSize;
}

number FileSystemStorage::size()
{
    return locationCounter - blockSize;
}

number FileSystemStorage::nometaSize() {
    return locationCounter - 2 * blockSize;
}

void FileSystemStorage::checkLocation(number location)
{
    if (location > locationCounter || location % blockSize != 0)
    {
        printf("attempt to access memory that was not malloced %llu\n", location);
        throw Exception("runtime error.");
    }
}



