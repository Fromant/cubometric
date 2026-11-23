#include "app/application.h"

#include <SDL3/SDL.h>

#include "core/check.h"
#include "core/log.h"

namespace ih::app {

Application::Application (uint32_t width, uint32_t height)
    : m_window ("IndustrialHard [Vulkan]", width, height), m_renderThread (m_window, m_frameParams) {}

int Application::run () {
    postWindowSize ();
    m_renderThread.start ();

    bool quit = false;
    SDL_Event event{};
    while (!quit) {
        while (SDL_PollEvent (&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    quit = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    postWindowSize ();
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) quit = true;
                    break;
                default:
                    break;
            }
        }
        SDL_Delay (1);
    }

    m_renderThread.stop ();
    return 0;
}

void Application::postWindowSize () {
    core::FrameParams params{};
    m_window.getSize (params.width, params.height);
    m_frameParams.post (params);
}

}  // namespace ih::app
