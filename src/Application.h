#ifndef APPLICATION_H
#define APPLICATION_H

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
    SDL_Window *Window{};
    SDL_GLContext GLContext{};
    debug::DebugRenderer *debugRenderer{};
    WorldRenderer worldRenderer{};
    Camera camera;
    World world;
    void Init();

    bool HandleEvents();

    void Update(float deltaTime);

    void Render();
};

#endif //APPLICATION_H
