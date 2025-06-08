#pragma once

#include "BlockType.h"
#include "ChunkData.hpp"
#include "render/Frustum.h"

class Chunk {
public:
    static constexpr int WIDTH = ChunkData::WIDTH;
    static constexpr int HEIGHT = ChunkData::HEIGHT;
    static constexpr int DEPTH = ChunkData::DEPTH;
    static constexpr int SUB_HEIGHT = ChunkData::WIDTH;
    static constexpr int SUB_COUNT = HEIGHT / SUB_HEIGHT;

private:
    AABB box;

    static uint64_t mortonEncode(uint32_t x, uint32_t z) {
        uint64_t morton = 0;
        for (int i = 0; i < 21; ++i) {
            morton |= ((uint64_t)(x & 1) << (2 * i));
            morton |= ((uint64_t)(z & 1) << (2 * i + 1));
            x >>= 1;
            z >>= 1;
        }
        return morton;
    }

    static void mortonDecode(uint64_t morton, uint32_t& x, uint32_t& z) {
        x = 0;
        z = 0;
        for (int i = 0; i < 21; ++i) {
            x |= ((morton >> (2 * i)) & 1) << i;
            z |= ((morton >> (2 * i + 1)) & 1) << i;
        }
    }

public:
    int xCoord; //chunk coordinate
    int zCoord; //chunk coordinate

    ChunkData data;

    explicit Chunk(glm::ivec2 coords) : xCoord(coords.x), zCoord(coords.y),
                                        box{
                                            glm::vec3(coords.x * WIDTH, 0, coords.y * DEPTH),
                                            glm::vec3((coords.x + 1) * WIDTH, HEIGHT, (coords.y + 1) * DEPTH)
                                        } {};

    Chunk() = delete;

    Chunk(Chunk&& other) noexcept {
        data = std::move(other.data);
        xCoord = other.xCoord;
        zCoord = other.zCoord;
        box = other.box;
    }

    size_t getId() const {
        return Chunk::getId(xCoord, zCoord);
    }

    static size_t getId(int xCoord, int zCoord) {
        constexpr uint64_t OFFSET = 1 << 20; // 2^20 (handles ±1M chunks)
        return mortonEncode(xCoord + OFFSET, zCoord + OFFSET);
    }

    const ChunkData& getBlocks() const { return data; }
    ChunkData& getBlocks() { return data; }

    const AABB& getChunkBoundingBox() const {
        return box;
    }
};
