#pragma once

#include <vector>

#include "SkyRenderer.hpp"
#include "game/world/EFacing.h"
#include "game/world/World.h"
#include "render/Camera.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/utils/Shader.h"

class WorldRenderer {
    GPU::MappedChunkBuffer* bufferPool = nullptr;

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

    void renderChunk(size_t id, const glm::ivec2& coords, size_t y0, size_t y1,
                     const glm::vec3& cameraCoords,
                     const GPU::MappedChunkBuffer::ChunkBufferView& buffer);
    void renderSubChunk(const glm::ivec3& coords, const glm::vec3& cameraCoords,
                        size_t offset,
                        const GPU::MappedChunkBuffer::ChunkBufferView& buffer);
    static void renderSubChunkFacing(
        const GPU::MappedChunkBuffer::ChunkBufferView& buf, int y, Facing f,
        size_t offset, std::vector<DrawArraysIndirectCommand>& cmds);
    void renderChunkGrid(const Camera& camera);

   public:
    int render(World& w, const Camera& c);

    void init();

    GPU::MappedChunkBuffer& getBufferPool() { return *bufferPool; }

    void switchWireframeRendering() { renderWireframe = !renderWireframe; }

    ~WorldRenderer();
    explicit WorldRenderer(const Camera& camera) {}
};
