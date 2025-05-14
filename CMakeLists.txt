cmake_minimum_required(VERSION 3.15)
project(IndustrialHard)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-static")
IF(WIN32)
    set(SDL3_DIR "C:\\Users\\frama\\.vcpkg-clion\\vcpkg\\packages\\sdl3_x64-mingw-dynamic\\share\\sdl3")
    set(GLAD_DIR "C:\\Users\\frama\\.vcpkg-clion\\vcpkg\\packages\\glad_x64-mingw-dynamic\\share\\glad")
    set(glm_DIR "C:\\Users\\frama\\.vcpkg-clion\\vcpkg\\packages\\glm_x64-mingw-dynamic\\share\\glm")
ENDIF()
# Find SDL3 and OpenGL
find_package(OpenGL REQUIRED)
find_package(GLAD CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

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
        src/game/data_loaders/TextureManager.h
        src/game/world/World.h
        src/render/Plane.h
        src/render/renderers/block/FaceInstance.h
        src/game/data_loaders/globals.h
        src/game/world/EFacing.h
        src/render/renderers/block/CubeModel.h
        src/render/renderers/block/Vertex.h
        src/render/utils.h
        src/game/data_loaders/JsonStream.cpp
        src/game/data_loaders/JsonStream.h
        src/render/Frustum.h
        src/render/globals.h
        src/render/buffers/MappedBufferPool.h
        src/render/buffers/GPUBuffer.h
        src/render/renderers/world/WorldRenderer.cpp
        src/render/renderers/world/WorldRenderer.h
        src/render/renderers/world/ChunkMesher.cpp
        src/render/renderers/world/ChunkMesher.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 OpenGL::GL glad::glad glm::glm)