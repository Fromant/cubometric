#pragma once

#include <SDL3/SDL.h>

#include "game/world/World.h"
#include "render/renderers/debug/DebugRenderer.h"
#include "render/Camera.h"
#include "render/renderers/world/WorldRenderer.h"

class Application {
public:
    Application();

    void Run();

    ~Application();

private:

    static constexpr int w = 800;
    static constexpr int h = 600;


    SDL_Window* Window{};
    SDL_GLContext GLContext{};
    debug::DebugRenderer* debugRenderer{};
    Camera camera;
    World world;
    WorldRenderer worldRenderer{world, camera};
    bool captureMouse = false;
    void Init();

    bool HandleEvents();

    void Update(float deltaTime);

    void Render();
};
