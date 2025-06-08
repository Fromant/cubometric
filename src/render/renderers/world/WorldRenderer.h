#pragma once

#include <vector>

#include "SkyRenderer.hpp"
#include "game/world/EFacing.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/utils/Shader.h"
#include "render/Camera.h"

class WorldRenderer {
    constexpr static int VIEW_DISTANCE = 16;
    MappedBufferPool bufferPool{VIEW_DISTANCE * VIEW_DISTANCE * 4};

    Shader* shader = nullptr;
    SkyRenderer skyRenderer;

    GLuint VAO = 0;
    GLuint indirectBuffer = 0;

    bool renderWireframe = false;

    typedef struct {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;
    } DrawArraysIndirectCommand;

    std::vector<DrawArraysIndirectCommand> cmds;

    void renderChunk(const glm::ivec2& coords, size_t y0, size_t y1,
                     const glm::vec3& cameraCoords,
                     const GPUBuffer* buffer);
    void renderSubChunk(const glm::ivec3& coords,
                        const glm::vec3& cameraCoords,
                        const GPUBuffer* buffer);
    static void renderSubChunkFacing(const GPUBuffer* buf, int y, Facing f,
                                     std::vector<DrawArraysIndirectCommand>& cmds);

public:
    int render(World& w, const Camera& c);

    void init();

    MappedBufferPool& getBufferPool() { return bufferPool; }

    void switchWireframeRendering() { renderWireframe = !renderWireframe; }

    ~WorldRenderer();
};
