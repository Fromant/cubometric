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
- Make sure dependencies are installed (sdl3, opengl, glad, glm) (I recommend using `vcpkg install glm` and etc)
- Build with cmake
- Before running, make sure SDL3.dll is located inside build folder (Important!)
- ??? Profit
- If something off, try running application inside build folder

# Dependencies:
- OpenGL 4.4+
- SDL 3.2.4
- glad
- glm

# Texure units:
- 0 - main 16x16x2048 2d texture array
- 1 - 64x64 sky atlas


