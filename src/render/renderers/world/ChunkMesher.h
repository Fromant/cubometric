#ifndef CHUNKMESHER_H
#define CHUNKMESHER_H

#include <vector>

#include "game/world/Chunk.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/renderers/block/CubeModel.h"

class ChunkMesher {
    static inline std::array<std::vector<FaceInstance>, 6> buffer{};

public:
    static void update(World& world, Chunk& chunk, MappedBufferPool& pool);
};


#endif //CHUNKMESHER_H
