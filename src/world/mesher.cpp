#include "world/mesher.h"

#include <glm/glm.hpp>

namespace ih::world {

namespace {

constexpr uint16_t MAX_VERTICES = 65535;

// Face -> (fixed axis, plane axis u, plane axis v).
constexpr std::array<glm::ivec3, 6> AXES = {{
    {0, 2, 1},  // +X: plane (z, y)
    {0, 2, 1},  // -X
    {1, 0, 2},  // +Y: plane (x, z)
    {1, 0, 2},  // -Y
    {2, 0, 1},  // +Z: plane (x, y)
    {2, 0, 1},  // -Z
}};

// Corner order in (u, v) giving an outward winding for each face.
// Base order (a,b),(a+w,b),(a+w,b+h),(a,b+h) yields -X, -Y, +Z normals;
// the other three faces use the reversed order.
constexpr std::array<bool, 6> REVERSED = {true, false, true, false, false, true};

struct Quad {
    glm::ivec3 pos;  // corner (a, b) mapped to xyz, fixed axis at face plane
    glm::ivec3 size;  // extents along u, v (fixed axis extent 0)
    int face;
    int layer;
};

class MeshBuilder {
public:
    void addQuad (const Quad& quad) {
        if (m_current.vertices.size () + 4 > MAX_VERTICES) flush ();
        const auto base = static_cast<uint16_t> (m_current.vertices.size ());
        const int u = AXES[quad.face].y;
        const int v = AXES[quad.face].z;

        glm::ivec3 corners[4] = {
            quad.pos,
            quad.pos,
            quad.pos,
            quad.pos,
        };
        corners[1][u] += quad.size[u];
        corners[2][u] += quad.size[u];
        corners[2][v] += quad.size[v];
        corners[3][v] += quad.size[v];

        const int order[4] = {0, 1, 2, 3};
        for (int i = 0; i < 4; ++i) {
            const glm::ivec3& c = corners[REVERSED[quad.face] ? order[3 - i] : order[i]];
            m_current.vertices.push_back (packVertex (c.x, c.y, c.z, quad.face, 255, quad.layer));
        }
        m_current.indices.insert (m_current.indices.end (),
                                  {base, static_cast<uint16_t> (base + 1), static_cast<uint16_t> (base + 2),
                                   base, static_cast<uint16_t> (base + 2), static_cast<uint16_t> (base + 3)});
    }

    std::vector<MeshData> finish () {
        flush ();
        return std::move (m_parts);
    }

private:
    void flush () {
        if (!m_current.empty ()) m_parts.push_back (std::move (m_current));
        m_current = MeshData{};
    }

    MeshData m_current{};
    std::vector<MeshData> m_parts{};
};

}  // namespace

std::vector<MeshData> meshChunk (const ChunkData& chunk, const NeighborChunks& neighbors,
                                 const FaceLayers& faceLayers) {
    const auto& dims = chunk.dims ();
    const std::array<int, 3> extent = {dims.width, dims.height, dims.depth};
    MeshBuilder builder;

    for (int face = 0; face < 6; ++face) {
        const int d = AXES[face].x;
        const int u = AXES[face].y;
        const int v = AXES[face].z;
        const int sign = (face % 2 == 0) ? 1 : -1;
        const int du = extent[u];
        const int dv = extent[v];

        std::vector<uint32_t> mask (static_cast<size_t> (du) * dv, 0);

        for (int slice = 0; slice < extent[d]; ++slice) {
            // Build the face mask for this slice.
            for (int b = 0; b < dv; ++b) {
                for (int a = 0; a < du; ++a) {
                    glm::ivec3 pos (0);
                    pos[d] = slice;
                    pos[u] = a;
                    pos[v] = b;

                    const uint16_t block = chunk.get (pos.x, pos.y, pos.z);
                    if (block == 0) continue;

                    glm::ivec3 next = pos;
                    next[d] += sign;
                    uint16_t neighborBlock = 0;
                    if (chunk.inBounds (next.x, next.y, next.z)) {
                        neighborBlock = chunk.get (next.x, next.y, next.z);
                    } else if (d != 1) {
                        // Horizontal boundary: sample the neighbor chunk.
                        const int side = (d == 0) ? (sign > 0 ? 0 : 1) : (sign > 0 ? 2 : 3);
                        const ChunkData* neighbor = neighbors.sides[side];
                        if (neighbor) {
                            glm::ivec3 np = next;
                            np[d] = (sign > 0) ? 0 : extent[d] - 1;
                            neighborBlock = neighbor->get (np.x, np.y, np.z);
                        }
                    }
                    // Vertical out-of-chunk stays air.
                    if (neighborBlock == 0) {
                        mask[static_cast<size_t> (a) + b * du] = faceLayers[block][face] + 1u;
                    }
                }
            }

            // Greedy rectangle merge over the mask.
            for (int b = 0; b < dv; ++b) {
                for (int a = 0; a < du;) {
                    const uint32_t value = mask[static_cast<size_t> (a) + b * du];
                    if (value == 0) {
                        ++a;
                        continue;
                    }
                    int w = 1;
                    while (a + w < du && mask[static_cast<size_t> (a + w) + b * du] == value) ++w;
                    int h = 1;
                    bool rowOk = true;
                    while (b + h < dv && rowOk) {
                        for (int dx = 0; dx < w; ++dx) {
                            if (mask[static_cast<size_t> (a + dx) + (b + h) * du] != value) {
                                rowOk = false;
                                break;
                            }
                        }
                        if (rowOk) ++h;
                    }
                    for (int dy = 0; dy < h; ++dy) {
                        for (int dx = 0; dx < w; ++dx) {
                            mask[static_cast<size_t> (a + dx) + (b + dy) * du] = 0;
                        }
                    }

                    Quad quad{};
                    quad.pos[d] = slice + (sign > 0 ? 1 : 0);
                    quad.pos[u] = a;
                    quad.pos[v] = b;
                    quad.size = glm::ivec3 (0);
                    quad.size[u] = w;
                    quad.size[v] = h;
                    quad.face = face;
                    quad.layer = value;
                    builder.addQuad (quad);

                    a += w;
                }
            }
        }
    }

    return builder.finish ();
}

}  // namespace ih::world
