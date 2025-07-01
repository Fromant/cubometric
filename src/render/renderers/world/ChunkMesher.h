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

public:
    static void update(World& world, const glm::ivec2& pos, MappedBufferPool& pool);
    static void updateLOD(World& world, const glm::ivec2& pos, MappedBufferPool& pool, int LOD);
};


#endif //CHUNKMESHER_H
