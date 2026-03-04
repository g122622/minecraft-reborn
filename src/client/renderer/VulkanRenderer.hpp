#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "VulkanContext.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanPipeline.hpp"
#include "UniformBuffer.hpp"
#include "Descriptor.hpp"
#include "Camera.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

// 前置声明
struct GLFWwindow;

namespace mr::client {

// 渲染器配置
struct RendererConfig {
    VulkanConfig vulkanConfig;
    SwapChainConfig swapChainConfig;
    bool enableValidation = true;
    bool enableVSync = true;
};

// 帧同步对象
struct FrameSync {
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};

// 渲染器
class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    // 禁止拷贝
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    // 生命周期
    [[nodiscard]] Result<void> initialize(GLFWwindow* window, const RendererConfig& config);
    void destroy();

    // 帧渲染
    [[nodiscard]] Result<void> beginFrame();
    [[nodiscard]] Result<void> endFrame();
    [[nodiscard]] Result<void> render();

    // 窗口大小变化
    [[nodiscard]] Result<void> onResize(u32 width, u32 height);

    // 相机设置
    void setCamera(Camera* camera) { m_camera = camera; }
    Camera* camera() { return m_camera; }
    const Camera* camera() const { return m_camera; }

    // 状态
    bool isInitialized() const { return m_initialized; }
    bool isMinimized() const { return m_minimized; }

    // Vulkan对象
    VulkanContext* context() { return m_context.get(); }
    VulkanSwapchain* swapchain() { return m_swapchain.get(); }
    VkRenderPass renderPass() const { return m_renderPass; }
    VkCommandPool commandPool() const { return m_commandPool; }
    VkCommandBuffer currentCommandBuffer() const;
    VkPipelineLayout pipelineLayout() const { return m_pipelineLayout; }

    // 描述符
    VkDescriptorSetLayout cameraDescriptorLayout() const { return m_cameraDescriptorLayout; }
    VkDescriptorSetLayout textureDescriptorLayout() const { return m_textureDescriptorLayout; }
    VkDescriptorPool descriptorPool() { return m_descriptorPool; }

    // 帧信息
    u32 currentFrameIndex() const { return m_currentFrame; }
    u32 currentImageIndex() const { return m_imageIndex; }

    // Uniform缓冲区
    UniformBuffer& cameraUBO() { return m_cameraUBO; }
    UniformBuffer& lightingUBO() { return m_lightingUBO; }

private:
    std::unique_ptr<VulkanContext> m_context;
    std::unique_ptr<VulkanSwapchain> m_swapchain;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkFramebuffer> m_framebuffers;

    // 描述符
    VkDescriptorSetLayout m_cameraDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_textureDescriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // Uniform缓冲区
    UniformBuffer m_cameraUBO;
    UniformBuffer m_lightingUBO;

    // 相机
    Camera* m_camera = nullptr;

    // 同步
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<FrameSync> m_frameSyncs;
    u32 m_currentFrame = 0;
    u32 m_imageIndex = 0;

    // 状态
    bool m_initialized = false;
    bool m_minimized = false;
    bool m_frameStarted = false;
    RendererConfig m_config;

    // 创建函数
    [[nodiscard]] Result<void> createRenderPass();
    [[nodiscard]] Result<void> createCommandPool();
    [[nodiscard]] Result<void> createCommandBuffers();
    [[nodiscard]] Result<void> createFramebuffers();
    [[nodiscard]] Result<void> createSyncObjects();
    [[nodiscard]] Result<void> createDescriptorSetLayouts();
    [[nodiscard]] Result<void> createPipelineLayout();
    [[nodiscard]] Result<void> createDescriptorPool();
    [[nodiscard]] Result<void> createUniformBuffers();

    void destroyRenderPass();
    void destroyCommandPool();
    void destroyCommandBuffers();
    void destroyFramebuffers();
    void destroySyncObjects();
    void destroyDescriptors();

    // 辅助函数
    [[nodiscard]] Result<void> recreateSwapchain();
    void updateUniformBuffers();
};

} // namespace mr::client
