#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "core/config.h"
#include "core/frame_params.h"
#include "platform/window.h"
#include "render/renderers/world_renderer.h"
#include "render/vulkan/arena.h"
#include "render/vulkan/context.h"
#include "render/vulkan/swapchain.h"
#include "render/vulkan/texture_array.h"
#include "world/mesher.h"
#include "world/world.h"

namespace ih::render {

// Owns every Vulkan object and the world pipeline; runs the frame loop.
// The event thread only pushes FrameParams; workers only produce meshes.
class RenderThread {
public:
    RenderThread (platform::Window& window, core::FrameParamsQueue& params, const core::Config& config);
    ~RenderThread ();

    void start ();
    void stop ();

private:
    void loop ();
    void ingestMeshes (uint32_t frame);

    platform::Window& m_window;
    core::FrameParamsQueue& m_params;
    const core::Config& m_config;
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    // Created/destroyed on the render thread, in this order.
    std::unique_ptr<vulkan::Context> m_context;
    std::unique_ptr<vulkan::Swapchain> m_swapchain;
    std::unique_ptr<vulkan::TextureArray> m_textures;
    std::unique_ptr<vulkan::Arena> m_arena;
    std::unique_ptr<vulkan::UploadRing> m_ring;
    world::FaceLayers m_faceLayers{};
    std::unique_ptr<world::World> m_world;
    std::unique_ptr<WorldRenderer> m_worldRenderer;
    uint32_t m_frame = 0;
    uint64_t m_frameCounter = 0;
};

}  // namespace ih::render
