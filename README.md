# IndustrialHard = Minecraft and Factorio parody

IndustrialHard would be about: 
- Building massive buildings, exploring and survive wild realistic world
- Building your factory to Flying out to moon to accomplish your child dream (end goal for now)

Right now it's unplayable stone simulator written in CPP+OpenGL+GLAD+SDL+glm+...

# Controls:
- WASD movement
- F5 to open frame time graph
- F4 to enable wireframe rendering
- B to switch mouse capturing
- LMB to capture mouse
- C for zoom
- '+' and '-' - control chunk render distance

# Compile & run:
- Make sure vcpkg is installed and a enviroment variables are set: `VCPKG_ROOT` (check via `$ENV:VCPKG_ROOT`). 
- Default vcpkg triplets can be set in enviroment variables (`VCPKG_DEFAULT_TRIPLET` and `VCPKG_DEFAULT_HOST_TRIPLET`)
- If needed, change used triplet in CMakePresets.json (`VCPKG_TARGET_TRIPLET`) 
- vcpkg cmake toolchain will be used automaticallly
- build with vscode Cmake tools (using CMakePresets)
- Before running, copy SDL3.dll from build/type/_deps/sdl3-build or download it (same version as SDL3 in deps)
- Run application inside build folder

# Dependencies:
- OpenGL 4.4+
- SDL 3.2.16
- glad
- glm
- nlohmann-json 3.12.0
- 

# Texure units:
- 0 - main 16x16x2048 2d texture array
- 1 - 64x64 sky atlas


