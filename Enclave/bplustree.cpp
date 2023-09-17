#include "bplustree.hpp"
// #include <algorithm>
// #include <math.h>

// namespace BPlusTree
// {
    // using namespace std;

    pair<BlockType, number> getTypeSize(number typeAndSize);
    number setTypeSize(BlockType type, number size);

    BPlusTreeIndex::BPlusTreeIndex(shared_ptr<AbsStorage> storage) :
        storage(storage)
    {
        b = (storage->getBlockSize() - sizeof(number)) / (2 * sizeof(number));
        if (b < 2)
        {
            printf("storage block size too small for the tree\n");
            throw Exception("runtime error.");
        }

        bytes rootBytes;
        storage->get(storage->meta(), rootBytes);
        rootBytes.resize(sizeof(number));
        if (numberFromBytes(rootBytes) != storage->empty())
        {
            root = numberFromBytes(rootBytes);
        }
    }

    BPlusTreeIndex::BPlusTreeIndex(shared_ptr<AbsStorage> storage, vector<pair<number, bytes>> &data) :
        BPlusTreeIndex(storage)
    {
        sort(data.begin(), data.end(), [](const pair<number, bytes> &a, const pair<number, bytes> &b) { 
                return a.first < b.first; 
            });
        // data layer
        vector<pair<number, number>> layer;
        layer.resize(data.size());
        for (int i = data.size() - 1; i >= 0; i--)
        {
            layer[i].first	= data[i].first;
            layer[i].second = createDataBlock(
                data[i].second,
                data[i].first,
                (uint)i == data.size() - 1 ? storage->empty() : layer[i + 1].second);
        }
        leftmostDataBlock = layer[0].second;

        // leaf layer
        layer = pushLayer(layer);

        // nodes layer
        while (layer.size() > 1)
        {
            layer = pushLayer(layer);
        }
        root = layer[0].second;

        auto rootBytes = bytesFromNumber(root);
        rootBytes.resize(storage->getBlockSize());
        storage->set(storage->meta(), rootBytes);
    }

    /* BPlusTreeIndex::BPlusTreeIndex(number blockSize, vector<pair<number, bytes>> &data) {
        storage = make_shared<InMemoryStorage>(blockSize);

        b = (storage->getBlockSize() - sizeof(number)) / (2 * sizeof(number));
        if (b < 2)
        {
            printf("storage block size too small for the tree\n");
            throw Exception("runtime error.");
        }

        bytes rootBytes;
        storage->get(storage->meta(), rootBytes);
        rootBytes.resize(sizeof(number));
        if (numberFromBytes(rootBytes) != storage->empty())
        {
            root = numberFromBytes(rootBytes);
        }

        sort(data.begin(), data.end(), [](const pair<number, bytes> &a, const pair<number, bytes> &b) { 
                return a.first < b.first; 
            });
        // data layer
        vector<pair<number, number>> layer;
        layer.resize(data.size());
        for (int i = data.size() - 1; i >= 0; i--)
        {
            layer[i].first	= data[i].first;
            layer[i].second = createDataBlock(
                data[i].second,
                data[i].first,
                (uint)i == data.size() - 1 ? storage->empty() : layer[i + 1].second);
        }
        leftmostDataBlock = layer[0].second;

        // leaf layer
        layer = pushLayer(layer);

        // nodes layer
        while (layer.size() > 1)
        {
            layer = pushLayer(layer);
        }
        root = layer[0].second;

        rootBytes = bytesFromNumber(root);
        rootBytes.resize(storage->getBlockSize());
        storage->set(storage->meta(), rootBytes);
    } */

    void BPlusTreeIndex::search(number key, vector<pair<number,bytes>> &response)
    {
        return search(key, key, response);
    }

    void BPlusTreeIndex::search(number start, number end, vector<pair<number,bytes>> &response, bool binSearch)
    {
        auto address = root;
        while (true)
        {
            auto pr = checkType(address);
            auto type = pr.first;
            auto read = pr.second;
            
            switch (type)
            {
                case NodeBlock:
                {
                    auto block = readNodeBlock(read);
                    address	   = storage->empty();
                    for (uint i = 0; i < block.size(); i++)
                    {
                        if (start <= block[i].first)
                        {
                            address = block[i].second;
                            break;
                        }
                    }
                    if (address == storage->empty())
                    {
                        // key is larger than the largest
                        return;
                    }
                    break;
                }
                case DataBlock:
                {
                    tuple<bytes, number, number> block;
                    while (true)
                    {
                        block = readDataBlock(read);
                        if (binSearch) {
                            vector<number> value = deconstructNumbers(get<0>(block));
                            assert(value.size() == 2);
                            number bucketStart = value[0];
                            if (get<1>(block) < start || bucketStart > end)
                            {
                                // if we have read the block outside of the range, we are done
                                return;
                            }
                        } else {
                            if (get<1>(block) < start || get<1>(block) > end)
                            {
                                // if we have read the block outside of the range, we are done
                                return;
                            }
                        }
                        response.push_back({get<1>(block), get<0>(block)});
                        if (get<2>(block) == storage->empty())
                        {
                            // if it is the last block in the linked list, we are done
                            return;
                        }
                        read = checkType(get<2>(block)).second;
                    }
                }
            }
        }
    }

    vector<pair<number, number>> BPlusTreeIndex::pushLayer(const vector<pair<number, number>> &input)
    {
        vector<pair<number, number>> layer;
        // go in a B-increments
        for (uint i = 0; i < input.size(); i += b)
        {
            vector<pair<number, number>> block;
            block.resize(b);
            number max = 0uLL;
            // pack in b-sets
            for (uint j = 0; j < b; j++)
            {
                if (i + j < input.size())
                {
                    block[j] = input[i + j];
                    max		 = block[j].first > max ? block[j].first : max;
                }
                else
                {
                    // if less than b items exist, resize
                    block.resize(j);
                    break;
                }
            }
            auto address = createNodeBlock(block);
            // keep creating the next (top) layer
            layer.push_back({max, address});
        }

        return layer;
    }

    number BPlusTreeIndex::createNodeBlock(const vector<pair<number, number>> &data)
    {
        if (storage->getBlockSize() - sizeof(number) < data.size() * 2 * sizeof(number))
        {
            printf("data size (%lu pairs) is too big for the block size %llu\n", data.size(), (storage->getBlockSize() - (uint)sizeof(number)));
            throw Exception("runtime error.");
        }

        // pairs and size of the block itself (4 bytes size, 4 bytes type)
        number numbers[data.size() * 2 + 1];

        numbers[0] = setTypeSize(NodeBlock, data.size() * 2 * sizeof(number));
        for (uint i = 0; i < data.size(); i++)
        {
            numbers[1 + 2 * i]	   = data[i].first;
            numbers[1 + 2 * i + 1] = data[i].second;
        }
        bytes block((uchar *)numbers, (uchar *)numbers + (data.size() * 2 + 1) * sizeof(number));
        block.resize(storage->getBlockSize());

        auto address = storage->malloc();
        storage->set(address, block);

        return address;
    }

    vector<pair<number, number>> BPlusTreeIndex::readNodeBlock(const bytes &block)
    {
        auto deconstructed = deconstruct(block, {sizeof(number)});
        auto pr  = getTypeSize(numberFromBytes(deconstructed[0]));
        auto type = pr.first;
        auto size = pr.second;
        auto blockData	   = deconstructed[1];

        if (type != NodeBlock)
        {
            throw Exception("attempt to read a non-node block as node block");
        }

        blockData.resize(size);

        auto count = blockData.size() / sizeof(number);

        uchar buffer[count * sizeof(number)];
        copy(blockData.begin(), blockData.end(), buffer);

        vector<pair<number, number>> result;
        result.resize(count / 2);
        for (uint i = 0; i < count / 2; i++)
        {
            result[i].first	 = ((number *)buffer)[2 * i];
            result[i].second = ((number *)buffer)[2 * i + 1];
        }

        return result;
    }

    number BPlusTreeIndex::createDataBlock(const bytes &data, number key, number next)
    {
        // different if all fits in a single storage block, or not
        number firstBlockSize = storage->getBlockSize() - 4 * sizeof(number);
        number otherBlockSize = storage->getBlockSize() - 2 * sizeof(number);

        auto blocks =
            data.size() <= firstBlockSize ?
                1 :
                1 + (data.size() - firstBlockSize + otherBlockSize - 1) / otherBlockSize;

        // request necessary addresses in advance
        vector<number> addresses;
        addresses.resize(blocks);
        for (uint i = 0; i < blocks; i++)
        {
            addresses[i] = storage->malloc();
        }

        // scan data block by block
        auto readSoFar = 0;
        for (number i = 0; i < blocks; i++)
        {
            bytes buffer;
            auto end = min(readSoFar + (i == 0 ? firstBlockSize : otherBlockSize), (number)data.size());

            copy(data.begin() + readSoFar, data.begin() + end, back_inserter(buffer));
            buffer.resize(i == 0 ? firstBlockSize : otherBlockSize);

            auto thisTypeAndSize = setTypeSize(DataBlock, end - readSoFar);
            auto nextBlock		 = i < blocks - 1 ? addresses[i + 1] : storage->empty();
            bytes numbers;
            // different for the fist and subsequent blocks
            if (i == 0)
            {
                numbers = concatNumbers(4, thisTypeAndSize, nextBlock, next, key);
            }
            else
            {
                numbers = concatNumbers(2, thisTypeAndSize, nextBlock);
            }
            storage->set(
                addresses[i],
                concat(2, &numbers, &buffer));

            readSoFar = end;
        }

        return addresses[0];
    }

    tuple<bytes, number, number> BPlusTreeIndex::readDataBlock(const bytes &block)
    {
        bytes data;
        auto address = storage->empty();
        number nextBucket;
        number key;

        while (true)
        {
            auto first = address == storage->empty();

            bytes read;
            if (first)
            {
                read.insert(read.begin(), block.begin(), block.end());
            }
            else
            {
                storage->get(address, read);
            }

            auto deconstructed = deconstruct(read, {(first ? 4 : 2) * (int)sizeof(number)});
            auto numbers	   = deconstructNumbers(deconstructed[0]);
            auto blockData	   = deconstructed[1];

            auto pr = getTypeSize(numbers[0]);
            auto type = pr.first;
            auto thisSize = pr.second;

            if (type != DataBlock)
            {
                throw Exception("attempt to read a non-data block as data block");
            }
            auto nextBlock = numbers[1];

            if (first)
            {
                nextBucket = numbers[2];
                key		   = numbers[3];
            }
            blockData.resize(thisSize);
            data = concat(2, &data, &blockData);

            if (nextBlock != storage->empty())
            {
                address = nextBlock;
                continue;
            }
            else
            {
                return {data, key, nextBucket};
            }
        }
    }

    pair<BlockType, bytes> BPlusTreeIndex::checkType(number address)
    {
        bytes block;
        storage->get(address, block);
        auto deconstructed = deconstruct(block, {sizeof(number)});
        auto typeAndSize   = numberFromBytes(deconstructed[0]);
        return {getTypeSize(typeAndSize).first, block};
    }

    void BPlusTreeIndex::checkConsistency()
    {
        checkConsistency(root, UINT_MAX, true);
    }

    void BPlusTreeIndex::checkConsistency(number address, number largestKey, bool rightmost)
    {
        // helper to throw exception if condition fails
        auto throwIf = [](bool expression, Exception exception) {
            if (expression)
            {
                throw exception;
            }
        };

        auto pr = checkType(address);
        auto type = pr.first;
        auto read = pr.second;

        switch (type)
        {
            case NodeBlock:
            {
                auto block = readNodeBlock(read);
                if ((block.size() < b / 2 && !rightmost) || block.size() == 0) {
                    printf("block undeflow (%lu) for b = %llu and block is not the rightmost\n", block.size(), b);
                    throw Exception("runtime error.");
                }

                for (uint i = 0; i < block.size(); i++)
                {
                    if (i != 0)
                    {
                        throwIf(
                            block[i].first < block[i - 1].first,
                            Exception("wrong order of keys in a block"));
                    }
                    throwIf(
                        block[i].first > largestKey,
                        Exception("keys larger than the parent key is found in a block"));
                    throwIf(
                        block[i].second == storage->empty(),
                        Exception("empty pointer found in a block"));

                    checkConsistency(block[0].second, block[0].first, rightmost && i == block.size() - 1);
                }
                break;
            }
            case DataBlock:
            {
                auto block = readDataBlock(read);
                throwIf(
                    get<2>(block) == storage->empty() && !rightmost,
                    Exception("empty pointer to the next data block, not the rightmost"));
                throwIf(
                    get<1>(block) != largestKey,
                    Exception("data block has the key different from the parent's"));
                break;
            }
            default:
                throw Exception("invalid block type");
        }
    }

    /**
     * @brief deconstruct the number into type and size
     *
     * @param typeAndSize the number to break up
     * @return pair<BlockType, number> the type and size of the block
     */
    pair<BlockType, number> getTypeSize(number typeAndSize)
    {
        number buffer[1]{typeAndSize};
        auto type = ((uint *)buffer)[0];
        auto size = ((uint *)buffer)[1];

        return {(BlockType)type, (number)size};
    }

    /**
     * @brief compose type and size into number
     *
     * @param type type of the storage block
     * @param size size of the storage block in bytes
     * @return number the composition of the arguments as number
     */
    number setTypeSize(BlockType type, number size)
    {
        uint buffer[2]{type, (uint)size};
        return ((number *)buffer)[0];
    }
// }
