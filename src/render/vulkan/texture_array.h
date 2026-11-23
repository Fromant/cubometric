#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "render/vulkan/context.h"
#include "render/vulkan/vk.h"

namespace ih::vulkan {

// Block texture array: one 2D-array image, tile-sized layers, sRGB format,
// CPU-generated mip chain (box filter in linear space). Nearest filtering
// with repeat addressing, matching the GL build's look.
class TextureArray {
public:
    // Loads every png under folder (sorted => deterministic layer ids).
    void load (Context& context, const std::string& folder, int tileSize, int mipLevels);

    vk::ImageView view () const { return m_view; }
    vk::Sampler sampler () const { return m_sampler; }
    uint32_t layerCount () const { return static_cast<uint32_t> (m_layers.size ()); }
    uint32_t layerOf (const std::string& name) const;

private:
    std::vector<std::string> m_layers{};
    std::unordered_map<std::string, uint32_t> m_layerByName{};
    vk::raii::Image m_image{nullptr};
    vk::raii::DeviceMemory m_memory{nullptr};
    vk::raii::ImageView m_view{nullptr};
    vk::raii::Sampler m_sampler{nullptr};
};

}  // namespace ih::vulkan
