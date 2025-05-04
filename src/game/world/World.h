#ifndef WORLD_H
#define WORLD_H

#include <deque>

#include "Chunk.h"
#include "../../render/Camera.h"
#include "../../render/buffers/MappedBufferPool.h"
#include "../../render/renderers/block/BlockFaceRenderer.h"
#include "../../render/renderers/block/ChunkMappedRenderer.h"


class World {
    static constexpr int VIEW_DISTANCE = 16; //internal const for now.

    //chunks[z][x], 2*VIEW_DISTANCE chunks in each side
    //Using deque because of O(1) index search, O(1) insertion in each side (like when moving frontwards or backwards)
    std::deque<std::deque<Chunk> > chunks;

    MappedBufferPool bufferPool{VIEW_DISTANCE * VIEW_DISTANCE * 4};

    // render::BlockFaceRenderer renderer;
    ChunkMappedRenderer renderer;

public:
    explicit World() {
        //create 16x16 grid
        chunks.resize(VIEW_DISTANCE * 2);
    }

    size_t getChunkID(int z, int x) {
        return ((z - VIEW_DISTANCE) << 16) + ((x - VIEW_DISTANCE));
    }

    void Init() {
        renderer.Init();
        for (int z = 0; z < 2 * VIEW_DISTANCE; z++) {
            for (int x = 0; x < 2 * VIEW_DISTANCE; x++) {
                chunks[z].emplace_back(z - VIEW_DISTANCE, x - VIEW_DISTANCE);
                bufferPool.createBuffer(getChunkID(z, x),
                                        1000 * sizeof(FaceInstance));
                chunks[z][x].updateMesh();

                const auto &faces = chunks[z][x].getFaces();

                auto *chunkBuffer = bufferPool.getBuffer(getChunkID(z, x));

                chunkBuffer->write(faces.data(), faces.size() * sizeof(FaceInstance));
            }
        }
    }

    int Render(const Camera &camera);
};


#endif //WORLD_H
