#include "core/config.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/check.h"
#include "core/log.h"

namespace ih::core {

namespace {

using json = nlohmann::json;

json readJson (const std::filesystem::path& path) {
    std::ifstream file (path);
    IH_CHECK (file.is_open ());
    return json::parse (file);
}

glm::vec3 readVec3 (const json& j, const glm::vec3& fallback) {
    if (!j.is_array () || j.size () != 3) return fallback;
    return {j[0].get<float> (), j[1].get<float> (), j[2].get<float> ()};
}

void loadRender (const json& j, RenderConfig& out) {
    if (j.contains ("chunk")) {
        const auto& c = j["chunk"];
        out.chunk.width = c.value ("width", out.chunk.width);
        out.chunk.height = c.value ("height", out.chunk.height);
        out.chunk.depth = c.value ("depth", out.chunk.depth);
    }
    out.renderDistance = j.value ("renderDistance", out.renderDistance);
    out.tileSize = j.value ("textureTileSize", out.tileSize);
    out.mipLevels = j.value ("mipLevels", out.mipLevels);
    out.workerThreads = j.value ("workerThreads", out.workerThreads);
    out.sunDirection = glm::normalize (readVec3 (j.value ("sunDirection", json::array ()), out.sunDirection));
    out.ambient = j.value ("ambient", out.ambient);
    if (j.contains ("camera")) {
        const auto& c = j["camera"];
        out.camera.fovDegrees = c.value ("fov", out.camera.fovDegrees);
        out.camera.nearPlane = c.value ("near", out.camera.nearPlane);
        out.camera.farPlane = c.value ("far", out.camera.farPlane);
        out.camera.moveSpeed = c.value ("speed", out.camera.moveSpeed);
    }
}

void loadWorldgen (const json& j, WorldgenConfig& out) {
    out.seed = j.value ("seed", out.seed);
    out.scale = j.value ("scale", out.scale);
    out.baseHeight = j.value ("baseHeight", out.baseHeight);
    out.amplitude = j.value ("amplitude", out.amplitude);
    out.octaves = j.value ("octaves", out.octaves);
    out.persistence = j.value ("persistence", out.persistence);
    out.lacunarity = j.value ("lacunarity", out.lacunarity);
    if (j.contains ("layers")) {
        for (const auto& layer : j["layers"]) {
            out.layers.push_back ({layer.at ("block").get<std::string> (), layer.value ("depth", -1)});
        }
    }
}

void loadBlock (const json& j, BlockDef& out) {
    out.name = j.at ("name").get<std::string> ();
    out.solid = j.value ("solid", true);
    const auto& textures = j.at ("textures");
    std::array<std::string, 6> faces{};
    if (textures.contains ("all")) {
        faces.fill (textures["all"].get<std::string> ());
    } else {
        const std::string side = textures.value ("side", textures.value ("all", out.name));
        faces[0] = faces[1] = faces[4] = faces[5] = side;
        faces[2] = textures.value ("top", side);
        faces[3] = textures.value ("bottom", side);
    }
    // Explicit per-face overrides: "+x", "-x", "+y", "-y", "+z", "-z".
    static constexpr std::array<const char*, 6> keys = {"+x", "-x", "+y", "-y", "+z", "-z"};
    for (size_t i = 0; i < keys.size (); ++i) {
        if (textures.contains (keys[i])) faces[i] = textures[keys[i]].get<std::string> ();
    }
    out.faceTextures = faces;
}

}  // namespace

Config Config::load (const std::string& assetsDir) {
    Config config{};
    const std::filesystem::path root (assetsDir);

    loadRender (readJson (root / "config" / "render.json"), config.render);
    loadWorldgen (readJson (root / "config" / "worldgen.json"), config.worldgen);

    // Air is always id 0.
    BlockDef air{};
    air.id = 0;
    air.name = "air";
    air.solid = false;
    config.blocks.push_back (air);
    config.blockIds["air"] = 0;

    std::vector<std::filesystem::path> blockFiles;
    for (const auto& entry : std::filesystem::recursive_directory_iterator (root / "config" / "blocks")) {
        if (entry.is_regular_file () && entry.path ().extension () == ".json") blockFiles.push_back (entry.path ());
    }
    std::sort (blockFiles.begin (), blockFiles.end ());  // deterministic ids
    for (const auto& path : blockFiles) {
        BlockDef def{};
        loadBlock (readJson (path), def);
        IH_CHECK (!config.blockIds.contains (def.name));
        def.id = static_cast<uint16_t> (config.blocks.size ());
        config.blockIds[def.name] = def.id;
        config.blocks.push_back (def);
    }

    // Worldgen layers must reference known blocks.
    for (const auto& layer : config.worldgen.layers) {
        IH_CHECK (config.blockIds.contains (layer.block));
    }

    core::logInfo ("Config: " + std::to_string (config.blocks.size () - 1) + " blocks, render distance "
                   + std::to_string (config.render.renderDistance));
    return config;
}

uint16_t Config::blockId (const std::string& name) const {
    const auto it = blockIds.find (name);
    IH_CHECK (it != blockIds.end ());
    return it->second;
}

}  // namespace ih::core
