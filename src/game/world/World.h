#ifndef WORLD_H
#define WORLD_H

#include <deque>
#include <map>

#include "Chunk.h"
#include "../../render/renderers/world/ChunkMesher.h"


class World {
public:
    //chunks[z][x], 2*VIEW_DISTANCE chunks in each side
    //Using deque because of O(1) index search, O(1) insertion in each side (like when moving frontwards or backwards)
    // std::deque<std::deque<Chunk>> chunks;
    std::map<size_t, Chunk> chunks;

    void updateChunk(int x, int y, int z, MappedBufferPool& pool) {
        auto it = chunks.find(Chunk::getId(x, y, z));
        if (it != chunks.end()) ChunkMesher::update(it->second, pool);
    }

    Chunk& getChunk(int x, int y, int z) {
        auto it = chunks.find(Chunk::getId(x, y, z));
        if (it == chunks.end()) {
            //cannot find, generate
            return generateChunk(x, y, z);
        }
        return it->second;
    }

private:
    Chunk& generateChunk(int x, int y, int z) {
        size_t id = Chunk::getId(x, y, z);
        Chunk generated{x, y, z};
        //sin wave
        auto& blocks = generated.getBlocks();
        for (int z1 = 0; z1 < Chunk::DEPTH; z1++) {
            for (int x1 = 0; x1 < Chunk::WIDTH; x1++) {
                const int y1 = int(abs(sin(double(x1 + z1)/M_PI_4))*16);
                blocks[x1 + z1 * Chunk::DEPTH + y1 * Chunk::WIDTH * Chunk::DEPTH] = BlockType::DIRT;
            }
        }

        return chunks.emplace(id, generated).first->second;
    }
};


#endif //WORLD_H
