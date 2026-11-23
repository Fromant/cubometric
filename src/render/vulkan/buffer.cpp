#include "render/vulkan/buffer.h"

#include <cstring>

#include "core/check.h"

namespace ih::vulkan {

namespace {

// One-shot copy on the graphics queue; waits for completion. Fine for startup
// uploads; per-frame streaming gets a proper staging ring in M1.
void copyBuffer (Context& context, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = context.graphicsFamily ();
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::raii::CommandPool pool (context.device (), poolInfo);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;
    auto commandBuffers = context.device ().allocateCommandBuffers (allocInfo);
    auto& cmd = commandBuffers[0];

    cmd.begin (vk::CommandBufferBeginInfo (vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    cmd.copyBuffer (src, dst, vk::BufferCopy (0, 0, size));
    cmd.end ();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    const vk::CommandBuffer cmdHandle = *cmd;
    submitInfo.pCommandBuffers = &cmdHandle;
    context.queue ().submit (submitInfo, nullptr);
    context.queue ().waitIdle ();
}

}  // namespace

void Buffer::create (Context& context, vk::DeviceSize size, vk::BufferUsageFlags usage, const void* data) {
    m_size = size;

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = data ? (usage | vk::BufferUsageFlagBits::eTransferDst) : usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    m_buffer = vk::raii::Buffer (context.device (), bufferInfo);

    const auto requirements = m_buffer.getMemoryRequirements ();
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType (context.physicalDevice (), requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_memory = vk::raii::DeviceMemory (context.device (), allocInfo);
    m_buffer.bindMemory (m_memory, 0);

    if (data) {
        vk::BufferCreateInfo stagingInfo{};
        stagingInfo.size = size;
        stagingInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        stagingInfo.sharingMode = vk::SharingMode::eExclusive;
        vk::raii::Buffer staging (context.device (), stagingInfo);

        const auto stagingRequirements = staging.getMemoryRequirements ();
        vk::MemoryAllocateInfo stagingAlloc{};
        stagingAlloc.allocationSize = stagingRequirements.size;
        stagingAlloc.memoryTypeIndex = findMemoryType (context.physicalDevice (),
                                                       stagingRequirements.memoryTypeBits,
                                                       vk::MemoryPropertyFlagBits::eHostVisible
                                                           | vk::MemoryPropertyFlagBits::eHostCoherent);
        vk::raii::DeviceMemory stagingMemory (context.device (), stagingAlloc);
        staging.bindMemory (stagingMemory, 0);

        void* mapped = stagingMemory.mapMemory (0, size);
        std::memcpy (mapped, data, static_cast<size_t> (size));
        stagingMemory.unmapMemory ();

        copyBuffer (context, staging, m_buffer, size);
    }
}

}  // namespace ih::vulkan
