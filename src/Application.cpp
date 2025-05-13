#include "Application.h"

#include <iostream>
#include <fstream>

#include "game/data_loaders/globals.h"

#include "render/globals.h"

Application::Application(): camera(float(render::screenWidth) / float(render::screenHeight)) {
    Init();
}

void Application::Run() {
    constexpr Uint64 TARGET_FPS = 60;
    constexpr Uint64 TARGET_FRAME_TIME = 1000 / TARGET_FPS; // ms per frame


    GLuint queryID;
    glGenQueries(1, &queryID);

    Uint64 lastFrame = SDL_GetTicks();
    while (true) {
        Uint64 currentFrame = SDL_GetTicks();
        //cpu frame time
        Uint64 deltaTime = currentFrame - lastFrame;


        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Cap frame rate
        if (deltaTime < TARGET_FRAME_TIME) {
            SDL_Delay(TARGET_FRAME_TIME - deltaTime);
            currentFrame = SDL_GetTicks(); // Update after delay
            deltaTime = currentFrame - lastFrame;
        }

        lastFrame = currentFrame;

        // Rest of the loop (HandleEvents, Update, Render)
        if (!HandleEvents()) break;
        Update(deltaTime / 1000.0f); // Convert ms to seconds

        glBeginQuery(GL_TIME_ELAPSED, queryID);
        Render();
        glEndQuery(GL_TIME_ELAPSED);

        GLuint64 elapsedTime = 0;
        glGetQueryObjectui64v(queryID, GL_QUERY_RESULT, &elapsedTime);
        debugRenderer->Render(elapsedTime / 1000000.0f, render::screenWidth, render::screenHeight);

        SDL_GL_SwapWindow(Window);
    }
}

void Application::Update(float deltaTime) {
    camera.Update(deltaTime);
}


void Application::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // OpenGL Context Settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create Window
    Window = SDL_CreateWindow("Voxel Game", render::screenWidth, render::screenHeight, SDL_WINDOW_OPENGL);
    if (!Window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!SDL_SetWindowResizable(Window, true)) {
        std::cerr << "SDL_SetWindowResizable Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Hide cursor
    SDL_HideCursor();
    SDL_SetWindowRelativeMouseMode(Window, true);

    GLContext = SDL_GL_CreateContext(Window);
    // Load OpenGL Functions (GLAD)
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Create OpenGL Context
    if (!GLContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }
    SDL_GL_MakeCurrent(Window, GLContext);

    // Enable depth testing and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    debugRenderer = new debug::DebugRenderer();
    textureManager.Init("../assets/textures/");
    worldRenderer.init();


    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << "\n"
        << "Renderer: " << glGetString(GL_RENDERER) << "\n"
        << "Version: " << glGetString(GL_VERSION) << std::endl;

    int profile;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);

    GLint max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);

    GLint max_textures;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_textures);

    std::cout << "Context Profile: "
        << (profile == SDL_GL_CONTEXT_PROFILE_CORE ? "Core" : "Compatibility")
        << "Max 2d texture array layers: " << max_layers << std::endl
        << "Max texture units: " << max_textures << std::endl;


    camera.changeAspectRatio(float(render::screenWidth) / render::screenHeight);
}

bool Application::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        camera.HandleEvent(event);
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) return false;
            if (event.key.key == SDLK_F5) debugRenderer->switchEnabled();
        }
        else if (event.type == SDL_EVENT_QUIT) return false;
        else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            render::screenWidth = event.window.data1;
            render::screenHeight = event.window.data2;
            glViewport(0, 0, render::screenWidth, render::screenHeight);
            camera.changeAspectRatio(float(render::screenWidth) / float(render::screenHeight));
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            SDL_WarpMouseInWindow(Window, float(render::screenWidth) / 2, float(render::screenHeight) / 2);
        }
    }
    return true;
}


void Application::Render() {
    // Draw
    worldRenderer.render(world, camera);
}

Application::~Application() {
    delete debugRenderer;
    SDL_GL_DestroyContext(GLContext);
    SDL_DestroyWindow(Window);
    SDL_Quit();
}
