#pragma once

#include <cstdint>

#include "app/camera.h"
#include "core/config.h"
#include "core/frame_params.h"
#include "platform/window.h"
#include "render/render_thread.h"

namespace ih::app {

// Main (event) thread: window, input, camera. Publishes frame parameters to
// the render thread; never touches Vulkan or world data.
class Application {
public:
    Application (uint32_t width, uint32_t height);
    int run ();

private:
    void postFrameParams ();
    void setMouseCaptured (bool captured);

    core::Config m_config;
    platform::Window m_window;
    Camera m_camera;
    core::FrameParamsQueue m_frameParams;
    render::RenderThread m_renderThread;
    bool m_mouseCaptured = false;
    bool m_wireframe = false;
    uint64_t m_lastTicks = 0;
};

}  // namespace ih::app
