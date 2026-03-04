#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace mr::client {

// Vulkan版本
struct VulkanVersion {
    u32 major;
    u32 minor;
    u32 patch;

    String toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

// 队列族索引
struct QueueFamilyIndices {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    std::optional<u32> transferFamily;
    std::optional<u32> computeFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    bool hasTransfer() const {
        return transferFamily.has_value();
    }

    bool hasCompute() const {
        return computeFamily.has_value();
    }
};

// 交换链支持详情
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Vulkan配置
struct VulkanConfig {
    String appName = "Minecraft Reborn";
    String engineName = "Minecraft Engine";
    VulkanVersion appVersion{1, 0, 0};
    VulkanVersion engineVersion{1, 0, 0};
    VulkanVersion apiVersion{1, 3, 0};

    bool enableValidation = true;
    bool enableRenderDoc = false;
    std::vector<String> requiredDeviceExtensions;
    std::vector<String> optionalDeviceExtensions;
    std::vector<String> requiredLayers;
    std::vector<String> optionalLayers;

    VulkanConfig() {
        // 默认必需的设备扩展
        requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // 默认可选的设备扩展
        optionalDeviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

        // 调试层
        #ifdef _DEBUG
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
        #endif
    }
};

// Vulkan上下文
class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // 禁止拷贝
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    // 初始化（完整流程）
    [[nodiscard]] Result<void> initialize(const VulkanConfig& config, VkSurfaceKHR surface);

    // 分阶段初始化
    // 第一步：创建instance
    [[nodiscard]] Result<void> createInstanceOnly(const VulkanConfig& config);
    // 第二步：设置surface
    void setSurface(VkSurfaceKHR surface);
    // 第三步：创建设备
    [[nodiscard]] Result<void> createDevice();

    void destroy();

    // 实例和设备
    VkInstance instance() const { return m_instance; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkDevice device() const { return m_device; }
    VkSurfaceKHR surface() const { return m_surface; }

    // 队列
    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    VkQueue presentQueue() const { return m_presentQueue; }
    VkQueue transferQueue() const { return m_transferQueue; }
    VkQueue computeQueue() const { return m_computeQueue; }

    // 队列族索引
    const QueueFamilyIndices& queueFamilies() const { return m_queueFamilies; }

    // 设备属性
    const VkPhysicalDeviceProperties& deviceProperties() const { return m_deviceProperties; }
    const VkPhysicalDeviceFeatures& deviceFeatures() const { return m_deviceFeatures; }
    const VkPhysicalDeviceMemoryProperties& memoryProperties() const { return m_memoryProperties; }

    // 交换链支持
    SwapChainSupportDetails querySwapChainSupport() const;

    // 工具函数
    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] Result<VkFormat> findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) const;
    [[nodiscard]] Result<VkFormat> findDepthFormat() const;

    // 等待设备空闲
    void waitIdle() const;

    // 调试
    bool isValidationEnabled() const { return m_validationEnabled; }
    VkDebugUtilsMessengerEXT debugMessenger() const { return m_debugMessenger; }

private:
    // Vulkan对象
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    // 队列
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;

    // 队列族索引
    QueueFamilyIndices m_queueFamilies;

    // 设备属性
    VkPhysicalDeviceProperties m_deviceProperties{};
    VkPhysicalDeviceFeatures m_deviceFeatures{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};

    // 配置
    VulkanConfig m_config;
    bool m_validationEnabled = false;
    bool m_initialized = false;

    // 创建函数
    [[nodiscard]] Result<void> createInstance();
    [[nodiscard]] Result<void> setupDebugMessenger();
    [[nodiscard]] Result<void> pickPhysicalDevice();
    [[nodiscard]] Result<void> createLogicalDevice();

    // 辅助函数
    bool checkValidationLayerSupport();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
    std::vector<const char*> getRequiredInstanceExtensions();
    std::vector<const char*> getRequiredDeviceExtensions();

    // 调试回调
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

} // namespace mr::client
