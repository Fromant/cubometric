#include "render/render_thread.h"

#include <SDL3/SDL_vulkan.h>

#include "core/check.h"
#include "core/log.h"
#include "render/renderers/triangle_renderer.h"

namespace ih::render {

namespace {

std::vector<std::string> requiredInstanceExtensions (SDL_Window* /*window*/) {
    uint32_t count = 0;
    const char* const* names = SDL_Vulkan_GetInstanceExtensions (&count);
    return {names, names + count};
}

}  // namespace

RenderThread::RenderThread (platform::Window& window, core::FrameParamsQueue& params)
    : m_window (window), m_params (params) {}

RenderThread::~RenderThread () {
    stop ();
}

void RenderThread::start () {
    IH_CHECK (!m_running);
    m_running = true;
    m_thread = std::thread ([this] { loop (); });
}

void RenderThread::stop () {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable ()) m_thread.join ();
}

void RenderThread::loop () {
    try {
#ifdef ENABLE_VK_VALIDATION
        constexpr bool validation = true;
#else
        constexpr bool validation = false;
#endif
        m_context = std::make_unique<vulkan::Context> (validation, requiredInstanceExtensions (m_window.handle ()));

        uint32_t width = 0, height = 0;
        m_window.getSize (width, height);
        m_swapchain = std::make_unique<vulkan::Swapchain> ();
        m_swapchain->init (*m_context, m_window.handle (), width, height);

        m_renderers.push_back (std::make_unique<TriangleRenderer> ());
        for (auto& renderer : m_renderers) {
            renderer->init (*m_context, m_swapchain->renderPass ());
        }

        core::FrameParams latest{};
        bool haveParams = false;
        while (m_running) {
            core::FrameParams incoming{};
            if (m_params.waitLatest (incoming, std::chrono::milliseconds (4))) {
                latest = incoming;
                haveParams = true;
            }
            if (!haveParams) continue;

            if (latest.width != m_swapchain->width () || latest.height != m_swapchain->height ()) {
                m_swapchain->recreate (latest.width, latest.height);
            }

            if (!m_swapchain->beginFrame (m_frame)) continue;
            FrameContext frame{m_swapchain->commandBuffer (m_frame), m_frame, m_swapchain->imageIndex (m_frame),
                               {m_swapchain->width (), m_swapchain->height ()}};
            for (auto& renderer : m_renderers) {
                renderer->render (frame);
            }
            m_swapchain->endFrame (m_frame);
            m_frame = (m_frame + 1) % vulkan::FRAMES_IN_FLIGHT;
        }

        m_context->device ().waitIdle ();
    } catch (const std::exception& e) {
        core::logError (std::string ("Render thread failed: ") + e.what ());
        m_running = false;
    }
    // RAII teardown: renderers -> swapchain -> context (device/instance).
    m_renderers.clear ();
    m_swapchain.reset ();
    m_context.reset ();
}

}  // namespace ih::render
