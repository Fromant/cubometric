#ifndef CHUNKMESHER_H
#define CHUNKMESHER_H

#include <vector>

#include "game/world/Chunk.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/models/CubeModel.h"

class ChunkMesher {
    std::array<std::array<std::unordered_map<short, unsigned int>, 6>, Chunk::SUB_COUNT> chunkFaces;
    std::array<std::array<std::vector<FaceMesh>, 6>, Chunk::SUB_COUNT>
    greedChunkFaces;
    std::vector<bool> processed = std::vector<bool>(Chunk::WIDTH * Chunk::WIDTH, false);

    std::array<bool, 6> checkAdjacentBlocks(World& world,
                                            const ChunkData& chunkData,
                                            const glm::ivec2& chunkPos,
                                            const glm::ivec3& blockPos);

    void generateChunkMeshData(World& world, const glm::ivec2& chunkPos);

    void generateChunkMesh(const glm::ivec2& chunkPos,
                           GPU::MappedChunkBuffer& pool);

    void greedyMesh();

public:
    void update(World& world, const glm::ivec2& chunkPos,
                GPU::MappedChunkBuffer& pool);
};

#endif  // CHUNKMESHER_H
