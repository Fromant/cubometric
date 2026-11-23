#include "render/vulkan/swapchain.h"

#include <algorithm>

#include "core/check.h"
#include "core/log.h"

namespace ih::vulkan {

namespace {

constexpr vk::Format DEPTH_FORMAT = vk::Format::eD32Sfloat;

vk::SurfaceFormatKHR pickSurfaceFormat (const std::vector<vk::SurfaceFormatKHR>& formats) {
    for (const auto& format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Unorm) return format;
    }
    return formats[0];
}

}  // namespace

void Swapchain::init (Context& context, SDL_Window* window, uint32_t width, uint32_t height) {
    m_context = &context;

    // Win32 surface from the SDL window's native handle.
    HWND hwnd = static_cast<HWND> (
        SDL_GetPointerProperty (SDL_GetWindowProperties (window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    IH_CHECK (hwnd != nullptr);
    vk::Win32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.hinstance = GetModuleHandleW (nullptr);
    surfaceInfo.hwnd = hwnd;
    m_surface = vk::raii::SurfaceKHR (context.instance (), surfaceInfo);

    // The render pass needs the final color format, so pick it up front.
    const auto surfaceFormat = pickSurfaceFormat (context.physicalDevice ().getSurfaceFormatsKHR (m_surface));
    m_colorFormat = surfaceFormat.format;
    m_colorSpace = surfaceFormat.colorSpace;

    createRenderPass ();
    createSyncObjects ();
    rebuild (width, height);
}

void Swapchain::createRenderPass () {
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = m_colorFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = DEPTH_FORMAT;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorRef (0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthRef (1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array attachments = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = static_cast<uint32_t> (attachments.size ());
    renderPassInfo.pAttachments = attachments.data ();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    m_renderPass = vk::raii::RenderPass (m_context->device (), renderPassInfo);
}

void Swapchain::createSyncObjects () {
    // Fences start signaled so the first frame's wait passes immediately.
    const vk::FenceCreateInfo fenceInfo (vk::FenceCreateFlagBits::eSignaled);
    const vk::SemaphoreCreateInfo semaphoreInfo{};
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        m_fences[i] = vk::raii::Fence (m_context->device (), fenceInfo);
        m_imageAvailable[i] = vk::raii::Semaphore (m_context->device (), semaphoreInfo);
    }

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = m_context->graphicsFamily ();
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    m_commandPool = vk::raii::CommandPool (m_context->device (), poolInfo);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = FRAMES_IN_FLIGHT;
    m_commandBuffers = m_context->device ().allocateCommandBuffers (allocInfo);
}

void Swapchain::recreate (uint32_t width, uint32_t height) {
    m_context->device ().waitIdle ();
    rebuild (width, height);
}

void Swapchain::rebuild (uint32_t width, uint32_t height) {
    auto& device = m_context->device ();
    auto& physicalDevice = m_context->physicalDevice ();

    // Drop old per-size resources (RAII destroys them here).
    m_framebuffers.clear ();
    m_depthViews.clear ();
    m_depthMemory.clear ();
    m_depthImages.clear ();
    m_colorViews.clear ();

    const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR (m_surface);

    vk::Extent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = std::clamp (width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp (height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    m_width = extent.width;
    m_height = extent.height;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) imageCount = std::min (imageCount, capabilities.maxImageCount);

    vk::SwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.surface = m_surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = m_colorFormat;
    swapchainInfo.imageColorSpace = m_colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainInfo.presentMode = vk::PresentModeKHR::eFifo;
    swapchainInfo.clipped = vk::True;
    swapchainInfo.oldSwapchain = m_swapchain;
    m_swapchain = vk::raii::SwapchainKHR (device, swapchainInfo);

    const auto images = m_swapchain.getImages ();

    // Present-wait semaphores are per swapchain image: an image is only
    // re-acquired after its previous present completed, so its semaphore is
    // guaranteed unsignaled by then. (Per-frame semaphores here would race
    // with the presentation engine — VUID-vkQueueSubmit-pSignalSemaphores-00067.)
    m_renderFinished.clear ();
    const vk::SemaphoreCreateInfo semaphoreInfo{};
    for (size_t i = 0; i < images.size (); ++i) {
        m_renderFinished.emplace_back (device, semaphoreInfo);
    }

    for (const auto& image : images) {
        vk::ImageViewCreateInfo colorViewInfo{};
        colorViewInfo.image = image;
        colorViewInfo.viewType = vk::ImageViewType::e2D;
        colorViewInfo.format = m_colorFormat;
        colorViewInfo.subresourceRange = vk::ImageSubresourceRange (vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        m_colorViews.emplace_back (device, colorViewInfo);

        vk::ImageCreateInfo depthInfo{};
        depthInfo.imageType = vk::ImageType::e2D;
        depthInfo.format = DEPTH_FORMAT;
        depthInfo.extent = vk::Extent3D (extent.width, extent.height, 1);
        depthInfo.mipLevels = 1;
        depthInfo.arrayLayers = 1;
        depthInfo.samples = vk::SampleCountFlagBits::e1;
        depthInfo.tiling = vk::ImageTiling::eOptimal;
        depthInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depthInfo.sharingMode = vk::SharingMode::eExclusive;
        depthInfo.initialLayout = vk::ImageLayout::eUndefined;
        m_depthImages.emplace_back (device, depthInfo);

        const auto requirements = m_depthImages.back ().getMemoryRequirements ();
        vk::MemoryAllocateInfo depthAlloc{};
        depthAlloc.allocationSize = requirements.size;
        depthAlloc.memoryTypeIndex =
            findMemoryType (physicalDevice, requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_depthMemory.emplace_back (device, depthAlloc);
        m_depthImages.back ().bindMemory (m_depthMemory.back (), 0);

        vk::ImageViewCreateInfo depthViewInfo{};
        depthViewInfo.image = m_depthImages.back ();
        depthViewInfo.viewType = vk::ImageViewType::e2D;
        depthViewInfo.format = DEPTH_FORMAT;
        depthViewInfo.subresourceRange = vk::ImageSubresourceRange (vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);
        m_depthViews.emplace_back (device, depthViewInfo);

        std::array attachments = {*m_colorViews.back (), *m_depthViews.back ()};
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t> (attachments.size ());
        framebufferInfo.pAttachments = attachments.data ();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        m_framebuffers.emplace_back (device, framebufferInfo);
    }
}

bool Swapchain::beginFrame (uint32_t frame) {
    auto& device = m_context->device ();
    const vk::Fence fence = m_fences[frame];
    (void) device.waitForFences (fence, vk::True, UINT64_MAX);
    device.resetFences (fence);

    const auto [result, index] = m_swapchain.acquireNextImage (UINT64_MAX, m_imageAvailable[frame], nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreate (m_width, m_height);
        return false;
    }
    m_imageIndex[frame] = index;

    auto& cmd = m_commandBuffers[frame];
    cmd.begin (vk::CommandBufferBeginInfo (vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue (std::array<float, 4>{0.02f, 0.02f, 0.04f, 1.0f});
    clears[1].depthStencil = vk::ClearDepthStencilValue (1.0f, 0);

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[index];
    renderPassInfo.renderArea = vk::Rect2D (vk::Offset2D (0, 0), vk::Extent2D (m_width, m_height));
    renderPassInfo.clearValueCount = static_cast<uint32_t> (clears.size ());
    renderPassInfo.pClearValues = clears.data ();
    cmd.beginRenderPass (renderPassInfo, vk::SubpassContents::eInline);
    return true;
}

void Swapchain::endFrame (uint32_t frame) {
    auto& cmd = m_commandBuffers[frame];
    cmd.endRenderPass ();
    cmd.end ();

    const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::Semaphore waitSemaphore = m_imageAvailable[frame];
    const vk::Semaphore signalSemaphore = m_renderFinished[m_imageIndex[frame]];
    const vk::CommandBuffer cmdHandle = *cmd;
    const vk::SubmitInfo submitInfo (waitSemaphore, waitStage, cmdHandle, signalSemaphore);
    m_context->queue ().submit (submitInfo, m_fences[frame]);

    const vk::SwapchainKHR swapchainHandle = m_swapchain;
    const vk::PresentInfoKHR presentInfo (signalSemaphore, swapchainHandle, m_imageIndex[frame]);
    const auto result = m_context->queue ().presentKHR (presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        recreate (m_width, m_height);
    }
}

}  // namespace ih::vulkan
