#include "platform/window.h"

#include "core/check.h"

namespace ih::platform {

Window::Window (const char* title, uint32_t width, uint32_t height) {
    window = SDL_CreateWindow (title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    IH_CHECK (window != nullptr);
}

Window::~Window () {
    if (window) SDL_DestroyWindow (window);
}

void Window::getSize (uint32_t& width, uint32_t& height) const {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels (window, &w, &h);
    width = static_cast<uint32_t> (w);
    height = static_cast<uint32_t> (h);
}

}  // namespace ih::platform
