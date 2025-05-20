#ifndef WORLD_H
#define WORLD_H

#include <deque>
#include <unordered_map>
#include <cmath>

#include "Chunk.h"


class World {
public:
    //chunks[z][x], 2*VIEW_DISTANCE chunks in each side
    //Using deque because of O(1) index search, O(1) insertion in each side (like when moving frontwards or backwards)
    // std::deque<std::deque<Chunk>> chunks;
    constexpr static int WORLD_HEIGHT = 256;
    // std::deque<std::deque<std::array<Chunk, 16>>> chunks;
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
    Chunk& generateChunk(int x, int y, int z) {
        size_t id = Chunk::getId(x, y, z);
        Chunk generated{x, y, z};
        auto& blocks = generated.getBlocks();
        //sin wave
        for (int z1 = 0; z1 < Chunk::DEPTH; z1++) {
            for (int x1 = 0; x1 < Chunk::WIDTH; x1++) {
                const int y1 = int((sin((double(x1 + z1) / Chunk::WIDTH * M_PI)) / 2 + 0.5) * (Chunk::HEIGHT - 1));
                blocks[x1 + z1 * Chunk::DEPTH + y1 * Chunk::WIDTH * Chunk::DEPTH] = BlockType::DIRT;
            }
        }

        //full cube
        // for (int i = 0; i < Chunk::WIDTH * Chunk::HEIGHT * Chunk::DEPTH; i++) {
        //     blocks[i] = BlockType::DIRT;
        // }

        return chunks.emplace(id, generated).first->second;
    }
};


#endif //WORLD_H
