#pragma once

#include <cstdint>

#include "core/frame_params.h"
#include "platform/window.h"
#include "render/render_thread.h"

namespace ih::app {

// Main (event) thread: owns the window, pumps SDL events and pushes frame
// parameters to the render thread. Game state (camera, input) joins here.
class Application {
public:
    Application (uint32_t width, uint32_t height);
    int run ();

private:
    void postWindowSize ();

    platform::Window m_window;
    core::FrameParamsQueue m_frameParams;
    render::RenderThread m_renderThread;
};

}  // namespace ih::app
