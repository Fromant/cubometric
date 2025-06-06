#pragma once

#include <array>

#include "BlockType.h"
#include "ChunkData.hpp"
#include "render/Frustum.h"

class Chunk {
public:
    static constexpr int WIDTH = ChunkData::SIZE;
    static constexpr int HEIGHT = ChunkData::SIZE;
    static constexpr int DEPTH = ChunkData::SIZE;

private:
    AABB box;

    static uint64_t mortonEncode(uint32_t x, uint32_t y, uint32_t z) {
        uint64_t morton = 0;
        for (int i = 0; i < 21; ++i) {
            morton |= ((uint64_t)(x & 1) << (3 * i));
            morton |= ((uint64_t)(y & 1) << (3 * i + 1));
            morton |= ((uint64_t)(z & 1) << (3 * i + 2));
            x >>= 1;
            y >>= 1;
            z >>= 1;
        }
        return morton;
    }

    static void mortonDecode(uint64_t morton, uint32_t& x, uint32_t& y, uint32_t& z) {
        x = 0;
        y = 0;
        z = 0;
        for (int i = 0; i < 21; ++i) {
            x |= ((morton >> (3 * i)) & 1) << i;
            y |= ((morton >> (3 * i + 1)) & 1) << i;
            z |= ((morton >> (3 * i + 2)) & 1) << i;
        }
    }

public:
    int xCoord; //chunk coordinate
    int yCoord; //chunk coordinate
    int zCoord; //chunk coordinate

    ChunkData blocks{};

    std::array<size_t, 6> faceCounts{};
    std::array<size_t, 6> faceOffsets{};

    explicit Chunk(glm::ivec3 coords) : xCoord(coords.x), yCoord(coords.y), zCoord(coords.z),
                               box{
                                   glm::vec3(coords.x, coords.y, coords.z),
                                   glm::vec3(coords.x, coords.y, coords.z) + glm::vec3(
                                       Chunk::WIDTH, Chunk::HEIGHT, Chunk::DEPTH)
                               } {};

    Chunk() = delete;

    Chunk(Chunk&& other) noexcept {
        blocks = std::move(other.blocks);
        xCoord = other.xCoord;
        yCoord = other.yCoord;
        zCoord = other.zCoord;
        box = other.box;
    }

    size_t getId() const {
        return Chunk::getId(xCoord, yCoord, zCoord);
    }

    static size_t getId(int xCoord, int yCoord, int zCoord) {
        constexpr uint64_t OFFSET = 1 << 20; // 2^20 (handles ±1M chunks)
        return mortonEncode(xCoord + OFFSET, yCoord + OFFSET, zCoord + OFFSET);
    }

    const ChunkData& getBlocks() const { return blocks; }
    ChunkData& getBlocks() { return blocks; }

    const AABB& getChunkBoundingBox() const {
        return box;
    }
};
