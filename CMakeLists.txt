cmake_minimum_required(VERSION 3.21)

# --- Toolchain: w64devkit (MinGW-w64) ---
# The kit's c++.exe can be a broken stub that cannot locate cc1plus; the real
# drivers are g++/gcc in the same directory. Select them explicitly, discovered
# through PATH (no hardcoded install paths). Strawberry Perl's older GCC may
# sit earlier on PATH, so match the kit directory by name.
if (WIN32)
    set (IH_KIT_DIR "")
    if (DEFINED CMAKE_CXX_COMPILER)
        get_filename_component (IH_KIT_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    else ()
        set (IH_PATH "$ENV{PATH}")
        foreach (dir IN LISTS IH_PATH)
            if (dir MATCHES "w64devkit")
                set (IH_KIT_DIR "${dir}")
                break ()
            endif ()
        endforeach ()
    endif ()
    if (IH_KIT_DIR AND EXISTS "${IH_KIT_DIR}/g++.exe")
        set (CMAKE_CXX_COMPILER "${IH_KIT_DIR}/g++.exe" CACHE FILEPATH "C++ compiler" FORCE)
        set (CMAKE_C_COMPILER "${IH_KIT_DIR}/gcc.exe" CACHE FILEPATH "C compiler" FORCE)
        # Binutils too: PATH may resolve ld/ar to another (older) toolchain.
        if (EXISTS "${IH_KIT_DIR}/ld.exe")
            set (CMAKE_LINKER "${IH_KIT_DIR}/ld.exe" CACHE FILEPATH "Linker" FORCE)
        endif ()
        if (EXISTS "${IH_KIT_DIR}/gcc-ar.exe")
            set (CMAKE_AR "${IH_KIT_DIR}/gcc-ar.exe" CACHE FILEPATH "Archiver" FORCE)
            set (CMAKE_RANLIB "${IH_KIT_DIR}/gcc-ranlib.exe" CACHE FILEPATH "Ranlib" FORCE)
        endif ()
    endif ()
endif ()

project(IndustrialHard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-static -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-static -g -fno-omit-frame-pointer -O0 -DDEBUG")

# --- Vulkan SDK (system-installed; the only dependency that is not fetched) ---
find_package(Vulkan REQUIRED COMPONENTS glslangValidator)

# --- Fetched dependencies ---
include(FetchContent)

FetchContent_Declare(glm
        URL https://github.com/g-truc/glm/releases/download/1.0.3/glm-1.0.3.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(json)

FetchContent_Declare(SDL3
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(SDL3)

# C++ bindings (vk::raii) ship with the Vulkan SDK itself — no extra fetch needed.

# --- Shaders: GLSL -> SPIR-V at build time, via the SDK glslangValidator ---
set(GLSLANG_VALIDATOR ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
set(SHADER_NAMES world)
set(SHADER_STAGES vert frag)
set(SPIRV_OUTPUTS)
foreach (shader_name IN LISTS SHADER_NAMES)
    foreach (stage IN LISTS SHADER_STAGES)
        set (shader_src ${CMAKE_SOURCE_DIR}/shaders/${shader_name}/${stage}.glsl)
        set (shader_out ${SHADER_OUT_DIR}/${shader_name}.${stage}.spv)
        add_custom_command(
                OUTPUT ${shader_out}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
                COMMAND ${GLSLANG_VALIDATOR} -V -S ${stage} --target-env vulkan1.2 -o ${shader_out} ${shader_src}
                DEPENDS ${shader_src}
                COMMENT "glslang: ${shader_name}.${stage}"
                VERBATIM
        )
        list (APPEND SPIRV_OUTPUTS ${shader_out})
    endforeach ()
endforeach ()
add_custom_target (shaders DEPENDS ${SPIRV_OUTPUTS})

# --- Executable ---
add_executable (${PROJECT_NAME}
        src/main.cpp

        src/core/log.h
        src/core/log.cpp
        src/core/check.h
        src/core/frame_params.h
        src/core/config.h
        src/core/config.cpp
        src/core/frustum.h
        src/core/queue.h

        src/platform/window.h
        src/platform/window.cpp

        src/app/application.h
        src/app/application.cpp
        src/app/camera.h
        src/app/camera.cpp

        src/world/chunk.h
        src/world/noise.h
        src/world/worldgen.h
        src/world/worldgen.cpp
        src/world/mesher.h
        src/world/mesher.cpp
        src/world/world.h
        src/world/world.cpp

        src/render/render_thread.h
        src/render/render_thread.cpp
        src/render/vulkan/vk.h
        src/render/vulkan/context.h
        src/render/vulkan/context.cpp
        src/render/vulkan/swapchain.h
        src/render/vulkan/swapchain.cpp
        src/render/vulkan/buffer.h
        src/render/vulkan/buffer.cpp
        src/render/vulkan/shader.h
        src/render/vulkan/texture_array.h
        src/render/vulkan/texture_array.cpp
        src/render/vulkan/arena.h
        src/render/vulkan/arena.cpp
        src/render/renderers/renderer.h
        src/render/renderers/world_renderer.h
        src/render/renderers/world_renderer.cpp

        3rdparty/compile.cpp
)
add_dependencies (${PROJECT_NAME} shaders)

target_include_directories (${PROJECT_NAME} PRIVATE
        src
        3rdparty
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries (${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        SDL3::SDL3
        glm::glm
        nlohmann_json::nlohmann_json
)

target_compile_definitions (${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)

# Validation layers + debug labels
option (ENABLE_VK_VALIDATION "Enable Vulkan validation layers" ON)
if (ENABLE_VK_VALIDATION)
    target_compile_definitions (${PROJECT_NAME} PRIVATE ENABLE_VK_VALIDATION)
endif ()

# --- Runtime files: game data is data (copied, not compiled) ---
add_custom_target (copy-runtime-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
)
add_dependencies (${PROJECT_NAME} copy-runtime-files)

# Stage SDL3.dll next to the exe so the build dir is directly runnable
add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_BINARY_DIR}/SDL3.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Staging SDL3.dll"
)
