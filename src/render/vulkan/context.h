#pragma once

#include <string>
#include <vector>

#include "render/vulkan/vk.h"

namespace ih::vulkan {

// Owns the Vulkan instance, the chosen physical device, the logical device and
// the graphics queue. Created and destroyed on the render thread only.
class Context {
public:
    Context (bool validation, const std::vector<std::string>& instanceExtensions);

    vk::raii::Instance& instance () { return m_instance; }
    vk::raii::PhysicalDevice& physicalDevice () { return m_physicalDevice; }
    vk::raii::Device& device () { return m_device; }
    vk::raii::Queue& queue () { return m_queue; }

    uint32_t graphicsFamily () const { return m_graphicsFamily; }
    bool validation () const { return m_validation; }

private:
    bool m_validation;
    uint32_t m_graphicsFamily = 0;

    vk::raii::Context m_loader;
    vk::raii::Instance m_instance{nullptr};
    vk::raii::PhysicalDevice m_physicalDevice{nullptr};
    vk::raii::Device m_device{nullptr};
    vk::raii::Queue m_queue{nullptr};
};

// Picks a memory type satisfying both the resource's type bits and the
// requested properties. Aborts if the device offers none.
uint32_t findMemoryType (const vk::raii::PhysicalDevice& physicalDevice,
                         uint32_t typeBits,
                         vk::MemoryPropertyFlags properties);

}  // namespace ih::vulkan
