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
    constexpr bool CAP_FRAME_RATE = false;

    GLuint queryID;
    glGenQueries(1, &queryID);

    Uint64 frameCount = 0;

    Uint64 lastFrame = SDL_GetTicks();
    constexpr int frame_avg_count = 180;
    std::vector<Uint64> frametimes(frame_avg_count);
    while (true) {
        Uint64 currentFrame = SDL_GetTicks();
        //cpu frame time
        Uint64 deltaTime = currentFrame - lastFrame;


        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Cap frame rate
        if constexpr (CAP_FRAME_RATE && deltaTime < TARGET_FRAME_TIME) {
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
        frametimes.emplace_back(deltaTime);
        frameCount++;
        if (frameCount % frame_avg_count == 0) {
            Uint64 sum = 0;
            for (auto frameTime : frametimes) sum += frameTime;
            std::cout << "Avg FPS: " << 1000.0 * frame_avg_count / sum << "; Min: " <<
                1000.0f / *std::ranges::max_element(frametimes) << std::endl;
            frametimes.clear();
        }
    }
}

void Application::Update(float deltaTime) {
    camera.Update(deltaTime);
}

// Define the callback
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar* message, const void* userParam) {
    std::cerr << "OpenGL Debug Message: " << message << std::endl;
}


void Application::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // OpenGL Context Settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
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
    SDL_SetWindowRelativeMouseMode(Window, captureMouse);

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

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);

    // Enable depth testing and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    debugRenderer = new debug::DebugRenderer();
    textureManager.Init("assets/textures/");
    worldRenderer.init();


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

    camera.changeAspectRatio(float(render::screenWidth) / render::screenHeight);
}

bool Application::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) return false;
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            render::screenWidth = event.window.data1;
            render::screenHeight = event.window.data2;
            glViewport(0, 0, render::screenWidth, render::screenHeight);
            camera.changeAspectRatio(float(render::screenWidth) / float(render::screenHeight));
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                captureMouse = true;
                SDL_SetWindowRelativeMouseMode(Window, captureMouse);
            }
        }
        if (!captureMouse) continue;
        camera.HandleEvent(event, captureMouse);
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) return false;
            if (event.key.key == SDLK_F5) debugRenderer->switchEnabled();
            if (event.key.key == SDLK_F4) worldRenderer.switchWireframeRendering();
            if (event.key.key == SDLK_B) {
                captureMouse = !captureMouse;
                SDL_SetWindowRelativeMouseMode(Window, captureMouse);
            }
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
