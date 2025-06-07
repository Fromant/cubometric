cmake_minimum_required(VERSION 3.15)
project(IndustrialHard)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-static -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-static -g -fno-omit-frame-pointer -O0 -DDEBUG")

IF (WIN32)
    set(Vulkan_DIR "D:/Programs/VulkanSDK/")
    set(Vulkan_INCLUDE_DIR "D:/Programs/VulkanSDK/Include")
    set(Vulkan_LIBRARY "D:/Programs/VulkanSDK/Lib/vulkan-1.lib")
    set(SDL3_DIR "C:\\Users\\frama\\.vcpkg-clion\\vcpkg\\packages\\sdl3_x64-mingw-dynamic\\share\\sdl3")
    set(glm_DIR "C:\\Users\\frama\\.vcpkg-clion\\vcpkg\\packages\\glm_x64-mingw-dynamic\\share\\glm")
    set(Vulkan_glslc_EXECUTABLE "D:/Programs/VulkanSDK/Bin/glslc.exe")
ENDIF ()

# Find SDL3 and glm
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS glslc)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Build executable
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Application.cpp
        src/Application.h
        src/render/Camera.cpp
        #
        #        3rdparty/compile.cpp
        #        src/game/world/BlockType.h
        #        src/game/world/Chunk.h
        #        src/game/world/Chunk.cpp
        #
        #        src/game/world/ChunkData.cpp
        #        src/game/world/ChunkData.hpp
        #        src/utils/AABB.hpp
        #
        #        src/game/world/worldgen/WorldGenerator.cpp
        #        src/game/world/worldgen/WorldGenerator.hpp
        #        src/game/world/worldgen/noise/PerlinNoise.cpp
        #        src/game/world/worldgen/noise/PerlinNoise.hpp
        #        src/game/world/worldgen/noise/Interpolator.hpp
        src/utils/VulkanWindow.cpp
        src/utils/VulkanWindow.hpp
        src/utils/Shader.cpp
        src/utils/Shader.hpp
)

add_custom_target(copy-runtime-files ALL
#        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
        DEPENDS ${MY_TARGET})

add_dependencies(IndustrialHard copy-runtime-files)

if (NOT Vulkan_glslc_FOUND)
    message(WARNING "glslc not found - shaders won't be compiled")
else ()
    # Set shader source and output directories
    set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
    set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/compiled_shaders")

    # Create output directory
    file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

    # Get all shader files
    file(GLOB_RECURSE SHADER_SOURCES "${SHADER_SRC_DIR}/*.vert" "${SHADER_SRC_DIR}/*.frag")

    # Create compilation command for each shader
    foreach (shader ${SHADER_SOURCES})
        # Get relative path
        file(RELATIVE_PATH rel_path ${SHADER_SRC_DIR} ${shader})

        # Set output path
        set(out_path "${SHADER_OUT_DIR}/${rel_path}.spv")

        # Add compilation command
        add_custom_command(
                OUTPUT ${out_path}
                COMMAND ${Vulkan_glslc_EXECUTABLE} -o ${out_path} ${shader}
                DEPENDS ${shader}
                COMMENT "Compiling GLSL shader: ${rel_path}"
                VERBATIM
        )

        list(APPEND SHADER_BINARIES ${out_path})
    endforeach ()

    # Create target for shader compilation
    add_custom_target(CompileShaders ALL DEPENDS ${SHADER_BINARIES})

    # Add dependency to your main target
    add_dependencies(${PROJECT_NAME} CompileShaders)

    # Copy shaders to build directory
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SHADER_OUT_DIR} $<TARGET_FILE_DIR:IndustrialHard>/shaders
    )
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE "src" "3rdparty" ${Vulkan_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 glm::glm Vulkan::Vulkan)