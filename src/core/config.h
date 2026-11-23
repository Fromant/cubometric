#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace ih::core {

// Everything game-tunable lives in JSON under assets/config; the C++ core
// contains no game constants. Loaded once at startup, then read-only.

struct ChunkDims {
    int width = 32;
    int height = 256;
    int depth = 32;
};

struct CameraConfig {
    float fovDegrees = 70.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float moveSpeed = 20.0f;
};

struct RenderConfig {
    ChunkDims chunk{};
    int renderDistance = 8;  // chunks
    int tileSize = 16;       // block texture tile size in pixels
    int mipLevels = 4;
    int workerThreads = 0;   // 0 = auto
    glm::vec3 sunDirection{0.5f, 0.8f, 0.3f};
    glm::vec3 groundSunDirection{0.5f, 0.8f, 0.3f};
    float groundSunIntensity = 0.5f;
    float ambient = 0.5f;
    CameraConfig camera{};
};

struct WorldgenLayer {
    std::string block;
    int depth = 0;  // blocks thick; -1 = everything below
};

struct WorldgenConfig {
    unsigned seed = 1337;
    float scale = 0.01f;
    float baseHeight = 64.0f;
    float amplitude = 48.0f;
    int octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    std::vector<WorldgenLayer> layers{};
};

// Face order: +X, -X, +Y, -Y, +Z, -Z.
struct BlockDef {
    uint16_t id = 0;
    std::string name{};
    bool solid = true;
    std::array<std::string, 6> faceTextures{};
};

class Config {
public:
    // assetsDir contains config/ and textures/.
    static Config load (const std::string& assetsDir);

    uint16_t blockId (const std::string& name) const;
    const BlockDef& block (uint16_t id) const { return blocks[id]; }

    RenderConfig render{};
    WorldgenConfig worldgen{};
    std::vector<BlockDef> blocks{};  // index == block id; 0 is always air
    std::unordered_map<std::string, uint16_t> blockIds{};
};

}  // namespace ih::core
