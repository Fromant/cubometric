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

AABB getSubChunkBoundingBox(const glm::ivec3& chunkPosition, int scale) {
    AABB box{};
    box.min = chunkPosition;
    box.max = box.min + glm::ivec3(Chunk::WIDTH, Chunk::SUB_HEIGHT, Chunk::DEPTH) * scale;
    return box;
}


void WorldRenderer::init() {
    shader = new Shader("shaders/face/vert.glsl", "shaders/face/frag.glsl", "shaders/face/geom.glsl");
    shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureManager.getTextureArray());
    shader->setSampler("atlasTexture", 0);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glGenBuffers(1, &indirectBuffer);

    skyRenderer.init();

    cmds.reserve(4 * Chunk::HEIGHT / Chunk::WIDTH);
}

void WorldRenderer::renderSubChunkFacing(const GPUBuffer* buf, int y, Facing f,
                                         std::vector<DrawArraysIndirectCommand>& cmds) {
    auto& view = buf->subChunks[y][f];
    if (view.size > 0) {
        if (!cmds.empty()) {
            auto& lastCMD = cmds.back();
            if (lastCMD.first + lastCMD.count == view.offset * 6) {
                lastCMD.count += view.size * 6;
                // return;
            }
        }
        cmds.emplace_back(view.size * 6, 1, view.offset * 6, 0);
    }
}

void WorldRenderer::renderSubChunk(const glm::ivec3& coords,
                                   const glm::vec3& cameraCoords,
                                   const GPUBuffer* buffer, bool& bufferBind, int scale) {
    cmds.clear();

    if (coords.x * Chunk::WIDTH - static_cast<int>(cameraCoords.x) < Chunk::WIDTH * scale)
        renderSubChunkFacing(buffer, coords.y, EAST, cmds);
    if (static_cast<int>(cameraCoords.x) - coords.x * Chunk::WIDTH < Chunk::WIDTH * scale)
        renderSubChunkFacing(buffer, coords.y, WEST, cmds);

    if (coords.y * Chunk::SUB_HEIGHT - static_cast<int>(cameraCoords.y) < Chunk::SUB_HEIGHT * scale)
        renderSubChunkFacing(buffer, coords.y, UP, cmds);
    if (static_cast<int>(cameraCoords.y) - coords.y * Chunk::SUB_HEIGHT < Chunk::SUB_HEIGHT * scale)
        renderSubChunkFacing(buffer, coords.y, DOWN, cmds);

    if (coords.z * Chunk::DEPTH - static_cast<int>(cameraCoords.z) < Chunk::DEPTH * scale)
        renderSubChunkFacing(buffer, coords.y, NORTH, cmds);
    if (static_cast<int>(cameraCoords.z) - coords.z * Chunk::DEPTH < Chunk::DEPTH * scale)
        renderSubChunkFacing(buffer, coords.y, SOUTH, cmds);

    if (cmds.empty()) return;
    if (!bufferBind) {
        bufferBind = true;
        buffer->bind();
    }
    shader->setIVec3("chunkCoords", coords);

    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand) * cmds.size(), cmds.data(),
                 GL_DYNAMIC_DRAW);
    glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, cmds.size(), 0);
}

void WorldRenderer::renderChunk(const glm::ivec2& coords, const size_t y0, const size_t y1,
                                const glm::vec3& cameraCoords,
                                const GPUBuffer* buffer, int scale) {
    if (!buffer || buffer->size() == 0) return;

    if (auto err = glGetError(); err != GL_NO_ERROR)
        std::cout << __FILE__ << ':' << __LINE__ << ' ' << err << std::endl;

    if (y0 == -1) return;
    bool bufferBind = false;
    for (int y = y0; y <= y1; y++) {
        // std::cout << "Rendering subchunk " << coords.x << ' ' << y << ' ' << coords.y << std::endl;
        renderSubChunk({coords.x, y, coords.y}, cameraCoords, buffer, bufferBind, scale);
    }

    if (auto err = glGetError(); err != GL_NO_ERROR)
        std::cout << __FILE__ << ':' << __LINE__ << ' ' << err << std::endl;

    // buffer->notifySubmitted();
}

void WorldRenderer::renderChunkGrid(const Camera& camera) {}

