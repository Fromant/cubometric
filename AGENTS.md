# AGENTS.md

"IndustrialHard": voxel-game engine parody of Minecraft + Factorio. Single C++20 executable — **Vulkan 1.2** (via the Vulkan SDK's `vk::raii`), SDL3, GLM, nlohmann-json. Windows-only build (`.exe` + `SDL3.dll`). No tests, no CI, no lint.

## Build & run

- All deps arrive via CMake `FetchContent` (glm 1.0.3, nlohmann-json 3.12.0, SDL3 3.4.2). Vulkan SDK is the only system-installed dep — `find_package(Vulkan)` requires glslangValidator (part of the SDK).
- Shaders are compiled GLSL → SPIR-V at build time (`shaders/world/*.glsl` → `build/shaders/world.*.spv`).
- w64devkit toolchain: the kit's c++.exe is a stub that can't locate cc1plus; `CMakeLists.txt` selects the kit's `g++.exe`/`gcc.exe` by name. Installed under Strawberry or the kit path.
- Build with CMake + Ninja into `build/` (exe at `build/IndustrialHard.exe`). `CMAKE_CXX_FLAGS` = `-static -O3` (Debug: `-static -g -O0 -DDEBUG`).
- `copy-runtime-files` (built with ALL) copies `assets/` into the build dir automatically; SDL3.dll is staged next to the exe at build time.
- New `.cpp` files must be added to the `add_executable(...)` list in `CMakeLists.txt` — sources are listed explicitly, no globbing.

## Architecture

- Entrypoint: `src/main.cpp` → `ih::app::Application` (`src/app/application.cpp`) → `run()`.
- **Event thread** (`Application`): window, input, camera. Publishes `FrameParams` (view/projection, frustum, camera pos, render distance, wireframe flag) into `core::FrameParamsQueue`; never touches Vulkan or world data.
- **Render thread** (`ih::render::RenderThread`): owns every Vulkan object. Loop: ingest mesh queues → update desired chunk set (`ih::world::World`) → begin frame → upload meshes to the shared `Arena` via the `UploadRing` staging ring → draw → present.
- **World pipeline** (`src/world/`): per-chunk lifecycle (Empty → Generating → Generated → Meshing → Ready/Dead). `World` spawns worker threads that generate chunks (Perlin heightmap + layered blocks from `assets/config/worldgen.json`) and run the greedy mesher (`src/world/mesher.cpp`) over them. Render thread drains finished meshes with a 64-mesh/frame budget.
- **Rendering**: one 2D 2D-array texture (`TextureArray`, block tiles with CPU-built mip chain), one arena buffer for all chunk meshes (first-fit free list), one vertex-buffer walk per frame. One draw call per visible chunk part, pushed through `WorldRenderer`; UVs derived from position in-shader so greedy-merged quads tile correctly.
- Camera, render distance, chunk/worldgen parameters live in `assets/config/render.json` / `worldgen.json`; blocks are defined in `assets/config/blocks/*.json` (texture array layer resolved at startup).

## Status

- Vulkan migration: M1 done — window/swapchain/validation, block pipeline (worldgen → mesh → arena upload → draw), GPU upload ring, wireframe.
- WIP/open: sky rendering (none — no renderer, no shader, no atlas), item/entity pipeline (none), physics (none), items (none).
- Manual verification keys: LMB/B = capture mouse, F4 = wireframe, ESC = quit.
