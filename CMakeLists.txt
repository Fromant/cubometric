cmake_minimum_required(VERSION 3.15)
project(IndustrialHard)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-static -O3")
#set(CMAKE_CXX_FLAGS "-static -g -fno-omit-frame-pointer -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-static -g -fno-omit-frame-pointer -O0")

# Find all the libs
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 CONFIG REQUIRED)

# install SDL3
include(FetchContent)
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG "release-3.2.16"
        EXCLUDE_FROM_ALL
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
        src/utils/AABB.hpp

        src/game/world/worldgen/WorldGenerator.cpp
        src/game/world/worldgen/WorldGenerator.hpp
        src/game/world/worldgen/noise/PerlinNoise.cpp
        src/game/world/worldgen/noise/PerlinNoise.hpp
        src/game/world/worldgen/noise/Interpolator.hpp
        src/render/renderers/world/QuadRenderer.cpp
        src/render/renderers/world/QuadRenderer.hpp
)

add_custom_target(copy-runtime-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
        DEPENDS ${MY_TARGET})


add_dependencies(IndustrialHard copy-runtime-files)

target_include_directories(${PROJECT_NAME} PRIVATE "src" "3rdparty")

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 OpenGL::GL glad::glad glm::glm)