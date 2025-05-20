#ifndef WORLDRENDERER_H
#define WORLDRENDERER_H

#include "../../../game/world/World.h"
#include "../../buffers/MappedBufferPool.h"
#include "../../utils/Shader.h"
#include "../../Camera.h"

class WorldRenderer {
    MappedBufferPool bufferPool{VIEW_DISTANCE * VIEW_DISTANCE * 4};

    Shader* shader = nullptr;

    GLuint VAO = 0;
    GLuint texture = 0;

    void renderChunk(const glm::ivec3& coords, const glm::vec3& cameraCoords, const Chunk& chunk);
    void renderChunkFacing(const glm::ivec3& coords, const glm::vec3& cameraCoords,
                                const Chunk& chunk, Facing f);

public:
    static constexpr int VIEW_DISTANCE = 12;

    int render(World& w, const Camera& c);

    void init();

    MappedBufferPool& getBufferPool() { return bufferPool; }
};

#endif //WORLDRENDERER_H
