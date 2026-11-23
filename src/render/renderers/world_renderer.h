#pragma once

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "core/frustum.h"
#include "render/renderers/renderer.h"
#include "render/vulkan/arena.h"
#include "render/vulkan/texture_array.h"
#include "world/chunk.h"

namespace ih::render {

// Draws all uploaded chunk meshes from one arena buffer. One draw call per
// visible chunk part; per-chunk origin goes through push constants.
class WorldRenderer final : public IRenderer {
public:
    struct DrawRecord {
        world::ChunkCoord coord{};
        vk::DeviceSize vertexOffset = 0;  // bytes into the arena
        vk::DeviceSize indexOffset = 0;   // bytes into the arena
        uint32_t indexCount = 0;
        core::AABB bounds{};
    };

    // Must be called before init().
    void setResources (const vulkan::TextureArray& textures, const vulkan::Arena& arena);
    void setChunkDims (const core::ChunkDims& dims) { m_dims = dims; }

    void init (vulkan::Context& context, vk::RenderPass renderPass) override;
    void render (FrameContext& frame) override;

    // Render-thread frame setup.
    void beginFrame (const glm::mat4& viewProj, const glm::vec3& sunDir, float ambient,
                     const core::Frustum& frustum, bool wireframe);

    // Draw registry maintenance (render thread only).
    void addDraw (DrawRecord record);
    void removeDraws (world::ChunkCoord coord);
    size_t drawCount () const { return m_draws.size (); }

private:
    struct PushConstants {
        glm::mat4 viewProj{1.0f};
        glm::vec3 sunDir{0.0f, 1.0f, 0.0f};
        float ambient = 0.5f;
        glm::vec3 chunkOrigin{0.0f};
        float pad = 0.0f;
    };

    void createPipeline (vulkan::Context& context, vk::RenderPass renderPass, bool wireframe);

    const vulkan::TextureArray* m_textures = nullptr;
    const vulkan::Arena* m_arena = nullptr;
    core::ChunkDims m_dims{};

    vk::raii::DescriptorSetLayout m_setLayout{nullptr};
    vk::raii::DescriptorPool m_pool{nullptr};
    vk::raii::DescriptorSet m_set{nullptr};
    vk::raii::PipelineLayout m_layout{nullptr};
    vk::raii::Pipeline m_pipeline{nullptr};
    vk::raii::Pipeline m_wirePipeline{nullptr};

    std::vector<DrawRecord> m_draws{};
    std::unordered_map<world::ChunkCoord, std::vector<size_t>, world::ChunkCoordHash> m_byChunk{};

    PushConstants m_push{};
    core::Frustum m_frustum{};
    bool m_wireframe = false;
};

}  // namespace ih::render
