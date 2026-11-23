#pragma once

#include "render/vulkan/context.h"
#include "render/vulkan/vk.h"

namespace ih::vulkan {

// A device-local buffer with its own memory allocation. Initial data is
// uploaded through a transient host-visible staging buffer.
// (M1 replaces per-chunk uses of this with a shared suballocated arena.)
class Buffer {
public:
    void create (Context& context, vk::DeviceSize size, vk::BufferUsageFlags usage, const void* data = nullptr);

    vk::Buffer handle () const { return m_buffer; }
    vk::DeviceSize size () const { return m_size; }

private:
    vk::raii::Buffer m_buffer{nullptr};
    vk::raii::DeviceMemory m_memory{nullptr};
    vk::DeviceSize m_size = 0;
};

}  // namespace ih::vulkan
