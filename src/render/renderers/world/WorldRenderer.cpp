#include "WorldRenderer.h"

#include <array>
#include <set>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "../../../../3rdparty/stb_image.h"


class Camera;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

bool isBoxInFrustum(const std::array<glm::vec4, 6>& frustumPlanes, const AABB& box) {
    const glm::vec3& vmin = box.min;
    const glm::vec3& vmax = box.max;

    for (auto g : frustumPlanes) {
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
    shader = new Shader("../shaders/face/vert.glsl", "../shaders/face/frag.glsl");

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    //only 1 texture lol
    glGenTextures(1, &texture);
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    auto image = stbi_load("../assets/textures/blocks/dirt.png", &w, &h, &channels, STBI_rgb_alpha);
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

void WorldRenderer::renderChunk(const glm::mat4& view, const glm::mat4& proj, GPUBuffer* buffer,
                                int numVertices, const glm::ivec3& coords) const {
    if (!buffer || numVertices == 0) return;

    shader->use();
    if (auto err = glGetError(); err!=GL_NO_ERROR) {
        std::cout << __LINE__ << err << std::endl;
    }
    buffer->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader->setSampler("atlasTexture", 0);
    shader->setMat4("view", view);
    shader->setMat4("projection", proj);
    shader->setIVec3("chunkCoords", coords);
    //TODO cut start and end based on camera position (render only what's above and lower than camera based on looking)
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    // buffer->notifySubmitted();
}

int WorldRenderer::render(World& world, const Camera& camera) {
    glBindVertexArray(VAO);
    auto frustumPlanes = camera.getFrustumPlanes();

    int chunksRendered = 0;

    const auto& view = camera.getViewMatrix();
    const auto& projection = camera.getProjectionMatrix();
    const int y = 0;
    std::set<size_t> rendered_chunks{};
    for (int z = camera.Position.z / 16 - VIEW_DISTANCE; z < camera.Position.z / 16 + VIEW_DISTANCE; z++) {
        for (int x = camera.Position.x / 16 - VIEW_DISTANCE; x < camera.Position.x / 16 + VIEW_DISTANCE; x++) {
            AABB chunkBox = getChunkBoundingBox(glm::vec3(x * Chunk::WIDTH, 0, z * Chunk::DEPTH));
            if (isBoxInFrustum(frustumPlanes, chunkBox)) {
                Chunk& chunk = world.getChunk(x, y, z);
                const size_t id = chunk.getId();
                rendered_chunks.emplace(id);
                if (!bufferPool.containsBuffer(id)) {
                    ChunkMesher::update(chunk, bufferPool);
                }
                auto* chunkBuffer = bufferPool.getBuffer(id);
                glm::ivec3 coords{chunk.xCoord, chunk.yCoord, chunk.zCoord};
                renderChunk(view, projection, chunkBuffer, chunkBuffer->size() / sizeof(Vertex),coords);
                chunksRendered++;
            }
        }
    }

    for (auto chunk : world.chunks) {
        if (!rendered_chunks.contains(chunk.first)) {
            rendered_chunks.erase(chunk.first);
            bufferPool.deleteBuffer(chunk.first);
        }
    }

    glFlush();

    return chunksRendered;
}
