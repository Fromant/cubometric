# IndustrialHard project

- [x] Move to mappedbufferpool in world rendering
- [x] Draw debug info (also redo debugDrawer to screenSpace coords `[-1; 1]`)
- [ ] Multithreading (draw thread, worldgen thread, mesher thread)
- 
- [ ] Block model loadings
- [ ] Add items
- [ ] Add entities(?) (separate buffer tho 100%)
- [ ] Delete old files, make nice-looking files structure. Document it maybe
- [ ] Mesh update worker

# Rendering

- [x] Binary voxels
- [x] by-face rendering
- [x] fix invalid value error
- [x] merge chunk facing buffers into one bigger buffer
- [x] debug Wireframe rendering
- [x] Move to 32x32x32 chunks
- [x] better world structuring
- [x] switch texturing to texture arrays instead of textures
- [ ] Switch to vulkan????
- [ ] Rewrite memory allocator to save all buffers in the same buffer. Group them somehow to reduce draw call count.
- [ ] Make a lot debug info nice looking and possible to easy add and delete.
- [ ] Add lightning
- [ ] Add font rendering (by pixel font, not ttf you silly)
- [ ] Item renderer

# Debug

- [x] Add fps counter in console

# Worldgen

- [x] Simple cube
- [x] y=sin(x+z)
- [ ] Perlin noise

# Game

# Documentation

- Document game concept, game stages, main gameplay ideas
- Document game realisation for open source
