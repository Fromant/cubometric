#pragma once

#include <unordered_map>

#include "Chunk.h"
#include "worldgen/WorldGenerator.hpp"

class World {
public:
    //chunks[z][x], 2*VIEW_DISTANCE chunks in each side
    //Using deque because of O(1) index search, O(1) insertion in each side (like when moving frontwards or backwards)
    constexpr static int WORLD_HEIGHT = 256;
    // std::deque<std::deque<std::array<Chunk, WORLD_HEIGHT / Chunk::HEIGHT>>> chunks;
    std::unordered_map<size_t, Chunk> chunks;

    Chunk* getChunk(int x, int y, int z) {
        if (y < 0 || y >= WORLD_HEIGHT / Chunk::HEIGHT) return nullptr;
        auto it = chunks.find(Chunk::getId(x, y, z));
        if (it == chunks.end()) {
            //cannot find, generate
            return &generateChunk(x, y, z);
        }
        return &it->second;
    }

    Chunk const* getChunk(int x, int y, int z) const {
        auto it = chunks.find(Chunk::getId(x, y, z));
        if (it == chunks.end()) {
            return nullptr;
        }
        return &it->second;
    }

private:

    WorldGenerator generator_;

    Chunk& generateChunk(int x, int y, int z) {
        return generator_.generateChunk(x,y,z,chunks);
    }
};
