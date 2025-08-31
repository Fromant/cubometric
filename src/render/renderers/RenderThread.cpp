#include "RenderThread.hpp"

#include <iostream>
#include <glad/glad.h>
#include <SDL3/SDL_init.h>

#include "debug/DebugRenderer.h"
#include "game/data_loaders/globals.h"

void RenderThread::stop() {
    do_run = false;
    renderThread.join();
}

void RenderThread::start() {
if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // OpenGL Context Settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create Window
    Window = SDL_CreateWindow("Voxel Game", w, h, SDL_WINDOW_OPENGL);
    if (!Window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!SDL_SetWindowResizable(Window, true)) {
        std::cerr << "SDL_SetWindowResizable Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Hide cursor
    if (!SDL_SetWindowRelativeMouseMode(Window, captureMouse)) {
        std::cerr << "SDL_SetWindowRelativeMouseMode Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    GLContext = SDL_GL_CreateContext(Window);
    // Load OpenGL Functions (GLAD)
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Create OpenGL Context
    if (!GLContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }
    SDL_GL_MakeCurrent(Window, GLContext);

    #ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //TODO fix
    // glDebugMessageCallback(DebugCallback, nullptr);
    #endif

    // Enable depth testing and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    debugRenderer = new debug::DebugRenderer();
    textureManager.Init("assets/textures/");
    // worldRenderer.init();


    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << "\n"
        << "Renderer: " << glGetString(GL_RENDERER) << "\n"
        << "Version: " << glGetString(GL_VERSION) << std::endl;

    int profile;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);

    GLint max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);

    std::cout << "Context Profile: "
        << (profile == SDL_GL_CONTEXT_PROFILE_CORE ? "Core" : "Compatibility")
        << "Max 2d texture array layers: " << max_layers << std::endl;

    std::cout << "Starting render thread" << std::endl;

    do_run = true;
    run();
}


void RenderThread::run() {
    while (do_run) {
        // render loop
    }
}

