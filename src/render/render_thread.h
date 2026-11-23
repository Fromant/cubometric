#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "core/frame_params.h"
#include "platform/window.h"
#include "render/renderers/renderer.h"
#include "render/vulkan/context.h"
#include "render/vulkan/swapchain.h"

namespace ih::render {

// Owns every Vulkan object and runs the frame loop. The event thread only
// pushes FrameParams; nothing else crosses the thread boundary.
class RenderThread {
public:
    RenderThread (platform::Window& window, core::FrameParamsQueue& params);
    ~RenderThread ();

    void start ();
    void stop ();

private:
    void loop ();

    platform::Window& m_window;
    core::FrameParamsQueue& m_params;
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    // Created/destroyed on the render thread. Declaration order matters:
    // renderers die before the swapchain, which dies before the device.
    std::unique_ptr<vulkan::Context> m_context;
    std::unique_ptr<vulkan::Swapchain> m_swapchain;
    std::vector<std::unique_ptr<IRenderer>> m_renderers;
    uint32_t m_frame = 0;
};

}  // namespace ih::render
