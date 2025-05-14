# IndustrialHard project
- [x] Move to mappedbufferpool in world rendering
- [x] Draw debug info (also redo debugDrawer to screenSpace coords `[-1; 1]`)
- [ ] Chunks data structure redo (save meshes by sides, etc)
- [ ] Smart mesh updating algorithm (only affected sides, also neighbours chunk matters)
- [ ] Add blockstates
- [ ] Block model loadings
- [ ] Add items
- [ ] Add entities(?) (separate buffer tho 100%)
- [ ] Delete old files, make nice-looking files structure. Document it maybe
- [ ] Mesh update worker


# Rendering
- [ ] pre-save terrain level indices when meshing to then start rendering from them or to them. save mesh ordered from bottom to top
- [x] Binary voxels
- [ ] by-face rendering
- [ ] Make a lot debug info nice looking and possible to easy add and delete.
- [ ] Add font rendering (by pixel font, not ttf you silly)
- [ ] Item renderer

# Game

# Documentation
- Document game concept, game stages, main gameplay ideas
- Document game realisation for open source
