#pragma once

#include <unordered_set>
#include <vector>

#include "SkyRenderer.hpp"
#include "game/world/EFacing.h"
#include "game/world/World.h"
#include "render/buffers/MappedBufferPool.h"
#include "render/utils/Shader.h"
#include "render/Camera.h"

class WorldRenderer {
    MappedBufferPool bufferPool;

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
    void renderChunks(int x, int z, int scale, World& world, std::unordered_set<size_t>& rendered_chunks, int yMin,
                      int yMax, const Frustum& frustum, const Camera& camera, int& subChunksRendered);

    void renderChunk(const glm::ivec2& coords, size_t y0, size_t y1,
                     const glm::vec3& cameraCoords,
                     const GPUBuffer* buffer, int scale);
    void renderSubChunk(const glm::ivec3& coords,
                        const glm::vec3& cameraCoords,
                        const GPUBuffer* buffer, bool& bufferBind, int scale);
    static void renderSubChunkFacing(const GPUBuffer* buf, int y, Facing f,
                                     std::vector<DrawArraysIndirectCommand>& cmds);
    static void renderChunkGrid(const Camera& camera);

public:
    int render(World& w, const Camera& c);

    void init();

    MappedBufferPool& getBufferPool() { return bufferPool; }

    void switchWireframeRendering() { renderWireframe = !renderWireframe; }

    ~WorldRenderer();
    explicit WorldRenderer(const Camera& camera) : bufferPool(camera.viewDistance * camera.viewDistance * 4) {}
};
