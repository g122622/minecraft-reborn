#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "MeshTypes.hpp"
#include "VulkanContext.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanTexture.hpp"
#include "ChunkRenderer.hpp"
#include "UniformBuffer.hpp"
#include "Descriptor.hpp"
#include "Camera.hpp"
#include "sky/SkyRenderer.hpp"
#include "../ui/Font.hpp"
#include "../ui/GuiRenderer.hpp"
#include "../resource/ResourceManager.hpp"
#include "../resource/ItemTextureAtlas.hpp"
#include "item/ItemRenderer.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>
#include <memory>

// 前置声明
struct GLFWwindow;

namespace mc::client {

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

// GUI渲染回调类型
using GuiRenderCallback = std::function<void()>;

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

    // 区块管理
    ChunkRenderer& chunkRenderer() { return m_chunkRenderer; }
    const ChunkRenderer& chunkRenderer() const { return m_chunkRenderer; }
    bool isChunkRendererInitialized() const { return m_chunkRendererInitialized; }

    /**
     * @brief 从 ResourceManager 更新纹理图集
     * @param atlasResult 纹理图集构建结果
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> updateTextureAtlas(const AtlasBuildResult& atlasResult);

    /**
     * @brief 获取纹理图集的区域信息
     * @param location 纹理位置
     * @return 纹理区域，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getTextureRegion(const ResourceLocation& location) const;

    // 天空渲染
    SkyRenderer& skyRenderer() { return m_skyRenderer; }
    const SkyRenderer& skyRenderer() const { return m_skyRenderer; }
    bool isSkyRendererInitialized() const { return m_skyRendererInitialized; }

    // 时间更新
    /**
     * @brief 更新时间状态 (从服务端同步)
     * @param dayTime 一天内的时间 (0-23999)
     * @param gameTime 游戏总 tick 数
     * @param partialTick 部分 tick (用于插值)
     */
    void updateTime(i64 dayTime, i64 gameTime, f32 partialTick = 0.0f);

    // GUI渲染
    GuiRenderer& guiRenderer() { return *m_guiRenderer; }
    const GuiRenderer& guiRenderer() const { return *m_guiRenderer; }
    Font& font() { return m_font; }
    const Font& font() const { return m_font; }
    bool isGuiRendererInitialized() const { return m_guiRendererInitialized; }
    /**
     * @brief 设置GUI渲染回调
     * 回调会在每帧的GUI渲染阶段被调用，用于绘制自定义GUI元素
     */
    void setGuiRenderCallback(GuiRenderCallback callback) { m_guiRenderCallback = std::move(callback); }

    // 物品渲染
    ItemRenderer& itemRenderer() { return *m_itemRenderer; }
    const ItemRenderer& itemRenderer() const { return *m_itemRenderer; }
    ItemTextureAtlas& itemTextureAtlas() { return m_itemTextureAtlas; }
    const ItemTextureAtlas& itemTextureAtlas() const { return m_itemTextureAtlas; }
    bool isItemTextureAtlasInitialized() const { return m_itemTextureAtlasInitialized; }

    /**
     * @brief 初始化物品渲染器
     *
     * 必须在资源加载完成后调用。
     *
     * @param resourceManager 资源管理器
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initializeItemRenderer(ResourceManager* resourceManager);

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

    // 深度缓冲区
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;

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

    // 区块渲染
    std::unique_ptr<VulkanPipeline> m_chunkPipeline;
    std::vector<VkDescriptorSet> m_chunkDescriptorSets;  // 每帧一个
    VkDescriptorSet m_chunkTextureDescriptorSet = VK_NULL_HANDLE;
    VulkanTextureAtlas m_chunkTextureAtlas;
    ChunkRenderer m_chunkRenderer;
    bool m_chunkRendererInitialized = false;
    std::map<ResourceLocation, TextureRegion> m_textureRegions;  // 纹理位置到UV映射

    // 天空渲染
    SkyRenderer m_skyRenderer;
    bool m_skyRendererInitialized = false;

    // 时间状态
    i64 m_dayTime = 0;
    i64 m_gameTime = 0;
    f32 m_partialTick = 0.0f;

    // GUI渲染
    std::unique_ptr<GuiRenderer> m_guiRenderer;
    Font m_font;
    bool m_guiRendererInitialized = false;

    // 物品纹理图集
    ItemTextureAtlas m_itemTextureAtlas;
    bool m_itemTextureAtlasInitialized = false;

    // 物品渲染器
    std::unique_ptr<ItemRenderer> m_itemRenderer;

    // GUI渲染回调
    GuiRenderCallback m_guiRenderCallback;

    // 创建函数
    [[nodiscard]] Result<void> createRenderPass();
    [[nodiscard]] Result<void> createDepthResources();
    [[nodiscard]] Result<void> createCommandPool();
    [[nodiscard]] Result<void> createCommandBuffers();
    [[nodiscard]] Result<void> createFramebuffers();
    [[nodiscard]] Result<void> createSyncObjects();
    [[nodiscard]] Result<void> createDescriptorSetLayouts();
    [[nodiscard]] Result<void> createPipelineLayout();
    [[nodiscard]] Result<void> createDescriptorPool();
    [[nodiscard]] Result<void> createUniformBuffers();
    [[nodiscard]] Result<void> createChunkPipeline();
    [[nodiscard]] Result<void> createChunkTextureAtlas();
    [[nodiscard]] Result<void> createGuiRenderer();

    void destroyRenderPass();
    void destroyDepthResources();
    void destroyCommandPool();
    void destroyCommandBuffers();
    void destroyFramebuffers();
    void destroySyncObjects();
    void destroyDescriptors();
    void destroyChunkResources();
    void destroyGuiResources();

    // 辅助函数
    [[nodiscard]] Result<void> recreateSwapchain();
    void updateUniformBuffers();

    // 渲染函数
    void renderChunks(VkCommandBuffer cmd);
    void renderGui(VkCommandBuffer cmd);
};

} // namespace mc::client
