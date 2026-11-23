#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <vector>

#include "render/vulkan/context.h"
#include "render/vulkan/vk.h"

namespace ih::vulkan {

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

// Surface + swapchain + depth targets + framebuffers + per-frame sync.
// All methods run on the render thread.
class Swapchain {
public:
    void init (Context& context, SDL_Window* window, uint32_t width, uint32_t height);
    void recreate (uint32_t width, uint32_t height);

    // Frame cycle. beginFrame returns false when the swapchain went
    // out-of-date and was recreated; the caller skips that frame.
    bool beginFrame (uint32_t frame);
    void endFrame (uint32_t frame);

    vk::raii::CommandBuffer& commandBuffer (uint32_t frame) { return m_commandBuffers[frame]; }
    uint32_t imageIndex (uint32_t frame) const { return m_imageIndex[frame]; }
    uint32_t width () const { return m_width; }
    uint32_t height () const { return m_height; }
    vk::RenderPass renderPass () const { return m_renderPass; }
    vk::Format colorFormat () const { return m_colorFormat; }

private:
    void rebuild (uint32_t width, uint32_t height);
    void createRenderPass ();
    void createSyncObjects ();

    Context* m_context = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    vk::Format m_colorFormat = vk::Format::eUndefined;
    vk::ColorSpaceKHR m_colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    vk::raii::SurfaceKHR m_surface{nullptr};
    vk::raii::SwapchainKHR m_swapchain{nullptr};
    vk::raii::RenderPass m_renderPass{nullptr};

    std::vector<vk::raii::ImageView> m_colorViews;
    std::vector<vk::raii::Image> m_depthImages;
    std::vector<vk::raii::DeviceMemory> m_depthMemory;
    std::vector<vk::raii::ImageView> m_depthViews;
    std::vector<vk::raii::Framebuffer> m_framebuffers;

    vk::raii::CommandPool m_commandPool{nullptr};
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;

    std::array<vk::raii::Fence, FRAMES_IN_FLIGHT> m_fences{nullptr, nullptr};
    std::array<vk::raii::Semaphore, FRAMES_IN_FLIGHT> m_imageAvailable{nullptr, nullptr};
    std::vector<vk::raii::Semaphore> m_renderFinished;  // one per swapchain image
    std::array<uint32_t, FRAMES_IN_FLIGHT> m_imageIndex{};
};

}  // namespace ih::vulkan
