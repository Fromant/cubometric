#include "WorldRenderer.h"

#include <array>
#include <iostream>
#include <unordered_set>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <SDL3/SDL_timer.h>

#include "ChunkMesher.h"

#include "game/data_loaders/globals.h"
#include "render/utils.h"
#include "render/renderers/block/CubeModel.h"


bool isBoxInFrustum(const std::array<glm::vec4, 6>& frustumPlanes, const AABB& box) {
    const glm::vec3& vmin = box.min;
    const glm::vec3& vmax = box.max;

    for (const auto& g : frustumPlanes) {
        if ((glm::dot(g, glm::vec4(vmin.x, vmin.y, vmin.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmax.x, vmin.y, vmin.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmin.x, vmax.y, vmin.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmax.x, vmax.y, vmin.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmin.x, vmin.y, vmax.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmax.x, vmin.y, vmax.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmin.x, vmax.y, vmax.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(vmax.x, vmax.y, vmax.z, 1.0f)) < 0.0)) {
            // Not visible - all returned negative
            return false;
        }
    }

    // Potentially visible
    return true;
}

AABB getChunkBoundingBox(const glm::vec3& chunkPosition) {
    AABB box{};
    box.min = chunkPosition;
    box.max = chunkPosition + glm::vec3(Chunk::WIDTH, Chunk::WIDTH, Chunk::DEPTH);
    return box;
}


void WorldRenderer::init() {
    shader = new Shader("shaders/face/vert.glsl", "shaders/face/frag.glsl", "shaders/face/geom.glsl");

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glGenBuffers(1, &indirectBuffer);

    skyRenderer.init();

    cmds.reserve(4 * Chunk::HEIGHT / Chunk::WIDTH);
}

void WorldRenderer::renderSubChunkFacing(const GPUBuffer* buf, int y, Facing f,
                                         std::vector<DrawArraysIndirectCommand>& cmds) {
    auto& view = buf->subChunks[y][f];
    if (view.size > 0)
        cmds.emplace_back(view.size * 6, 1, view.offset * 6, 0);
}

void WorldRenderer::renderSubChunk(const glm::ivec3& coords,
                                   const glm::vec3& cameraCoords,
                                   const GPUBuffer* buffer) {
    cmds.clear();
    shader->setIVec3("chunkCoords", coords);

    if (coords.x * Chunk::WIDTH - static_cast<int>(cameraCoords.x) < Chunk::WIDTH)
        renderSubChunkFacing(buffer, coords.y, SOUTH, cmds);
    if (static_cast<int>(cameraCoords.x) - coords.x * Chunk::WIDTH < Chunk::WIDTH)
        renderSubChunkFacing(buffer, coords.y, NORTH, cmds);

    if (coords.y * Chunk::SUB_HEIGHT - static_cast<int>(cameraCoords.y) < Chunk::SUB_HEIGHT)
        renderSubChunkFacing(buffer, coords.y, UP, cmds);
    if (static_cast<int>(cameraCoords.y) - coords.y * Chunk::SUB_HEIGHT < Chunk::SUB_HEIGHT)
        renderSubChunkFacing(buffer, coords.y, DOWN, cmds);

    if (coords.z * Chunk::DEPTH - static_cast<int>(cameraCoords.z) < Chunk::DEPTH)
        renderSubChunkFacing(buffer, coords.y, WEST, cmds);
    if (static_cast<int>(cameraCoords.z) - coords.z * Chunk::DEPTH < Chunk::DEPTH)
        renderSubChunkFacing(buffer, coords.y, EAST, cmds);

    if (cmds.size() == 0) return;

    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand) * cmds.size(), cmds.data(),
                 GL_DYNAMIC_DRAW);
    glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, cmds.size(), 0);
}

void WorldRenderer::renderChunk(const glm::ivec2& coords, const size_t y0, const size_t y1,
                                const glm::vec3& cameraCoords,
                                const GPUBuffer* buffer) {
    if (!buffer || buffer->size() == 0) return;

    buffer->bind();
    if (auto err = glGetError(); err != GL_NO_ERROR)
        std::cout << __FILE__ << ':' << __LINE__ << ' ' << err << std::endl;

    if (y0 == -1) return;
    for (int y = y0; y <= y1; y++) {
        // std::cout << "Rendering subchunk " << coords.x << ' ' << y << ' ' << coords.y << std::endl;
        renderSubChunk({coords.x, y, coords.y}, cameraCoords, buffer);
    }

    if (auto err = glGetError(); err != GL_NO_ERROR)
        std::cout << __FILE__ << ':' << __LINE__ << ' ' << err << std::endl;

    // buffer->notifySubmitted();
}

int WorldRenderer::render(World& world, const Camera& camera) {
    const auto& view = camera.getViewMatrix();
    const auto& proj = camera.getProjectionMatrix();

    skyRenderer.renderSkybox(view, proj, camera.Position);

    if (renderWireframe)
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureManager.getTextureArray());
    // shader->setSampler("atlasTexture", 0);
    shader->setMat4("view", view);
    shader->setMat4("projection", proj);

    auto time = SDL_GetTicks();
    float radius = 50.0f; // distance from center
    float height = 100.0f; // height above ground
    float angle = time / 1000.0f / World::DAY_DURATION_SEC; // same as before

    float x = cosf(angle) * radius;
    float z = sinf(angle) * radius;
    float y = height; // fixed height
    shader->setVec3("lightDir", glm::normalize(glm::vec3(x, y, z)));

    glBindVertexArray(VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

    auto frustum = camera.getFrustum();

    int subChunksRendered = 0;

    int xMin = camera.Position.x / Chunk::WIDTH - VIEW_DISTANCE;
    int xMax = camera.Position.x / Chunk::WIDTH + VIEW_DISTANCE;
    int yMin = 0;
    int yMax = Chunk::HEIGHT / Chunk::WIDTH;
    int zMin = camera.Position.z / Chunk::DEPTH - VIEW_DISTANCE;
    int zMax = camera.Position.z / Chunk::DEPTH + VIEW_DISTANCE;

    std::unordered_set<size_t> rendered_chunks{};

    for (int x = xMin; x < xMax; x++) {
        for (int z = zMin; z < zMax; z++) {
            int y0 = -1;
            int y1 = -1;

            const size_t id = Chunk::getId(x, z);

            if (!bufferPool.containsBuffer(id)) {
                ChunkMesher::update(world, {x, z}, bufferPool);
            }

            Chunk& chunk = *world.getChunk(x, z);
            rendered_chunks.emplace(chunk.getId());

            for (int y = yMin; y < yMax; y++) {
                AABB chunkBox = getChunkBoundingBox(glm::vec3(x * Chunk::WIDTH, y * Chunk::WIDTH, z * Chunk::DEPTH));
                if (frustum.isAABBVisible(chunkBox)) {
                    if (y0 == -1) y0 = y;
                    y1 = y;
                    subChunksRendered++;
                }
            }
            renderChunk({x, z}, y0, y1, camera.Position, bufferPool.getBuffer(id));
        }
    }

    for (const auto& chunk : world.chunks) {
        if (!rendered_chunks.contains(chunk.first)) {
            bufferPool.deleteBuffer(chunk.first);
        }
    }

    std::erase_if(world.chunks, [xMin, xMax, zMin, zMax](const auto& iter) {
        bool erase = iter.second.xCoord < xMin || iter.second.xCoord > xMax ||
            iter.second.zCoord < zMin || iter.second.zCoord > zMax;
        return erase;
    });

    // glFlush();

    return subChunksRendered;
}

WorldRenderer::~WorldRenderer() {
    delete shader;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &indirectBuffer);
}
