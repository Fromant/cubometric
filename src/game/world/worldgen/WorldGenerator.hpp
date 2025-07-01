#pragma once
#include "game/world/Chunk.h"
#include "noise/PerlinNoise.hpp"

class WorldGenerator {
public:
    explicit WorldGenerator(unsigned seed = 0) : terrainNoise(seed) {}

    Chunk& generateChunk(int x, int z, std::unordered_map<size_t, Chunk>& chunks) const {
        size_t id = Chunk::getId(x, z);

        Chunk& generated = chunks.emplace(id, glm::ivec2{x, z}).first->second;
        auto& blocks = generated.getBlocks();

        const int worldX0 = x * Chunk::WIDTH;
        const int worldZ0 = z * Chunk::DEPTH;

        // Terrain parameters
        constexpr float SCALE = 0.01f;
        constexpr int BASE_HEIGHT = 64;
        constexpr int AMPLITUDE = 48;

        for (int z1 = 0; z1 < Chunk::DEPTH; z1++) {
            for (int x1 = 0; x1 < Chunk::WIDTH; x1++) {
                const int worldX = worldX0 + x1;
                const int worldZ = worldZ0 + z1;

                // Get terrain height with multi-octave noise
                float noiseVal = terrainNoise.noise(
                    worldX * SCALE,
                    0.0f, // Y-coordinate not used for heightmap
                    worldZ * SCALE,
                    6, // Octaves
                    0.5f, // Persistence
                    2.0f // Lacunarity
                );

                // Scale to world height
                int surfaceY = BASE_HEIGHT + static_cast<int>(noiseVal * AMPLITUDE);

                // Generate vertical column
                for (int worldY = 0; worldY < Chunk::HEIGHT; worldY++) {
                    if (worldY > surfaceY) {
                        // blocks[index] = BlockType::AIR;
                    }
                    else if (worldY == surfaceY) {
                        blocks.changeBlock({x1, worldY, z1}, BlockType::GRASS);
                    }
                    else if (worldY > surfaceY - 4) {
                        blocks.changeBlock({x1, worldY, z1}, BlockType::DIRT);
                    }
                    else {
                        blocks.changeBlock({x1, worldY, z1}, BlockType::STONE);
                    }
                }
            }
        }

        return generated;
    }

private:
    PerlinNoise<2> terrainNoise;
};
