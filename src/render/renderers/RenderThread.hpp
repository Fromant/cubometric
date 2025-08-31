#pragma once
#include <thread>
#include <SDL3/SDL_video.h>

#include "world/WorldRenderer.h"

namespace debug {
    class DebugRenderer;
}

/// Thread that handles user input and draws everything on the screen
class RenderThread {
    static constexpr int w = 800;
    static constexpr int h = 600;

    SDL_Window* Window{};
    SDL_GLContext GLContext{};
    bool captureMouse = false;

    void start();
    void run();

    debug::DebugRenderer* debugRenderer{};
    // WorldRenderer worldRenderer;
    Camera camera{w, h};

    std::thread renderThread;
    std::atomic_bool do_run = false;

public:
    RenderThread() {
        renderThread = std::thread(start, this);
    }

    void stop();

    ~RenderThread() {
        stop();
        renderThread.join();
    }
};
