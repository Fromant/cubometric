#ifndef WORLDRENDERER_H
#define WORLDRENDERER_H

#include "game/world/EFacing.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/utils/Shader.h"
#include "render/Camera.h"

class WorldRenderer {
    constexpr static int VIEW_DISTANCE = 8;
    MappedBufferPool bufferPool{VIEW_DISTANCE * VIEW_DISTANCE * 4};

    Shader* shader = nullptr;

    GLuint VAO = 0;

    bool renderWireframe = false;

    void renderChunk(const glm::ivec3& coords, const glm::vec3& cameraCoords, const Chunk& chunk);
    void renderChunkFacing(const Chunk& chunk, Facing f);

public:
    int render(World& w, const Camera& c);

    void init();

    MappedBufferPool& getBufferPool() { return bufferPool; }

    void switchWireframeRendering() { renderWireframe = !renderWireframe; }

    ~WorldRenderer();
};

#endif //WORLDRENDERER_H
