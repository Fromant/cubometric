#pragma once

#include <map>
#include <shared_mutex>
#include <mutex>

#include "Chunk.h"
#include "globals.hpp"
#include "LowDetailChunk.hpp"
#include "worldgen/WorldGenerator.hpp"

class World {
public:
    //chunks[z][x], 2*VIEW_DISTANCE chunks in each side
    //Using deque because of O(1) index search, O(1) insertion in each side (like when moving frontwards or backwards)
    // std::deque<std::deque<std::array<Chunk, WORLD_HEIGHT / Chunk::HEIGHT>>> chunks;
    std::map<size_t, Chunk> chunks;
    mutable std::shared_mutex chunks_mutex;

    std::array<std::map<size_t, LowDetailChunk>, LOD_LEVELS> LODs;

    static constexpr float DAY_DURATION_SEC = 1.0f;

    Chunk* getChunk(int x, int z) {
        std::unique_lock guard(chunks_mutex);
        auto it = chunks.find(Chunk::getId(x, z));
        if (it == chunks.end()) {
            //cannot find, generate
            return &generateChunk(x, z);
        }
        return &it->second;
    }

    Chunk const* getChunk(int x, int z) const {
        std::shared_lock guard(chunks_mutex);
        auto it = chunks.find(Chunk::getId(x, z));
        if (it == chunks.end()) {
            return nullptr;
        }
        return &it->second;
    }

    LowDetailChunk const* getLowDetailChunk(int x, int z, int LOD) const {
        std::shared_lock guard(chunks_mutex);
        auto& lowDetailedChunks = LODs[LOD - 1];
        auto it = lowDetailedChunks.find(Chunk::getId(x, z));
        if (it == lowDetailedChunks.end()) return nullptr;
        return &it->second;
    }

    LowDetailChunk* getLowDetailChunk(int x, int z, int LOD) {
        std::unique_lock guard(chunks_mutex);
        auto& lowDetailedChunks = LODs[LOD - 1];
        auto it = lowDetailedChunks.find(Chunk::getId(x, z));
        if (it == lowDetailedChunks.end()) {
            return &generateChunkLOD(x, z, LOD);
        }
        return &it->second;
    }

private:
    WorldGenerator generator_;

    Chunk& generateChunk(int x, int z) {
        return generator_.generateChunk(x, z, chunks);
    }

    LowDetailChunk& generateChunkLOD(int x, int z, int LOD) {
        Chunk* chunk = getChunk(x, z);
        if (!chunk) {
            auto [it, b] = LODs[LOD-1].emplace(Chunk::getId(x, z), LOD);
            return it->second;
        }
        auto [it, b] = LODs[LOD-1].emplace(Chunk::getId(x, z), std::move(LowDetailChunk(LOD, *chunk)));
        return it->second;
    }
};
