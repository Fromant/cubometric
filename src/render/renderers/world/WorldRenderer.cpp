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

AABB getSubChunkBoundingBox(const glm::vec3& chunkPosition) {
    AABB box{};
    box.min = chunkPosition;
    box.max = chunkPosition + glm::vec3(Chunk::WIDTH, Chunk::SUB_HEIGHT, Chunk::DEPTH);
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
            }
        }
        cmds.emplace_back(view.size * 6, 1, view.offset * 6, 0);
    }
}

void WorldRenderer::renderSubChunk(const glm::ivec3& coords,
                                   const glm::vec3& cameraCoords,
                                   const GPUBuffer* buffer, bool& bufferBind) {
    cmds.clear();

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
                                const GPUBuffer* buffer) {
    if (!buffer || buffer->size() == 0) return;

    if (auto err = glGetError(); err != GL_NO_ERROR)
        std::cout << __FILE__ << ':' << __LINE__ << ' ' << err << std::endl;

    if (y0 == -1) return;
    bool bufferBind = false;
    for (int y = y0; y <= y1; y++) {
        // std::cout << "Rendering subchunk " << coords.x << ' ' << y << ' ' << coords.y << std::endl;
        renderSubChunk({coords.x, y, coords.y}, cameraCoords, buffer, bufferBind);
    }

    if (auto err = glGetError(); err != GL_NO_ERROR)
        std::cout << __FILE__ << ':' << __LINE__ << ' ' << err << std::endl;

    // buffer->notifySubmitted();
}

void WorldRenderer::renderChunkGrid(const Camera& camera) {
    //TODO bad calc
    float dx = -camera.Position.x + int(camera.Position.x) / Chunk::WIDTH * Chunk::WIDTH;
    float dy = -camera.Position.y;
    float dz = -camera.Position.z + int(camera.Position.z) / Chunk::DEPTH * Chunk::DEPTH;

    constexpr int step = 2;

    //TODO immediate mode unsupported in modern OpenGL. Switch to shaders
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for grid lines
    for (int z = 0; z <= 0 + Chunk::DEPTH; z += Chunk::DEPTH) {
        for (int x = 0; x <= 0 + Chunk::WIDTH; x += step) {
            for (int y = 0; y <= Chunk::HEIGHT; y += step) {
                // Draw rectangle
                glVertex3f(x + dx, y + dy, z + dz);
                glVertex3f(x + step + dx, y + dy, z + dz);
                // Right edge
                glVertex3f(x + step + dx, y + dy, z + dz);
                glVertex3f(x + step + dx, y + step + dy, z + dz);
                // Bottom edge
                glVertex3f(x + step + dx, y + step + dy, z + dz);
                glVertex3f(x + dx, y + step + dy, z + dz);
                // Left edge
                glVertex3f(x + dx, y + step + dy, z + dz);
                glVertex3f(x + dx, y + dy, z + dz);
            }
        }
    }
    glEnd();
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

    int xMin = camera.Position.x / Chunk::WIDTH - camera.viewDistance;
    int xMax = camera.Position.x / Chunk::WIDTH + camera.viewDistance;
    int zMin = camera.Position.z / Chunk::DEPTH - camera.viewDistance;
    int zMax = camera.Position.z / Chunk::DEPTH + camera.viewDistance;
    int yMin = 0;
    int yMax = Chunk::HEIGHT / Chunk::WIDTH;

    std::unordered_set<size_t> rendered_chunks{};

    const int RADIUS = camera.viewDistance * camera.viewDistance * Chunk::WIDTH * Chunk::WIDTH;

    for (int x = xMin; x < xMax; x++) {
        for (int z = zMin; z < zMax; z++) {
            float chunkCenterX = (x + 0.5f) * Chunk::WIDTH;
            float chunkCenterZ = (z + 0.5f) * Chunk::DEPTH;

            float dx = chunkCenterX - camera.Position.x;
            float dz = chunkCenterZ - camera.Position.z;
            float distanceSquared = dx * dx + dz * dz;

            if (distanceSquared > RADIUS) continue;

            // const int LODLevel = std::min(int(sqrtf(distanceSquared) / Chunk::WIDTH / LOD_DISTANCE), LOD_LEVELS);

            int y0 = -1;
            int y1 = -1;

            const size_t id = Chunk::getId(x, z);
            if (!bufferPool.containsBuffer(id)) {
                // if (LODLevel == 0)
                ChunkMesher::update(world, {x, z}, bufferPool);
                // else ChunkMesher::updateLOD(world, {x, z}, bufferPool, LODLevel);
            }

            rendered_chunks.emplace(Chunk::getId(x, z));

            for (int y = yMin; y < yMax; y++) {
                AABB chunkBox = getSubChunkBoundingBox(glm::vec3(x * Chunk::WIDTH, y * Chunk::WIDTH, z * Chunk::DEPTH));
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
        bool erase = iter.second.xCoord < xMin - 1 || iter.second.xCoord > xMax + 1 ||
            iter.second.zCoord < zMin - 1 || iter.second.zCoord > zMax + 1;
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
