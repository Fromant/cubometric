#include "render/vulkan/arena.h"

#include <cstring>

#include "core/check.h"

namespace ih::vulkan {

void Arena::create (Context& context, vk::DeviceSize size) {
    m_size = size;
    m_freeTotal = size;
    m_free[0] = size;

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                     | vk::BufferUsageFlagBits::eTransferDst;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    m_buffer = vk::raii::Buffer (context.device (), bufferInfo);

    const auto requirements = m_buffer.getMemoryRequirements ();
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType (context.physicalDevice (), requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_memory = vk::raii::DeviceMemory (context.device (), allocInfo);
    m_buffer.bindMemory (m_memory, 0);
}

std::optional<vk::DeviceSize> Arena::allocate (vk::DeviceSize size, vk::DeviceSize alignment) {
    for (auto it = m_free.begin (); it != m_free.end (); ++it) {
        const vk::DeviceSize blockOffset = it->first;
        const vk::DeviceSize blockLength = it->second;
        const vk::DeviceSize aligned = (blockOffset + alignment - 1) & ~(alignment - 1);
        const vk::DeviceSize padding = aligned - blockOffset;
        if (blockLength < padding + size) continue;

        m_free.erase (it);
        if (padding > 0) m_free[blockOffset] = padding;
        const vk::DeviceSize remainder = blockLength - padding - size;
        if (remainder > 0) m_free[aligned + size] = remainder;
        m_freeTotal -= size;
        return aligned;
    }
    return std::nullopt;
}

void Arena::free (vk::DeviceSize offset, vk::DeviceSize size) {
    m_freeTotal += size;
    // Coalesce with the previous and next free blocks.
    auto next = m_free.lower_bound (offset);
    if (next != m_free.begin ()) {
        auto prev = std::prev (next);
        if (prev->first + prev->second == offset) {
            offset = prev->first;
            size += prev->second;
            m_free.erase (prev);
        }
    }
    next = m_free.lower_bound (offset);
    if (next != m_free.end () && offset + size == next->first) {
        size += next->second;
        m_free.erase (next);
    }
    m_free[offset] = size;
}

void UploadRing::create (Context& context, vk::DeviceSize slotSize, uint32_t slots) {
    m_slotSize = slotSize;
    m_cursor.assign (slots, 0);

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = slotSize * slots;
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    m_buffer = vk::raii::Buffer (context.device (), bufferInfo);

    const auto requirements = m_buffer.getMemoryRequirements ();
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = findMemoryType (context.physicalDevice (), requirements.memoryTypeBits,
                                               vk::MemoryPropertyFlagBits::eHostVisible
                                                   | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_memory = vk::raii::DeviceMemory (context.device (), allocInfo);
    m_buffer.bindMemory (m_memory, 0);
    m_mapped = static_cast<uint8_t*> (m_memory.mapMemory (0, bufferInfo.size));
}

vk::DeviceSize UploadRing::write (uint32_t slot, const void* data, vk::DeviceSize size) {
    IH_CHECK (m_cursor[slot] + size <= m_slotSize);
    const vk::DeviceSize offset = slot * m_slotSize + m_cursor[slot];
    std::memcpy (m_mapped + offset, data, static_cast<size_t> (size));
    m_cursor[slot] += size;
    return offset;
}

void UploadRing::resetSlot (uint32_t slot) {
    m_cursor[slot] = 0;
}

}  // namespace ih::vulkan
