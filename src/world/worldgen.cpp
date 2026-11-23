#include "world/worldgen.h"

namespace ih::world {

WorldGenerator::WorldGenerator (const core::WorldgenConfig& config, const core::Config& blocks)
    : m_config (config), m_noise (config.seed) {
    for (const auto& layer : config.layers) {
        m_layerBlocks.push_back (blocks.blockId (layer.block));
    }
}

void WorldGenerator::generate (ChunkCoord coord, ChunkData& out) const {
    const auto& dims = out.dims ();
    const int worldX0 = coord.x * dims.width;
    const int worldZ0 = coord.z * dims.depth;

    for (int z = 0; z < dims.depth; ++z) {
        for (int x = 0; x < dims.width; ++x) {
            const float noiseValue = m_noise.noise ((worldX0 + x) * m_config.scale, 0.0f,
                                                    (worldZ0 + z) * m_config.scale, m_config.octaves,
                                                    m_config.persistence, m_config.lacunarity);
            const int surface = static_cast<int> (m_config.baseHeight + noiseValue * m_config.amplitude);

            // Walk the layer stack top-down; depth -1 fills everything below.
            int top = surface;
            size_t layerIndex = 0;
            for (int y = 0; y < dims.height; ++y) {
                if (y > surface) continue;  // air above terrain
                while (layerIndex + 1 < m_layerBlocks.size () && m_config.layers[layerIndex].depth >= 0
                       && y <= top - m_config.layers[layerIndex].depth) {
                    top -= m_config.layers[layerIndex].depth;
                    ++layerIndex;
                }
                out.set (x, y, z, m_layerBlocks[layerIndex]);
            }
        }
    }
}

}  // namespace ih::world
