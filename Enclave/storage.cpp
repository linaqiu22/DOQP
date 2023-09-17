#include "storage.hpp"
#include "Enclave-utility.hpp"


InMemoryStorage::InMemoryStorage(number blockSize) :
    AbsStorage(blockSize)
{
    auto emptyBlock = bytesFromNumber(empty());
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
    // checkLocation(location);

    response.insert(response.begin(), memory[location].begin(), memory[location].end());
}

void InMemoryStorage::set(number location, const bytes &data)
{
    if (data.size() != blockSize)
    {
        printf("data size %lu does not match block size %llu\n", data.size(), blockSize);
        throw Exception("runtime error.");
    }

    memory[location] = data;
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
    printf("locationCounter %d ", locationCounter);
    return (locationCounter - 2) * blockSize;
}

void InMemoryStorage::checkLocation(number location)
{
    if (location >= locationCounter)
    {
        printf("attempt to access memory that was not malloced %llu\n", location);
        throw Exception("runtime error.");
    }
}


