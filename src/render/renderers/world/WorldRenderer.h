#ifndef WORLDRENDERER_H
#define WORLDRENDERER_H

#include <vector>

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
    GLuint indirectBuffer = 0;

    bool renderWireframe = false;

    typedef struct {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;
    } DrawArraysIndirectCommand;

    void renderChunk(const glm::ivec3& coords, const glm::vec3& cameraCoords, const Chunk& chunk);
    void renderChunkFacing(const Chunk& chunk, Facing f, std::vector<DrawArraysIndirectCommand>& cmds);

public:
    int render(World& w, const Camera& c);

    void init();

    MappedBufferPool& getBufferPool() { return bufferPool; }

    void switchWireframeRendering() { renderWireframe = !renderWireframe; }

    ~WorldRenderer();
};

#endif //WORLDRENDERER_H
