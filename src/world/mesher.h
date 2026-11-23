#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "world/chunk.h"

namespace ih::world {

// Packed vertex, 8 bytes (uint64):
//   bits  0-5  : x (chunk-local, 0..63)
//   bits  6-14 : y (chunk-local, 0..511)
//   bits 15-20 : z (chunk-local, 0..63)
//   bits 21-23 : face (0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z)
//   bits 24-31 : light (0..255; 255 = unoccluded, reserved for baked AO)
//   bits 32-42 : texture array layer
// UVs are not stored: the shader derives them from position + face, which
// tiles greedy-merged quads correctly for free.
inline uint64_t packVertex (int x, int y, int z, int face, int light, int layer) {
    return (static_cast<uint64_t> (x) & 0x3F)
         | ((static_cast<uint64_t> (y) & 0x1FF) << 6)
         | ((static_cast<uint64_t> (z) & 0x3F) << 15)
         | ((static_cast<uint64_t> (face) & 0x7) << 21)
         | ((static_cast<uint64_t> (light) & 0xFF) << 24)
         | ((static_cast<uint64_t> (layer) & 0x7FF) << 32);
}

// Horizontal neighbors for boundary culling, order +X, -X, +Z, -Z.
// nullptr = not loaded = treated as air (face is emitted).
struct NeighborChunks {
    std::array<const ChunkData*, 4> sides{};
};

// Resolved texture-array layer per block face (block id -> 6 layers).
using FaceLayers = std::vector<std::array<uint16_t, 6>>;

// Greedy meshing over the whole chunk. Pure CPU function: no GPU, no world
// access beyond the given data. Returns one or more parts; each part stays
// under the 16-bit index limit (65535 vertices).
std::vector<MeshData> meshChunk (const ChunkData& chunk, const NeighborChunks& neighbors,
                                 const FaceLayers& faceLayers);

}  // namespace ih::world
