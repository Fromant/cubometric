#ifndef CHUNK_H
#define CHUNK_H

#include <array>

#include "BlockType.h"
#include "EFacing.h"

class Chunk {
public:
    static constexpr int WIDTH = 32;
    static constexpr int HEIGHT = 32;
    static constexpr int DEPTH = 32;

private:
    std::array<BlockType, DEPTH * WIDTH * HEIGHT> blocks{}; //bare 16kb chunk data.

public:
    int xCoord;
    int yCoord;
    int zCoord;
    std::array<size_t, 6> faceCounts{};
    std::array<size_t, 6> faceOffsets{};

    Chunk(int x, int y, int z) : xCoord(x), yCoord(y), zCoord(z) {
        // for (int i = 0; i < WIDTH * DEPTH; i++) {
        //     blocks[i] = BlockType::DIRT;
        // }
    };

    Chunk() = delete;

    size_t getId() const {
        return Chunk::getId(xCoord, yCoord, zCoord);
    }

    static size_t getId(int xCoord, int yCoord, int zCoord) {
        return (size_t(xCoord & 0xFFFFFF) << 24) | (size_t(zCoord & 0xFFFFFF)) | (size_t(yCoord & 0x3F) << 48);
    }

    const std::array<BlockType, DEPTH * WIDTH * HEIGHT>& getBlocks() const { return blocks; }
    std::array<BlockType, DEPTH * WIDTH * HEIGHT>& getBlocks() { return blocks; }
};

#endif //CHUNK_H
