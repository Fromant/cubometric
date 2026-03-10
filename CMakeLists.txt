cmake_minimum_required(VERSION 3.15)
project(IndustrialHard)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-static -O3")
#set(CMAKE_CXX_FLAGS "-static -g -fno-omit-frame-pointer -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-static -g -fno-omit-frame-pointer -O0 -DDEBUG")

# Find all the libs
find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(
        glm
        URL https://github.com/g-truc/glm/releases/download/1.0.3/glm-1.0.3.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(glm)

add_subdirectory(3rdparty/glad)


FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(json)

# install SDL3
FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(SDL3)

# Build executable
add_executable(${PROJECT_NAME} src/main.cpp
        src/Application.cpp
        src/Application.h
        src/render/Camera.cpp
        src/render/utils/Shader.cpp

        src/render/renderers/debug/DebugRenderer.cpp
        src/render/renderers/debug/DebugRenderer.h
        src/render/renderers/debug/GraphRenderer.cpp
        src/render/renderers/debug/GraphRenderer.h

        3rdparty/compile.cpp
        src/game/world/BlockType.h
        src/game/world/Chunk.h
        src/game/world/Chunk.cpp
        src/game/data_loaders/TextureManager.cpp
        src/game/data_loaders/JsonLoader.cpp
        src/render/renderers/world/WorldRenderer.cpp
        src/render/renderers/world/ChunkMesher.cpp
        src/render/renderers/world/SkyRenderer.cpp

        src/game/world/ChunkData.cpp
        src/game/world/ChunkData.hpp
        src/render/utils/AABB.hpp

        src/game/world/worldgen/WorldGenerator.cpp
        src/game/world/worldgen/WorldGenerator.hpp
        src/game/world/worldgen/noise/PerlinNoise.cpp
        src/game/world/worldgen/noise/PerlinNoise.hpp
        src/game/world/worldgen/noise/Interpolator.hpp
        src/render/renderers/world/QuadRenderer.cpp
        src/render/renderers/world/QuadRenderer.hpp
        src/render/renderers/RenderThread.cpp
        src/render/renderers/RenderThread.hpp
        src/render/buffers/ThreadSafeMappedBufferPool.hpp
        src/render/buffers/ChunkMeshQueue.hpp
)

add_custom_target(copy-runtime-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
        DEPENDS ${MY_TARGET})


add_dependencies(IndustrialHard copy-runtime-files)

target_include_directories(${PROJECT_NAME} PRIVATE "src" "3rdparty")

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 OpenGL::GL glad glm::glm)