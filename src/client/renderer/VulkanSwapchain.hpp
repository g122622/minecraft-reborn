#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "VulkanContext.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace mc::client {

// 交换链配置
struct SwapChainConfig {
    u32 width = 800;
    u32 height = 600;
    u32 imageCount = 3;         // 三缓冲
    bool vsync = true;
    VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
};

// 交换链
class VulkanSwapchain {
public:
    VulkanSwapchain();
    ~VulkanSwapchain();

    // 禁止拷贝
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    // 初始化
    [[nodiscard]] Result<void> initialize(VulkanContext* context, const SwapChainConfig& config);
    void destroy();

    // 重建交换链 (窗口大小变化时)
    [[nodiscard]] Result<void> recreate(u32 width, u32 height);

    // 获取下一帧图像
    [[nodiscard]] Result<u32> acquireNextImage(VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);

    // 呈现图像
    [[nodiscard]] Result<void> present(u32 imageIndex, VkSemaphore waitSemaphore);

    // 信息
    VkSwapchainKHR swapchain() const { return m_swapchain; }
    VkFormat imageFormat() const { return m_imageFormat; }
    VkExtent2D extent() const { return m_extent; }
    const std::vector<VkImage>& images() const { return m_images; }
    const std::vector<VkImageView>& imageViews() const { return m_imageViews; }
    u32 imageCount() const { return static_cast<u32>(m_images.size()); }

private:
    VulkanContext* m_context = nullptr;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D m_extent = {0, 0};

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    SwapChainConfig m_config;
    bool m_initialized = false;

    // 创建函数
    [[nodiscard]] Result<void> createSwapchain();
    void destroySwapchain();

    // 辅助函数
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availableModes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    [[nodiscard]] Result<void> createImageViews();
};

} // namespace mc::client
