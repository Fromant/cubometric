#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "core/config.h"

namespace ih::world {

// Chunk coordinate in chunk units (not blocks).
struct ChunkCoord {
    int x = 0;
    int z = 0;

    bool operator== (const ChunkCoord& other) const = default;
};

struct ChunkCoordHash {
    size_t operator() (const ChunkCoord& c) const noexcept {
        // Szudzik pairing on offset coordinates: cheap and collision-free.
        const uint32_t a = static_cast<uint32_t> (c.x) + 0x80000000u;
        const uint32_t b = static_cast<uint32_t> (c.z) + 0x80000000u;
        return static_cast<size_t> (a) * 0x9E3779B97F4A7C15ull ^ static_cast<size_t> (b);
    }
};

// Plain block storage. Dimensions come from config; ids index the block
// registry (0 = air). No rendering knowledge lives here.
class ChunkData {
public:
    ChunkData () = default;
    explicit ChunkData (const core::ChunkDims& dims)
        : m_dims (dims), m_blocks (static_cast<size_t> (dims.width) * dims.height * dims.depth, 0) {}

    const core::ChunkDims& dims () const { return m_dims; }

    size_t index (int x, int y, int z) const {
        return static_cast<size_t> (x) + static_cast<size_t> (y) * m_dims.width
             + static_cast<size_t> (z) * m_dims.width * m_dims.height;
    }

    uint16_t get (int x, int y, int z) const { return m_blocks[index (x, y, z)]; }
    void set (int x, int y, int z, uint16_t block) { m_blocks[index (x, y, z)] = block; }

    bool inBounds (int x, int y, int z) const {
        return x >= 0 && x < m_dims.width && y >= 0 && y < m_dims.height && z >= 0 && z < m_dims.depth;
    }

    const std::vector<uint16_t>& blocks () const { return m_blocks; }

private:
    core::ChunkDims m_dims{};
    std::vector<uint16_t> m_blocks{};
};

// CPU-side mesh output. Vertices are packed uint64 (see mesher.h for layout),
// indices are 16-bit. Move-only: ownership passes worker -> render thread.
struct MeshData {
    std::vector<uint64_t> vertices{};
    std::vector<uint16_t> indices{};

    bool empty () const { return indices.empty (); }
    void clear () {
        vertices.clear ();
        indices.clear ();
    }
};

}  // namespace ih::world
