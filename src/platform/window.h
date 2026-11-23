#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

namespace ih::platform {

// Thin RAII wrapper over an SDL3 window.
class Window {
public:
    Window (const char* title, uint32_t width, uint32_t height);
    ~Window ();

    Window (const Window&) = delete;
    Window& operator= (const Window&) = delete;

    SDL_Window* handle () const { return window; }

    void getSize (uint32_t& width, uint32_t& height) const;

private:
    SDL_Window* window = nullptr;
};

}  // namespace ih::platform
