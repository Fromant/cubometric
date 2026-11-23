#pragma once

// Single include point for the Vulkan C++ bindings (ship with the SDK).
// VK_USE_PLATFORM_WIN32_KHR is set by the build system.
// Return out-of-date swapchain results instead of throwing, so the render
// loop can recreate the swapchain gracefully.
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
