#include "CONFIG.hpp"
#include "type_def.hpp"
#include <unordered_map>
#pragma once


/**
 * @brief Abstraction over secondary storage (modeled as RAM)
 *
 */
class AbsStorage
{
    public:
    virtual void serializeData(char* sdata, bool exceptMeta=true) = 0;
    /**
     * @brief reads one block of bytes from the address
     *
     * @param location the address from which to read
     * @param response the bytes read, one block
     */
    virtual void get(number location, bytes &response) = 0;

    /**
     * @brief write one block of bytes to the address
     *
     * @param location the address to which to write
     * @param data the block of bytes to write (must be of block size)
     */
    virtual void set(number location, const bytes &data) = 0;

    /**
     * @brief request an address to which it isi possible to write a block
     *
     * @return number the allocated address.
     * Guaranteed to be writable and not to overalp with previously allocated
     */
    virtual number malloc() = 0;

    /**
     * @brief gives the "empty" address, so that the logic can check for "null pointers"
     *
     * @return number the "null" address, guaranteed to never be allocated
     */
    virtual number empty() = 0;

    /**
     * @brief gives the address of the special (reserved) meta block
     *
     * @return number the address of the meta block
     */
    virtual number meta() = 0;

    /**
     * @brief Returns the amount of malloc'ed space in bytes.
     *
     * @return number number of malloc'ed bytes.
     */
    virtual number size() = 0;
    virtual number nometaSize() = 0;

    /**
     * @brief Construct a new Abs Storage Adapter object
     *
     * @param blockSize the size of block in bytes (better be enough sized for the B+ tree)
     */
    AbsStorage(number blockSize) : blockSize(blockSize){};
    ~AbsStorage(){};

    /**
     * @brief a getter for block size
     *
     * @return number the size of block in bytes
     */
    number getBlockSize() {
        return blockSize;
    };

    protected:
    number blockSize;
};

/**
 * @brief In-memory implementation of the storage adapter.
 *
 * Uses a RAM array as the underlying storage.
 */
class InMemoryStorage : public AbsStorage
{
    private:
    unordered_map<number, bytes> memory;
    number locationCounter = META + 1;

    static const number EMPTY = 0;
    static const number META  = 1;

    void checkLocation(number location);

    public:
    InMemoryStorage(number blockSize);
    ~InMemoryStorage();

    void serializeData(char* sdata, bool exceptMeta=true) final;
    void get(number location, bytes &response) final;
    void set(number location, const bytes &data) final;
    number malloc() final;

    number empty() final;
    number meta() final;

    number size() final;
    number nometaSize() final; 
    // number getBlockSize();
};
