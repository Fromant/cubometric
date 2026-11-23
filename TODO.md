# IndustrialHard project

## Vulkan migration (done)
- [x] Window + swapchain with resize, validation layers
- [x] Render thread owns all Vulkan objects; event/render thread split
- [x] Arena-suballocated GPU buffer (one vertex+index buffer for all chunk meshes)
- [x] GPU upload ring (frame-in-flight slots), ingest budget (64 meshes/frame)
- [x] Block texture array (CPU mip chain, sRGB)
- [x] Greedy mesher (whole chunk, ≤16-bit index limit)
- [x] Perlin noise worldgen + layered block config (`assets/config/worldgen.json`)
- [x] Config-driven rendering (chunks, render distance, sun, camera, textures)
- [x] Wireframe toggle (F4)

## Next / open
- [ ] Sky rendering (renderer, shader, atlas — currently none)
- [ ] Shadow map (or another light source)
- [ ] Baked light into vertices + ambient occlusion
- [ ] Sky-lit/sun-lit ground alignment (sync sun and ground sun — done for render dir, needs lighting)
- [ ] Mesh update worker (ingest currently happens on the render thread — defers GPU upload)
- [ ] Frame-time debug graph (F5)
- [ ] Items / entities (separate pipeline; none in tree yet)
- [ ] Physics engine + world updates (none in tree yet)
- [ ] Item renderer (none in tree yet)
- [ ] Entity renderer (none in tree yet)

## World
- [ ] Mesh update worker (see above)
- [ ] Chunk border rendering
- [ ] LODs

## Docs
- [ ] Document game concept, game stages, main gameplay ideas
- [ ] Document game realisation for open source
