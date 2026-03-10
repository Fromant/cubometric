#pragma once

#include <vector>

#include "SkyRenderer.hpp"
#include "game/world/EFacing.h"
#include "game/world/World.h"
#include "render/Camera.h"
#include "render/buffers/ChunkMeshQueue.hpp"
#include "render/buffers/MappedBufferPool.h"
#include "render/utils/Shader.h"

class WorldRenderer {
    GPU::MappedChunkBuffer *bufferPool = nullptr;

    Shader *shader = nullptr;
    SkyRenderer skyRenderer;

    GLuint VAO = 0;
    GLuint cmdBuffer = 0;

    World &world;
    const Camera &camera;

    bool renderWireframe = false;

    typedef struct {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;
    } DrawArraysIndirectCommand;

    std::vector<DrawArraysIndirectCommand> cmds;

    ChunkMeshQueue chunkMeshQueue_;

    void renderChunk(size_t id, const glm::ivec2 &coords, size_t y0, size_t y1,
                     const GPU::MappedChunkBuffer::ChunkBufferView &buffer);

    void renderSubChunk(const glm::ivec3 &coords,
                        size_t offset,
                        const GPU::MappedChunkBuffer::ChunkBufferView &buffer);

    static void renderSubChunkFacing(
        const GPU::MappedChunkBuffer::ChunkBufferView &buf, int y, Facing f,
        size_t offset, std::vector<DrawArraysIndirectCommand> &cmds);

    void renderChunkGrid();

    std::atomic_bool do_mesh = false;

    static void meshLoop(WorldRenderer *const t);

public:
    WorldRenderer(World &w, Camera &c) : world(w), camera(c) {
    }

    int render();

    void init();

    void switchWireframeRendering() { renderWireframe = !renderWireframe; }

    ~WorldRenderer();
};
