#include "VulkanContext.hpp"
#include <spdlog/spdlog.h>
#include <set>
#include <cstring>

// GLFW包含Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace mr::client {

// 验证层名称
const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

VulkanContext::VulkanContext() = default;

VulkanContext::~VulkanContext() {
    destroy();
}

Result<void> VulkanContext::initialize(const VulkanConfig& config, VkSurfaceKHR surface) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Vulkan context already initialized");
    }

    m_config = config;
    m_surface = surface;

    // 创建实例
    auto instanceResult = createInstance();
    if (instanceResult.failed()) {
        return instanceResult.error();
    }

    // 设置调试信使
    if (m_validationEnabled) {
        auto debugResult = setupDebugMessenger();
        if (debugResult.failed()) {
            spdlog::warn("Failed to setup debug messenger: {}", debugResult.error().toString());
        }
    }

    // 选择物理设备
    auto deviceResult = pickPhysicalDevice();
    if (deviceResult.failed()) {
        destroy();
        return deviceResult.error();
    }

    // 创建逻辑设备
    auto logicalResult = createLogicalDevice();
    if (logicalResult.failed()) {
        destroy();
        return logicalResult.error();
    }

    m_initialized = true;
    spdlog::info("Vulkan context initialized successfully");
    spdlog::info("GPU: {}", m_deviceProperties.deviceName);
    spdlog::info("Vulkan API: {}.{}.{}",
        VK_API_VERSION_MAJOR(m_deviceProperties.apiVersion),
        VK_API_VERSION_MINOR(m_deviceProperties.apiVersion),
        VK_API_VERSION_PATCH(m_deviceProperties.apiVersion));

    return Result<void>::ok();
}

void VulkanContext::destroy() {
    if (!m_initialized) {
        return;
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    m_initialized = false;
    spdlog::info("Vulkan context destroyed");
}

Result<void> VulkanContext::createInstance() {
    // 检查验证层支持
    m_validationEnabled = m_config.enableValidation && checkValidationLayerSupport();
    if (m_config.enableValidation && !m_validationEnabled) {
        spdlog::warn("Validation layers requested but not available");
    }

    // 应用信息
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_config.appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(
        m_config.appVersion.major,
        m_config.appVersion.minor,
        m_config.appVersion.patch);
    appInfo.pEngineName = m_config.engineName.c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(
        m_config.engineVersion.major,
        m_config.engineVersion.minor,
        m_config.engineVersion.patch);
    appInfo.apiVersion = VK_MAKE_VERSION(
        m_config.apiVersion.major,
        m_config.apiVersion.minor,
        m_config.apiVersion.patch);

    // 实例创建信息
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // 扩展
    auto extensions = getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // 层
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_validationEnabled) {
        createInfo.enabledLayerCount = static_cast<u32>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        // 设置调试信使创建信息，用于vkCreateInstance和vkDestroyInstance期间的调试
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create Vulkan instance: " + std::to_string(result));
    }

    spdlog::info("Vulkan instance created");
    return Result<void>::ok();
}

Result<void> VulkanContext::setupDebugMessenger() {
    if (!m_validationEnabled) {
        return Result<void>::ok();
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func == nullptr) {
        return Error(ErrorCode::NotFound, "Failed to load vkCreateDebugUtilsMessengerEXT");
    }

    VkResult result = func(m_instance, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to setup debug messenger: " + std::to_string(result));
    }

    return Result<void>::ok();
}

Result<void> VulkanContext::pickPhysicalDevice() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return Error(ErrorCode::NotFound, "No Vulkan-capable GPUs found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    spdlog::info("Found {} Vulkan-capable GPU(s)", deviceCount);

    // 按评分选择最佳设备
    i32 bestScore = -1;
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

    for (const auto& device : devices) {
        if (!isDeviceSuitable(device)) {
            continue;
        }

        // 评分设备
        i32 score = 0;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        // 独立显卡优先
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // 最大纹理大小影响画质
        score += static_cast<i32>(props.limits.maxImageDimension2D);

        if (score > bestScore) {
            bestScore = score;
            bestDevice = device;
        }
    }

    if (bestDevice == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotFound, "No suitable GPU found");
    }

    m_physicalDevice = bestDevice;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
    m_queueFamilies = findQueueFamilies(m_physicalDevice);

    return Result<void>::ok();
}

