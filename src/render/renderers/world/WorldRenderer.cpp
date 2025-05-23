#include "WorldRenderer.h"

#include <array>
#include <unordered_set>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "ChunkMesher.h"
#include "../../../../3rdparty/stb_image.h"

#include "../block/FaceInstance.h"


class Camera;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

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
    box.max = chunkPosition + glm::vec3(Chunk::WIDTH, Chunk::HEIGHT, Chunk::DEPTH);
    return box;
}

void WorldRenderer::init() {
    shader = new Shader("shaders/face/vert.glsl", "shaders/face/frag.glsl");

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    //only 1 texture lol
    glGenTextures(1, &texture);
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    auto image = stbi_load("assets/textures/blocks/dirt.png", &w, &h, &channels, STBI_rgb_alpha);
    glBindTexture(GL_TEXTURE_2D, texture);
    if (channels != 4) {
        std::cerr << "channels != 4" << std::endl;
        exit(EXIT_FAILURE);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void WorldRenderer::renderChunkFacing(const Chunk& chunk, Facing f) {
    glDrawArrays(GL_TRIANGLES, chunk.faceOffsets[f] * sizeof(FaceInstance) / sizeof(Vertex),
                 chunk.faceSizes[f] * sizeof(FaceInstance) / sizeof(Vertex));
}

void WorldRenderer::renderChunk(const glm::ivec3& coords, const glm::vec3& cameraCoords,
                                const Chunk& chunk) {
    shader->setIVec3("chunkCoords", coords);

    auto* buffer = bufferPool.getBuffer(chunk.getId());

    if (!buffer || buffer->size() == 0) return;

    buffer->bind();

    if (coords.x * Chunk::WIDTH - static_cast<int>(cameraCoords.x) < Chunk::WIDTH)
        renderChunkFacing(chunk, SOUTH);
    if (static_cast<int>(cameraCoords.x) - coords.x * Chunk::WIDTH < Chunk::WIDTH)
        renderChunkFacing(chunk, NORTH);

    if (coords.y * Chunk::HEIGHT - static_cast<int>(cameraCoords.y) < Chunk::HEIGHT)
        renderChunkFacing(chunk, UP);
    if (static_cast<int>(cameraCoords.y) - coords.y * Chunk::HEIGHT < Chunk::HEIGHT)
        renderChunkFacing(chunk, DOWN);

    if (coords.z * Chunk::DEPTH - static_cast<int>(cameraCoords.z) < Chunk::DEPTH)
        renderChunkFacing(chunk, WEST);
    if (static_cast<int>(cameraCoords.z) - coords.z * Chunk::DEPTH < Chunk::DEPTH)
        renderChunkFacing(chunk, EAST);

    // buffer->notifySubmitted();
}

int WorldRenderer::render(World& world, const Camera& camera) {
    const auto& view = camera.getViewMatrix();
    const auto& proj = camera.getProjectionMatrix();
    if (renderWireframe)
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader->setSampler("atlasTexture", 0);
    shader->setMat4("view", view);
    shader->setMat4("projection", proj);

    glBindVertexArray(VAO);

    auto frustumPlanes = camera.getFrustumPlanes();

    int chunksRendered = 0;

    std::unordered_set<size_t> rendered_chunks{};
    for (int y = 0; y < World::WORLD_HEIGHT / Chunk::HEIGHT; y++) {
        for (int z = camera.Position.z / Chunk::DEPTH - VIEW_DISTANCE; z < camera.Position.z / Chunk::DEPTH + VIEW_DISTANCE - 1; z++) {
            for (int x = camera.Position.x / Chunk::WIDTH - VIEW_DISTANCE; x < camera.Position.x / Chunk::WIDTH + VIEW_DISTANCE - 1; x++) {
                AABB chunkBox = getChunkBoundingBox(glm::vec3(x * Chunk::WIDTH, y * Chunk::HEIGHT, z * Chunk::DEPTH));
                if (isBoxInFrustum(frustumPlanes, chunkBox)) {
                    Chunk& chunk = *world.getChunk(x, y, z);
                    const size_t id = chunk.getId();
                    rendered_chunks.emplace(id);
                    if (!bufferPool.containsBuffer(id)) {
                        ChunkMesher::update(world, chunk, bufferPool);
                    }
                    glm::ivec3 coords{chunk.xCoord, chunk.yCoord, chunk.zCoord};
                    renderChunk(coords, camera.Position, chunk);
                    chunksRendered++;
                }
            }
        }
    }

    for (auto chunk : world.chunks) {
        if (!rendered_chunks.contains(chunk.first)) {
            rendered_chunks.erase(chunk.first);
            bufferPool.deleteBuffer(chunk.first);
        }
    }

    // glFlush();

    return chunksRendered;
}
