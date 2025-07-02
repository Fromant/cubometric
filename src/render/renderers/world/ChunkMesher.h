#ifndef CHUNKMESHER_H
#define CHUNKMESHER_H

#include <vector>

#include "game/world/Chunk.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/renderers/block/CubeModel.h"

class ChunkMesher {
    static inline std::array<std::array<std::vector<FaceInstance>, 6>, Chunk::SUB_COUNT> buffer{};
    static inline std::vector<FaceInstance> totalFaces;

    static std::array<bool, 6>
    checkAdjacentBlocks(World& world, const ChunkData& chunkData, const glm::ivec2& chunkPos, const glm::ivec3& blockPos);

public:
    static void update(World& world, const glm::ivec2& chunkPos, MappedBufferPool& pool);
    static void updateLOD(World& world, const glm::ivec2& pos, MappedBufferPool& pool, int LOD);
};


#endif //CHUNKMESHER_H
