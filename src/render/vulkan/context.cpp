#include "render/vulkan/context.h"

#include <array>

#include "core/check.h"
#include "core/log.h"

namespace ih::vulkan {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                              vk::DebugUtilsMessageTypeFlagsEXT /*type*/,
                                              const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                              void* /*user*/) {
    const auto level = (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) ? core::LogLevel::Error
                       : (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) ? core::LogLevel::Warn
                                                                                         : core::LogLevel::Info;
    core::logf (level, "vk: %s", data->pMessage);
    return VK_FALSE;
}

vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo () {
    vk::DebugUtilsMessengerCreateInfoEXT info{};
    info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    info.pfnUserCallback = &debugCallback;
    return info;
}

std::vector<const char*> toCStrings (const std::vector<std::string>& strings) {
    std::vector<const char*> out;
    out.reserve (strings.size ());
    for (const auto& s : strings) out.push_back (s.c_str ());
    return out;
}

bool instanceExtensionAvailable (const std::string& name) {
    for (const auto& ext : vk::enumerateInstanceExtensionProperties ()) {
        if (name == ext.extensionName) return true;
    }
    return false;
}

bool deviceExtensionAvailable (const vk::raii::PhysicalDevice& device, const std::string& name) {
    for (const auto& ext : device.enumerateDeviceExtensionProperties ()) {
        if (name == ext.extensionName) return true;
    }
    return false;
}

int deviceTypeScore (vk::PhysicalDeviceType type) {
    switch (type) {
        case vk::PhysicalDeviceType::eDiscreteGpu: return 3;
        case vk::PhysicalDeviceType::eVirtualGpu: return 2;
        case vk::PhysicalDeviceType::eIntegratedGpu: return 1;
        default: return 0;
    }
}

}  // namespace

Context::Context (bool validation, const std::vector<std::string>& instanceExtensions) : m_validation (validation) {
    // --- Instance ---
    std::vector<std::string> extensions = instanceExtensions;
    if (validation && instanceExtensionAvailable (VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        extensions.emplace_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    const std::vector<std::string> layers = validation ? std::vector<std::string>{"VK_LAYER_KHRONOS_validation"}
                                                       : std::vector<std::string>{};
    const auto extPtrs = toCStrings (extensions);
    const auto layerPtrs = toCStrings (layers);

    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "IndustrialHard";
    appInfo.applicationVersion = VK_MAKE_VERSION (1, 0, 0);
    appInfo.pEngineName = "IndustrialHard";
    appInfo.engineVersion = VK_MAKE_VERSION (1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const auto debugInfo = debugMessengerInfo ();
    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = static_cast<uint32_t> (layerPtrs.size ());
    instanceInfo.ppEnabledLayerNames = layerPtrs.data ();
    instanceInfo.enabledExtensionCount = static_cast<uint32_t> (extPtrs.size ());
    instanceInfo.ppEnabledExtensionNames = extPtrs.data ();
    instanceInfo.pNext = validation ? &debugInfo : nullptr;

    m_instance = vk::raii::Instance (m_loader, instanceInfo);

    // --- Physical device: prefer discrete GPUs ---
    auto devices = m_instance.enumeratePhysicalDevices ();
    IH_CHECK (!devices.empty ());
    vk::raii::PhysicalDevice* best = &devices[0];
    for (auto& candidate : devices) {
        if (deviceTypeScore (candidate.getProperties ().deviceType) > deviceTypeScore (best->getProperties ().deviceType)) {
            best = &candidate;
        }
    }
    m_physicalDevice = std::move (*best);
    core::logInfo ("Vulkan device: " + std::string (m_physicalDevice.getProperties ().deviceName.data ()));

    // --- Queue family: first one with graphics support ---
    const auto families = m_physicalDevice.getQueueFamilyProperties ();
    bool found = false;
    for (uint32_t i = 0; i < families.size (); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_graphicsFamily = i;
            found = true;
            break;
        }
    }
    IH_CHECK (found);

    // --- Logical device + queue ---
    std::vector<std::string> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (validation && deviceExtensionAvailable (m_physicalDevice, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        deviceExtensions.emplace_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    const auto deviceExtPtrs = toCStrings (deviceExtensions);

    const float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo{};
    queueInfo.queueFamilyIndex = m_graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    vk::DeviceCreateInfo deviceInfo{};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtPtrs.size ());
    deviceInfo.ppEnabledExtensionNames = deviceExtPtrs.data ();
    // Note: the debug messenger lives in the instance pNext chain only; it
    // receives device-level messages as well.

    // Wireframe rendering needs non-solid fill modes.
    vk::PhysicalDeviceFeatures features{};
    if (m_physicalDevice.getFeatures ().fillModeNonSolid) {
        features.fillModeNonSolid = vk::True;
        deviceInfo.pEnabledFeatures = &features;
    }

    m_device = vk::raii::Device (m_physicalDevice, deviceInfo);
    m_queue = vk::raii::Queue (m_device, m_graphicsFamily, 0);
}

uint32_t findMemoryType (const vk::raii::PhysicalDevice& physicalDevice,
                         uint32_t typeBits,
                         vk::MemoryPropertyFlags properties) {
    const auto memoryProperties = physicalDevice.getMemoryProperties ();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        const bool typeOk = (typeBits & (1u << i)) != 0;
        const bool propsOk = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (typeOk && propsOk) return i;
    }
    ih::checkFail ("no suitable Vulkan memory type", __FILE__, __LINE__);
}

}  // namespace ih::vulkan
