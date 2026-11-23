#include "app/application.h"

#include <SDL3/SDL.h>

#include "core/check.h"
#include "core/log.h"

namespace ih::app {

Application::Application (uint32_t width, uint32_t height)
    : m_config (core::Config::load ("assets")),
      m_window ("IndustrialHard [Vulkan]", width, height),
      m_camera (m_config.render.camera, m_config.render.renderDistance),
      m_renderThread (m_window, m_frameParams, m_config) {
    m_camera.resize (width, height);
    m_lastTicks = SDL_GetTicks ();
}

int Application::run () {
    postFrameParams ();
    m_renderThread.start ();

    bool quit = false;
    SDL_Event event{};
    while (!quit) {
        const uint64_t now = SDL_GetTicks ();
        const float deltaTime = static_cast<float> (now - m_lastTicks) / 1000.0f;
        m_lastTicks = now;

        while (SDL_PollEvent (&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    quit = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED: {
                    uint32_t w = 0, h = 0;
                    m_window.getSize (w, h);
                    m_camera.resize (w, h);
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) quit = true;
                    if (event.key.key == SDLK_B) setMouseCaptured (!m_mouseCaptured);
                    if (event.key.key == SDLK_F4) m_wireframe = !m_wireframe;
                    m_camera.handleEvent (event, m_mouseCaptured);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) setMouseCaptured (!m_mouseCaptured);
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    m_camera.handleEvent (event, m_mouseCaptured);
                    break;
                default:
                    break;
            }
        }

        m_camera.update (deltaTime);
        postFrameParams ();
        SDL_Delay (1);
    }

    m_renderThread.stop ();
    return 0;
}

void Application::setMouseCaptured (bool captured) {
    m_mouseCaptured = captured;
    SDL_SetWindowRelativeMouseMode (m_window.handle (), captured);
}

void Application::postFrameParams () {
    core::FrameParams params{};
    m_window.getSize (params.width, params.height);
    params.cameraPos = m_camera.position ();
    params.viewProj = m_camera.viewProjection ();
    params.frustum = m_camera.frustum ();
    params.renderDistance = m_camera.viewDistance ();
    params.wireframe = m_wireframe;
    m_frameParams.post (params);
}

}  // namespace ih::app
