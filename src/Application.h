#pragma once

#define VK_PROTOTYPES
#include <optional>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include "game/world/World.h"
#include "render/Camera.h"

constexpr std::string NAME = "Cubometric";

class Application {
public:
    Application();
    void Run();
    ~Application();

private:
    SDL_Window* Window{};
    bool captureMouse = false;

    uint32_t currentFrame = 0;

    // Application objects
    Camera camera;
    World world;

    unsigned int width = 800;
    unsigned int height = 600;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    void Init();
    bool HandleEvents();
    void Update(float deltaTime);
    void UpdateViewport();

    void initVulkan();

    //Vulkan vars & helper functions
    void createInstance();
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    void pickPhysicalDevice();

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    void createSwapChain();
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImageView> swapChainImageViews;
    void createImageViews();

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    void createRenderPass();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline();


    VkDevice device = VK_NULL_HANDLE;
    void createLogicalDevice();

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    void createSurface();

    std::vector<VkFramebuffer> swapChainFramebuffers;
    void createFramebuffers();

    VkCommandPool commandPool;
    void createCommandPool();

    std::vector<VkCommandBuffer> commandBuffers;
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


    void createSyncObjects();
    void drawFrame();

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;


    #ifdef DEBUG
    static constexpr bool enableValidationLayers = true;
    #else
    static constexpr bool enableValidationLayers = false;
    #endif


    void cleanup();
};
