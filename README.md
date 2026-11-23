# IndustrialHard = Minecraft and Factorio parody

IndustrialHard would be about:
- Building massive buildings, exploring and surviving a realistic world
- Building your factory to fly out to the moon to accomplish your child dream (end goal for now)

Right now it's an unplayable stone simulator written in C++ (Vulkan).

# Controls:
- WASD + mouse movement
- F4 to enable wireframe rendering
- LMB / B to capture mouse
- C / +/- (future) to zoom and control chunk render distance
- ESC to quit

# Compile & run:
1. Install the Vulkan SDK (includes glslangValidator) and a w64devkit/Strawberry C++ toolchain.
2. CMake (≥3.21) + Ninja:
   `cmake -B build -G Ninja && cmake --build build`
   (or use the CMake tools in VS Code against `build/`)
3. Run `build/IndustrialHard.exe` **from inside the build folder** (relative `assets/` and `shaders/` paths).

Dependencies (all auto-fetched except Vulkan):
- Vulkan SDK (system) + glslangValidator
- nlohmann-json 3.12.0
- SDL3 3.4.2
- glm 1.0.3

# Config (game data):
- `assets/config/render.json` — chunk size, render distance, texture tile size, mip levels, sun/ambient, camera
- `assets/config/worldgen.json` — Perlin-based terrain (seed, scale, base height, amplitude, layers)
- `assets/config/blocks/*.json` — block definitions (texture array layers)
- `assets/textures/blocks/*.png` — 16x16 block texture tiles
