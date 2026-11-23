#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "render/vulkan/context.h"
#include "render/vulkan/vk.h"

namespace ih::vulkan {

// One big device-local buffer suballocated with a first-fit free list.
// All chunk meshes (vertices + indices) live here, so a frame binds one
// vertex buffer and one index buffer regardless of chunk count.
class Arena {
public:
    void create (Context& context, vk::DeviceSize size);

    // Returns a byte offset, or nullopt when the arena is exhausted.
    std::optional<vk::DeviceSize> allocate (vk::DeviceSize size, vk::DeviceSize alignment = 16);
    void free (vk::DeviceSize offset, vk::DeviceSize size);

    vk::Buffer buffer () const { return m_buffer; }
    vk::DeviceSize used () const { return m_size - m_freeTotal; }

private:
    vk::raii::Buffer m_buffer{nullptr};
    vk::raii::DeviceMemory m_memory{nullptr};
    vk::DeviceSize m_size = 0;
    vk::DeviceSize m_freeTotal = 0;
    std::map<vk::DeviceSize, vk::DeviceSize> m_free{};  // offset -> length
};

// Host-visible staging ring, one slot per frame in flight. A slot is only
// rewritten after its frame's fence has been waited on (Swapchain::beginFrame).
class UploadRing {
public:
    void create (Context& context, vk::DeviceSize slotSize, uint32_t slots);

    // Copies data into the slot and returns the buffer offset to copy from.
    vk::DeviceSize write (uint32_t slot, const void* data, vk::DeviceSize size);
    void resetSlot (uint32_t slot);

    vk::Buffer buffer () const { return m_buffer; }

private:
    vk::raii::Buffer m_buffer{nullptr};
    vk::raii::DeviceMemory m_memory{nullptr};
    uint8_t* m_mapped = nullptr;
    vk::DeviceSize m_slotSize = 0;
    std::vector<vk::DeviceSize> m_cursor{};
};

}  // namespace ih::vulkan