Result<void> VulkanContext::createLogicalDevice() {
    QueueFamilyIndices indices = m_queueFamilies;

    // 创建唯一队列族集合
    std::set<u32> uniqueQueueFamilies;
    if (indices.graphicsFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.graphicsFamily.value());
    }
    if (indices.presentFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.presentFamily.value());
    }
    if (indices.transferFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.transferFamily.value());
    }
    if (indices.computeFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.computeFamily.value());
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 设备特性
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = m_deviceFeatures.samplerAnisotropy;
    deviceFeatures.fillModeNonSolid = m_deviceFeatures.fillModeNonSolid;
    deviceFeatures.wideLines = m_deviceFeatures.wideLines;

    // 设备创建信息
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // 扩展
    auto extensions = getRequiredDeviceExtensions();
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // 层 (为了兼容旧版本)
    if (m_validationEnabled) {
        createInfo.enabledLayerCount = static_cast<u32>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::Unknown, "Failed to create logical device: " + std::to_string(result));
    }

    // 获取队列
    if (indices.graphicsFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    }
    if (indices.presentFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    }
    if (indices.transferFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);
    }
    if (indices.computeFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.computeFamily.value(), 0, &m_computeQueue);
    }

    spdlog::info("Logical device created");
    return Result<void>::ok();
}

bool VulkanContext::checkValidationLayerSupport() {
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS) {
        bool layerFound = false;
        for (const auto& layerProps : availableLayers) {
            if (strcmp(layerName, layerProps.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }

    return true;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<String> requiredExtensions(m_config.requiredDeviceExtensions.begin(),
                                         m_config.requiredDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    if (!indices.isComplete()) {
        return false;
    }

    if (!checkDeviceExtensionSupport(device)) {
        return false;
    }

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    u32 i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // 图形队列
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // 呈现队列
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        // 传输队列 (优先选择专用的)
        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.transferFamily = i;
        } else if (!indices.transferFamily.has_value() &&
                   (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
            indices.transferFamily = i;
        }

        // 计算队列 (优先选择专用的)
        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.computeFamily = i;
        } else if (!indices.computeFamily.has_value() &&
                   (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.computeFamily = i;
        }

        if (indices.isComplete() && indices.hasTransfer() && indices.hasCompute()) {
            break;
        }

        i++;
    }

    // 如果没有找到专用传输/计算队列，使用图形队列
    if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value()) {
        indices.transferFamily = indices.graphicsFamily;
    }
    if (!indices.computeFamily.has_value() && indices.graphicsFamily.has_value()) {
        indices.computeFamily = indices.graphicsFamily;
    }

    return indices;
}

SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

SwapChainSupportDetails VulkanContext::querySwapChainSupport() const {
    return querySwapChainSupport(m_physicalDevice);
}

std::vector<const char*> VulkanContext::getRequiredInstanceExtensions() {
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

std::vector<const char*> VulkanContext::getRequiredDeviceExtensions() {
    std::vector<const char*> extensions;

    for (const auto& ext : m_config.requiredDeviceExtensions) {
        extensions.push_back(ext.c_str());
    }

    // 检查可选扩展是否可用
    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    for (const auto& ext : m_config.optionalDeviceExtensions) {
        for (const auto& available : availableExtensions) {
            if (ext == available.extensionName) {
                extensions.push_back(ext.c_str());
                break;
            }
        }
    }

    return extensions;
}

Result<u32> VulkanContext::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const {
    for (u32 i = 0; i < m_memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::NotFound, "Failed to find suitable memory type");
}

Result<VkFormat> VulkanContext::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    return Error(ErrorCode::NotFound, "Failed to find supported format");
}

Result<VkFormat> VulkanContext::findDepthFormat() const {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanContext::waitIdle() const {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*)
{
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            SPDLOG_TRACE("Vulkan: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            SPDLOG_DEBUG("Vulkan: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn("Vulkan: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error("Vulkan: {}", pCallbackData->pMessage);
            break;
        default:
            spdlog::info("Vulkan: {}", pCallbackData->pMessage);
            break;
    }

    return VK_FALSE;
}

} // namespace mr::client
