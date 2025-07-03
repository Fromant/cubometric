#ifndef CHUNKMESHER_H
#define CHUNKMESHER_H

#include <vector>

#include "game/world/Chunk.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/renderers/block/CubeModel.h"

class ChunkMesher {
    static inline std::array<std::array<std::unordered_map<short, unsigned int>, 6>, Chunk::SUB_COUNT> chunkFaces;
    static inline std::array<std::array<std::vector<FaceMesh>, 6>, Chunk::SUB_COUNT> greedChunkFaces;
    static inline std::vector<bool> processed = std::vector<bool>(Chunk::WIDTH * Chunk::WIDTH, false);

    // static inline std::array<std::array<std::vector<FaceMesh>, 6>, Chunk::SUB_COUNT> chunkMesh{};
    static inline std::vector<FaceMesh> totalFaces;

    static std::array<bool, 6>
    checkAdjacentBlocks(World& world, const ChunkData& chunkData, const glm::ivec2& chunkPos,
                        const glm::ivec3& blockPos);
    static void generateChunkMeshData(World& world, const glm::ivec2& chunkPos);
    static void generateChunkMesh(const glm::ivec2& chunkPos, MappedBufferPool& pool);
    static void greedyMesh();

public:
    static void update(World& world, const glm::ivec2& chunkPos, MappedBufferPool& pool);
};


#endif //CHUNKMESHER_H
