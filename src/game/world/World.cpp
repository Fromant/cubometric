#include "World.h"

#include <SDL3/SDL_timer.h>

#include "../../render/Camera.h"

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

bool isBoxInFrustum(const std::array<glm::vec4, 6> &frustumPlanes, const AABB &box) {
    const glm::vec3 &vmin = box.min;
    const glm::vec3 &vmax = box.max;

    for (auto g: frustumPlanes) {
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

AABB getChunkBoundingBox(const glm::vec3 &chunkPosition) {
    AABB box{};
    box.min = chunkPosition;
    box.max = chunkPosition + glm::vec3(Chunk::WIDTH, Chunk::HEIGHT, Chunk::DEPTH);
    return box;
}

int World::Render(const Camera &camera) {
    auto frustumPlanes = camera.getFrustumPlanes();

    int chunksRendered = 0;

    const auto &view = camera.getViewMatrix();
    const auto &projection = camera.getProjectionMatrix();

    for (int z = 0; z < chunks.size(); z++) {
        for (int x = 0; x < chunks[z].size(); x++) {
            const int z1 = (z - VIEW_DISTANCE) * Chunk::DEPTH;
            const int x1 = (x - VIEW_DISTANCE) * Chunk::WIDTH;
            AABB chunkBox = getChunkBoundingBox(glm::vec3(z1, 0, x1));

            if (isBoxInFrustum(frustumPlanes, chunkBox)) {
                auto *chunkBuffer = bufferPool.getBuffer(getChunkID(z, x));
                renderer.Render(view, projection, chunkBuffer, chunks[z][x].getFaces().size() * 6);
                chunksRendered++;
            }
        }
    }

    // auto gpu_time = SDL_GetTicksNS();
    // glFinish();
    // gpu_time = SDL_GetTicksNS() - gpu_time;

    glFlush();

    // std::cout << "mesh load: " << mesh_load_time / 1000000.0f << std::endl;
    // std::cout << "mesh send: " << mesh_send_time / 1000000.0f << std::endl;
    // std::cout << "gpu render: " << gpu_time / 1000000.0f << std::endl;
    // std::cout << "chunks rendered: " << chunksRendered << std::endl;
    // std::cout << "faces rendered: " << renderer.getNumVertices() << std::endl;

    return chunksRendered;
}
