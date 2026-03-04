#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "../../common/renderer/MeshTypes.hpp"
#include "../../common/world/ChunkData.hpp"
#include "VulkanContext.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanTexture.hpp"
#include "ChunkRenderer.hpp"
#include "DefaultTextureAtlas.hpp"
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

// 测试三角形顶点
struct TestVertex {
    float pos[3];
    float normal[3];
    float texCoord[2];
    float color[4];
    float light;
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
    // 参考: https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;  // 每帧一个，用m_currentFrame索引
    std::vector<VkSemaphore> m_renderFinishedSemaphores;  // 每个交换链图像一个，用m_imageIndex索引
    std::vector<VkFence> m_inFlightFences;                // 每帧一个，用m_currentFrame索引
    std::vector<VkFence> m_imageFences;                   // 每个交换链图像一个，追踪当前使用的fence
    u32 m_currentFrame = 0;
    u32 m_imageIndex = 0;

    // 状态
    bool m_initialized = false;
    bool m_minimized = false;
    bool m_frameStarted = false;
    RendererConfig m_config;

    // 测试三角形
    std::unique_ptr<VulkanPipeline> m_testPipeline;
    VulkanBuffer m_testVertexBuffer;
    VulkanBuffer m_testIndexBuffer;
    std::vector<VkDescriptorSet> m_testDescriptorSets;  // 每帧一个
    u32 m_testIndexCount = 0;

    // 测试纹理
    VulkanTexture m_testTexture;
    VkDescriptorSetLayout m_textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_testTextureDescriptorSet = VK_NULL_HANDLE;

    // 区块渲染
    std::unique_ptr<VulkanPipeline> m_chunkPipeline;
    std::vector<VkDescriptorSet> m_chunkDescriptorSets;  // 每帧一个
    VkDescriptorSet m_chunkTextureDescriptorSet = VK_NULL_HANDLE;
    VulkanTextureAtlas m_chunkTextureAtlas;
    ChunkRenderer m_chunkRenderer;
    bool m_chunkRendererInitialized = false;

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
    [[nodiscard]] Result<void> createTestPipeline();
    [[nodiscard]] Result<void> createTestTriangle();
    [[nodiscard]] Result<void> createTestTexture();
    [[nodiscard]] Result<void> createChunkPipeline();
    [[nodiscard]] Result<void> createChunkTextureAtlas();

    void destroyRenderPass();
    void destroyCommandPool();
    void destroyCommandBuffers();
    void destroyFramebuffers();
    void destroySyncObjects();
    void destroyDescriptors();
    void destroyTestResources();
    void destroyChunkResources();

    // 辅助函数
    [[nodiscard]] Result<void> recreateSwapchain();
    void updateUniformBuffers();

    // 渲染函数
    void renderChunks(VkCommandBuffer cmd);
    void renderTestTriangle(VkCommandBuffer cmd);

    // 测试区块
    [[nodiscard]] Result<void> createTestChunk();
    void generateChunkMesh(const ChunkData& chunk, MeshData& outMesh);
    void addBlockFace(MeshData& mesh, BlockId blockId, Face face, f32 x, f32 y, f32 z);
    TextureRegion getBlockTexture(BlockId blockId, Face face);
};

} // namespace mr::client
