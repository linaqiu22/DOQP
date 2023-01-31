#include "storage.hpp"
#include "Enclave-utility.hpp"
#pragma once

// namespace BPlusTree
// {
    // using namespace std;

    /**
     * @brief Ids of different block types
     *
     */
    enum BlockType
    {
        /**
         * @brief holds raw data (ID + bytes)
         *
         */
        DataBlock,
        /**
         * @brief holds pointers to other blocks
         *
         */
        NodeBlock
    };

    /**
     * @brief The wrapper around the algorithms that traverse the tree
     *
     */
    class BPlusTreeIndex
    {
        public:
        /**
         * @brief returns the data for the given key
         *
         * Will return the vector of data blobs corresponding to the given key.
         * May be empty if key not found.
         * May be multi-element if this key has duplicates.
         *
         * @param key a key to look for
         * @param response the data corresponding to the key
         */
        void search(number key, vector<pair<number,bytes>> &response);

        /**
         * @brief same as search except it returns data corresponding to all key between given (inclusive)
         *
         * @param start the inclusive lower range endpoint
         * @param end the inclusive upper range endpoint
         * @param response the data corresponding to the range
         */
        void search(number start, number end, vector<pair<number,bytes>> &response, bool binSearch=false);

        /**
         * @brief Construct a new Tree object
         *
         * This constructor is used when the storage already has the data.
         *
         * @param storage the storage provider to use in the tree
         */
        BPlusTreeIndex(shared_ptr<AbsStorage> storage);

        /**
         * @brief Construct a new Tree object
         *
         * This constructor is used to create the tree data.
         *
         * \note
         * This tree implementation does not allow tree modififactions.
         * All data must be supplied in advance.
         *
         * @param storage the storage provider to use in the tree
         * @param data the data points to create tree from
         */
        BPlusTreeIndex(shared_ptr<AbsStorage> storage, vector<pair<number, bytes>> &data);

        // BPlusTreeIndex(number blockSize, vector<pair<number, bytes>> &data);

        private:
        shared_ptr<AbsStorage> storage;
        number root;
        number b;

        number leftmostDataBlock; // for testing

        /**
         * @brief Create a Data Block and store it in the storage
         *
         * The block byte structure is the following:
         * 	FIRST block:
         * 		4 bytes type (equals DataBlock)
         * 		4 bytes size of this storage block in bytes (unsigned int)
         * 		8 bytes next block address (storage block for this Data Block)
         * 		8 bytes next bucket address (address of the next Data Block in the linked list)
         * 		8 bytes key of the Data Block
         * 	SECOND+ blocks:
         * 		4 bytes type (equals DataBlock)
         * 		4 bytes size of this storage block in bytes (unsigned int)
         *		8 bytes next block address (storage block for this Data Block)
         * One should keep reading block and jumping to "next block" until it is EMPTY.
         * In every block one should read "size" bytes
         *
         * @param data the data to be stored in the block
         * @param key the key corresponding to the data
         * @param next the pointer to the next data block for linked list (may be EMPTY)
         * @return number the address of the newly created data block
         */
        number createDataBlock(const bytes &data, number key, number next);

        /**
         * @brief reads the data from the DataBlock
         *
         * @param block the first storage block of the Data Block (usually got with checkType)
         * @return tuple<bytes, number, number> tuple of data itself, associated key and address of the next Data Block
         */
        tuple<bytes, number, number> readDataBlock(const bytes &block);

        /**
         * @brief Create a Node Block and store it in the storage
         *
         * The block byte structure is the following:
         * 	4 bytes type (equals NodeBlock)
         * 	4 bytes size of this storage block in bytes (unsigned int)
         * One should read exactly "size" bytes of data.
         *
         * \note
         * This tree has an address per every key.
         * An optimization would be to hold one more address than the key, pointing to the keys larger than the largest.
         * This optimization is not implemented.
         *
         * @param data the indices to store in the block in a form of pairs of key to address.
         * It must be maintained that the address points to the subtree which contains all keys less than or equal to the given key.
         *
         * @return number the address of the newly creatred node block
         */
        number createNodeBlock(const vector<pair<number, number>> &data);

        /**
         * @brief reads the data from the node block in a form of pair keys to addresses
         *
         * @param block block the first storage block of the Node Block (usually got with checkType)
         * @return vector<pair<number, number>> the pairs (in-order) of keys to addresses
         */
        vector<pair<number, number>> readNodeBlock(const bytes &block);

        /**
         * @brief returns the type and the content of the block by the address
         *
         * @param address the address from which to read a block
         * @return pair<BlockType, bytes> the type and the bytes of the block itself (to avoid double reading)
         */
        pair<BlockType, bytes> checkType(number address);

        /**
         * @brief creates a layer of node blocks (od a single level) and returns the indices of the next layer
         *
         * This procedure is designed to create the tree level-by-level from the bottom to the top.
         * At some point, the result will contain just on address - the root address.
         *
         * @param input the in-order pairs of key and addresses
         * @return vector<pair<number, number>> the pairs of the next, higher level
         */
        vector<pair<number, number>> pushLayer(const vector<pair<number, number>> &input);

        /**
         * @brief traverses the tree looking for some of invariants to hold
         *
         * It looks for:
         *	block underflow (excluding root and rightmost nodes)
         *	wrong (non-ascending) order of keys
         *	children keys being greater than the parent keys
         *	empty pointers (except where they should be EMPTY)
         *	wrong data block keys
         *	wrong block types
         *
         * @param address the root of the tree (or the node of the subtree)
         * @param largestKey the largest possible key
         * @param rightmost if this node (by address) is the rightmost (true for the root)
         */
        void checkConsistency(number address, number largestKey, bool rightmost);

        /**
         * @brief runs checkConsistency with proper parameters (from root)
         *
         */
        void checkConsistency();

    };
// }
