#pragma once

#include "render/vulkan/context.h"
#include "render/vulkan/vk.h"

namespace ih::render {

// Everything a renderer needs to record one frame's commands.
struct FrameContext {
    vk::raii::CommandBuffer& cmd;
    uint32_t frameIndex;
    uint32_t imageIndex;
    vk::Extent2D extent;
};

// A render pass participant. init() runs once on the render thread;
// render() records commands into the active frame's command buffer.
class IRenderer {
public:
    virtual ~IRenderer () = default;
    virtual void init (vulkan::Context& context, vk::RenderPass renderPass) = 0;
    virtual void render (FrameContext& frame) = 0;
};

}  // namespace ih::render
