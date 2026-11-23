#pragma once

#include "render/renderers/renderer.h"
#include "render/vulkan/buffer.h"

namespace ih::render {

// M0 smoke test: a single colored triangle proving the whole
// instance -> device -> swapchain -> pipeline -> present path works.
class TriangleRenderer final : public IRenderer {
public:
    void init (vulkan::Context& context, vk::RenderPass renderPass) override;
    void render (FrameContext& frame) override;

private:
    vulkan::Buffer m_vertexBuffer;
    vk::raii::PipelineLayout m_layout{nullptr};
    vk::raii::Pipeline m_pipeline{nullptr};
};

}  // namespace ih::render
