#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "BlockType.h"
#include "Chunk.h"

struct LowDetailChunkData {
    const int width;
    const int height;
    const int depth;

    std::vector<BlockType> data;

    short getID(const glm::ivec3& pos) const {
        return pos.x + pos.z * width + pos.y * width * depth;
    }

    LowDetailChunkData(int width, int height, int depth) : width(width), height(height), depth(depth),
                                                           data(width * height * depth) {}

    bool containsBlock(const glm::ivec3& pos) const {
        return data[getID(pos)] != BlockType::AIR;
    }
};

class LowDetailChunk {
    LowDetailChunkData data_;
    int LOD_;

public:
    explicit LowDetailChunk(int LOD) : data_(Chunk::WIDTH >> LOD, Chunk::HEIGHT >> LOD, Chunk::DEPTH >> LOD),
                              LOD_(LOD) {}

    LowDetailChunk(int LOD, Chunk& chunk) : data_(Chunk::WIDTH >> LOD, Chunk::HEIGHT >> LOD, Chunk::DEPTH >> LOD),
                                            LOD_(LOD) {
        int step = 1 << LOD;
        for (int y = 0; y < Chunk::HEIGHT; y += step) {
            for (int z = 0; z < Chunk::DEPTH; z += step) {
                for (int x = 0; x < Chunk::WIDTH; x += step) {
                    bool done = false;
                    int y0 = 0;
                    int z0 = 0;
                    int x0 = 0;
                    for (; y0 < step; y0++) {
                        if (done) break;
                        for (; z0 < step; z0++) {
                            if (done) break;
                            for (; x0 < step; x0++) {
                                if (chunk.data.containsBlock({x + x0, y + y0, z + z0})) {
                                    done = true;
                                    break;
                                }
                            }
                        }
                    }
                    data_.data[data_.getID({x >> LOD, y >> LOD, z >> LOD})] =
                        chunk.data.getBlock({x + x0 % step, y + y0 % step, z + z0 % step});
                }
            }
        }
    }

    const LowDetailChunkData& getBlocks() { return data_; }
    int getLODLevel() const { return LOD_; }
};