void WorldRenderer::renderChunks(int x, int z, int scale, World& world, std::unordered_set<size_t>& rendered_chunks,
                                 int yMin, int yMax, const Frustum& frustum, const Camera& camera,
                                 int& subChunksRendered) {
    int y0 = -1;
    int y1 = -1;

    x *= scale;
    z *= scale;

    const size_t id = Chunk::getId(x, z, scale);
    if (!bufferPool.containsBuffer(id)) {
        ChunkMesher::update(world, {x, z}, bufferPool, scale);
    }

    rendered_chunks.emplace(id);

    for (int y = yMin; y < yMax; y++) {
        AABB chunkBox = getSubChunkBoundingBox(
            glm::vec3(x * Chunk::WIDTH, y * Chunk::SUB_HEIGHT, z * Chunk::DEPTH),
            scale);
        if (frustum.isAABBVisible(chunkBox)) {
            if (y0 == -1) y0 = y;
            y1 = y;
            subChunksRendered++;
        }
    }
    renderChunk({x, z}, y0, y1, camera.Position, bufferPool.getBuffer(id), scale);
}

int WorldRenderer::render(World& world, const Camera& camera) {
    const auto& view = camera.getViewMatrix();
    const auto& proj = camera.getProjectionMatrix();

    skyRenderer.renderSkybox(view, proj, camera);


    if (renderWireframe)
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    int scale = 1;

    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("projection", proj);

    auto time = SDL_GetTicks();
    float radius = 50.0f; // distance from center
    float height = 100.0f; // height above ground
    float angle = time / 1000.0f / World::DAY_DURATION_SEC; // same as before

    float lightX = cosf(angle) * radius;
    float lightZ = sinf(angle) * radius;
    float lightY = height; // fixed height
    shader->setVec3("lightDir", glm::normalize(glm::vec3(lightX, lightY, lightZ)));

    glBindVertexArray(VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

    auto frustum = camera.getFrustum();

    int subChunksRendered = 0;

    const int RADIUS = camera.viewDistance * camera.viewDistance / scale; //in chunks

    int x0 = (camera.Position.x + Chunk::WIDTH / 2) / Chunk::WIDTH;
    int z0 = (camera.Position.z + Chunk::WIDTH / 2) / Chunk::DEPTH;
    int yMin = 0;
    int yMax = Chunk::HEIGHT / Chunk::WIDTH / scale;

    std::unordered_set<size_t> rendered_chunks{};

    shader->setInt32("uScale", scale);
    //render 4 center chunks
    for (int x = x0 - 1; x <= x0; x++) {
        for (int z = z0 - 1; z <= z0; z++) {
            renderChunks(x, z, scale, world, rendered_chunks, yMin, yMax, frustum, camera, subChunksRendered);
        }
    }

    //render all other chunks in triangle
    for (int LOD = 0; LOD < 3; LOD++) {
        for (int x = -2; x < 2; x++) {
            int z = -2;
            renderChunks(x + x0, z + z0, scale, world, rendered_chunks,
                         yMin, yMax, frustum, camera, subChunksRendered);
        }

        renderChunks(x0 - 2, z0 - 1, scale, world, rendered_chunks,
                     yMin, yMax, frustum, camera, subChunksRendered);
        renderChunks(x0 - 2, z0, scale, world, rendered_chunks,
                     yMin, yMax, frustum, camera, subChunksRendered);
        renderChunks(x0 + 1, z0 - 1, scale, world, rendered_chunks,
                     yMin, yMax, frustum, camera, subChunksRendered);
        renderChunks(x0 + 1, z0, scale, world, rendered_chunks,
                     yMin, yMax, frustum, camera, subChunksRendered);


        for (int x = -2; x < 2; x++) {
            int z = 1;
            renderChunks(x + x0, z + z0, scale, world, rendered_chunks,
                         yMin, yMax, frustum, camera, subChunksRendered);
        }

        scale *= 2;
        yMax /= 2;

        x0 = (camera.Position.x + Chunk::WIDTH * scale / 2) / Chunk::WIDTH / scale;
        z0 = (camera.Position.z + Chunk::WIDTH * scale / 2) / Chunk::DEPTH / scale;

        shader->setInt32("uScale", scale);
    }

    for (const auto& chunk : world.chunks) {
        if (!rendered_chunks.contains(chunk.first)) {
            bufferPool.deleteBuffer(chunk.first);
        }
    }

    // std::erase_if(world.chunks, [rendered_chunks](const auto& iter) {
                      // return !rendered_chunks.contains(iter.second.getId());
                  // }
    // );

    // glFlush();

    return subChunksRendered;
}

WorldRenderer::~WorldRenderer() {
    delete shader;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &indirectBuffer);
}
