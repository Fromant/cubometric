#include "render/vulkan/texture_array.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

#include "core/check.h"
#include "core/log.h"
#include "stb_image.h"

namespace ih::vulkan {

namespace {

float srgbToLinear (uint8_t c) {
    const float s = c / 255.0f;
    return s <= 0.04045f ? s / 12.92f : std::pow ((s + 0.055f) / 1.055f, 2.4f);
}

uint8_t linearToSrgb (float l) {
    const float s = l <= 0.0031308f ? l * 12.92f : 1.055f * std::pow (l, 1.0f / 2.4f) - 0.055f;
    return static_cast<uint8_t> (std::clamp (std::lround (s * 255.0f), 0L, 255L));
}

// One mip level of RGBA8 tiles, row-major, layers consecutive.
struct MipLevel {
    int size = 0;
    std::vector<uint8_t> pixels{};  // size*size*4 per layer
};

std::vector<MipLevel> buildMips (const uint8_t* base, int tileSize, int levels, size_t layerCount) {
    std::vector<MipLevel> mips (levels);
    mips[0].size = tileSize;
    mips[0].pixels.resize (static_cast<size_t> (tileSize) * tileSize * 4 * layerCount);
    std::memcpy (mips[0].pixels.data (), base, mips[0].pixels.size ());

    for (int level = 1; level < levels; ++level) {
        const MipLevel& src = mips[level - 1];
        MipLevel& dst = mips[level];
        dst.size = std::max (1, src.size / 2);
        dst.pixels.resize (static_cast<size_t> (dst.size) * dst.size * 4 * layerCount);
        for (size_t layer = 0; layer < layerCount; ++layer) {
            const uint8_t* srcLayer = src.pixels.data () + layer * src.size * src.size * 4;
            uint8_t* dstLayer = dst.pixels.data () + layer * dst.size * dst.size * 4;
            for (int y = 0; y < dst.size; ++y) {
                for (int x = 0; x < dst.size; ++x) {
                    for (int c = 0; c < 4; ++c) {
                        float sum = 0.0f;
                        for (int dy = 0; dy < 2; ++dy) {
                            for (int dx = 0; dx < 2; ++dx) {
                                const int sx = std::min (src.size - 1, x * 2 + dx);
                                const int sy = std::min (src.size - 1, y * 2 + dy);
                                const uint8_t value = srcLayer[(sy * src.size + sx) * 4 + c];
                                sum += (c == 3) ? value / 255.0f : srgbToLinear (value);
                            }
                        }
                        const float avg = sum / 4.0f;
                        dstLayer[(y * dst.size + x) * 4 + c] =
                            (c == 3) ? static_cast<uint8_t> (std::lround (avg * 255.0f)) : linearToSrgb (avg);
                    }
                }
            }
        }
    }
    return mips;
}

}  // namespace

void TextureArray::load (Context& context, const std::string& folder, int tileSize, int mipLevels) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator (folder)) {
        if (entry.is_regular_file () && entry.path ().extension () == ".png") files.push_back (entry.path ());
    }
    std::sort (files.begin (), files.end ());
    IH_CHECK (!files.empty ());

    // Load base levels.
    std::vector<uint8_t> base;
    for (const auto& path : files) {
        int w = 0, h = 0, channels = 0;
        uint8_t* data = stbi_load (path.string ().c_str (), &w, &h, &channels, STBI_rgb_alpha);
        IH_CHECK (data != nullptr);
        IH_CHECK (w == tileSize && h == tileSize);
        base.insert (base.end (), data, data + static_cast<size_t> (w) * h * 4);
        stbi_image_free (data);
        const std::string name = path.stem ().string ();
        m_layerByName[name] = static_cast<uint32_t> (m_layers.size ());
        m_layers.push_back (name);
    }

    const auto mips = buildMips (base.data (), tileSize, mipLevels, m_layers.size ());
    size_t totalSize = 0;
    for (const auto& mip : mips) totalSize += mip.pixels.size ();

    auto& device = context.device ();
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = vk::Format::eR8G8B8A8Srgb;
    imageInfo.extent = vk::Extent3D (tileSize, tileSize, 1);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = static_cast<uint32_t> (m_layers.size ());
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    m_image = vk::raii::Image (device, imageInfo);

    const auto requirements = m_image.getMemoryRequirements ();
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType (context.physicalDevice (), requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_memory = vk::raii::DeviceMemory (device, allocInfo);
    m_image.bindMemory (m_memory, 0);

    // Staging upload: all mips of all layers in one buffer, one copy call.
    vk::BufferCreateInfo stagingInfo{};
    stagingInfo.size = totalSize;
    stagingInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    vk::raii::Buffer staging (device, stagingInfo);
    const auto stagingRequirements = staging.getMemoryRequirements ();
    vk::MemoryAllocateInfo stagingAlloc{};
    stagingAlloc.allocationSize = stagingRequirements.size;
    stagingAlloc.memoryTypeIndex = findMemoryType (context.physicalDevice (), stagingRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eHostVisible
                                                       | vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::raii::DeviceMemory stagingMemory (device, stagingAlloc);
    staging.bindMemory (stagingMemory, 0);
    uint8_t* mapped = static_cast<uint8_t*> (stagingMemory.mapMemory (0, totalSize));
    size_t cursor = 0;
    std::vector<vk::BufferImageCopy> regions;
    for (int level = 0; level < mipLevels; ++level) {
        const auto& mip = mips[level];
        const size_t layerBytes = static_cast<size_t> (mip.size) * mip.size * 4;
        for (uint32_t layer = 0; layer < m_layers.size (); ++layer) {
            std::memcpy (mapped + cursor, mip.pixels.data () + layer * layerBytes, layerBytes);
            vk::BufferImageCopy region{};
            region.bufferOffset = cursor;
            region.imageSubresource = {vk::ImageAspectFlagBits::eColor, static_cast<uint32_t> (level), layer, 1};
            region.imageExtent = vk::Extent3D (mip.size, mip.size, 1);
            regions.push_back (region);
            cursor += layerBytes;
        }
    }
    stagingMemory.unmapMemory ();

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = context.graphicsFamily ();
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::raii::CommandPool pool (device, poolInfo);
    vk::CommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.commandPool = pool;
    cmdAlloc.level = vk::CommandBufferLevel::ePrimary;
    cmdAlloc.commandBufferCount = 1;
    auto cmds = device.allocateCommandBuffers (cmdAlloc);
    auto& cmd = cmds[0];
    cmd.begin (vk::CommandBufferBeginInfo (vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::ImageMemoryBarrier toTransfer{};
    toTransfer.oldLayout = vk::ImageLayout::eUndefined;
    toTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = m_image;
    toTransfer.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, static_cast<uint32_t> (mipLevels), 0,
                                   static_cast<uint32_t> (m_layers.size ())};
    toTransfer.srcAccessMask = {};
    toTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    cmd.pipelineBarrier (vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                         toTransfer);

    cmd.copyBufferToImage (staging, m_image, vk::ImageLayout::eTransferDstOptimal, regions);

    vk::ImageMemoryBarrier toRead = toTransfer;
    toRead.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    toRead.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    toRead.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    toRead.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    cmd.pipelineBarrier (vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                         toRead);
    cmd.end ();

    const vk::CommandBuffer cmdHandle = *cmd;
    vk::SubmitInfo submit{};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmdHandle;
    context.queue ().submit (submit, nullptr);
    context.queue ().waitIdle ();

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = m_image;
    viewInfo.viewType = vk::ImageViewType::e2DArray;
    viewInfo.format = vk::Format::eR8G8B8A8Srgb;
    viewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, static_cast<uint32_t> (mipLevels), 0,
                                 static_cast<uint32_t> (m_layers.size ())};
    m_view = vk::raii::ImageView (device, viewInfo);

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.maxLod = static_cast<float> (mipLevels);
    m_sampler = vk::raii::Sampler (device, samplerInfo);

    core::logInfo ("Textures: " + std::to_string (m_layers.size ()) + " layers, " + std::to_string (mipLevels)
                   + " mip levels");
}

uint32_t TextureArray::layerOf (const std::string& name) const {
    const auto it = m_layerByName.find (name);
    IH_CHECK (it != m_layerByName.end ());
    return it->second;
}

}  // namespace ih::vulkan
