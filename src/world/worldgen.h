#pragma once

#include "core/config.h"
#include "world/chunk.h"
#include "world/noise.h"

namespace ih::world {

// Config-driven terrain: multi-octave Perlin heightmap + block layers.
// Pure function of (coord, config); no world state.
class WorldGenerator {
public:
    WorldGenerator (const core::WorldgenConfig& config, const core::Config& blocks);

    void generate (ChunkCoord coord, ChunkData& out) const;

private:
    const core::WorldgenConfig& m_config;
    PerlinNoise<2> m_noise;
    std::vector<uint16_t> m_layerBlocks{};
};

}  // namespace ih::world
