#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>

namespace mc::client::renderer::trident::cloud {

/**
 * @brief 云渲染模式
 *
 * 参考 MC 1.16.5 CloudOption
 * - Off: 关闭云渲染
 * - Fast: 快速模式，只渲染底面（单层平面）
 * - Fancy: 精致模式，渲染完整 3D 立方体
 */
enum class CloudMode : u8 {
    Off = 0,    ///< 关闭云渲染
    Fast = 1,   ///< 快速模式（单层平面）
    Fancy = 2   ///< 精致模式（3D 立方体）
};

/**
 * @brief 云 Uniform 缓冲区数据结构
 *
 * 传递给云着色器的 Uniform 数据。
 */
struct CloudUBO {
    alignas(16) glm::vec4 cloudColor;       ///< 云颜色 (RGBA)
    alignas(4)  f32 cloudHeight;            ///< 云高度 (192.0f 主世界)
    alignas(4)  f32 time;                   ///< 时间（用于动画）
    alignas(4)  f32 textureScale;           ///< 纹理缩放因子
    alignas(4)  f32 cameraY;                ///< 相机 Y 坐标（用于高度雾）
};

/**
 * @brief 云渲染器
 *
 * 负责渲染天空中的云层。
 * 参考 MC 1.16.5 WorldRenderer.renderClouds() 实现。
 *
 * 渲染模式：
 * - Fast: 只渲染云底面，性能优化
 * - Fancy: 渲染完整 3D 云体，包含顶部、底部和四个侧面
 *
 * 云特性：
 * - 云高度：主世界 192 格，下界和末地无云
 * - 云随时间缓慢移动（速度 0.03F/tick）
 * - 云颜色随时间和天气变化
 */
class CloudRenderer {
public:
    CloudRenderer();
    ~CloudRenderer();

    // 禁止拷贝
    CloudRenderer(const CloudRenderer&) = delete;
    CloudRenderer& operator=(const CloudRenderer&) = delete;

    // 允许移动
    CloudRenderer(CloudRenderer&& other) noexcept;
    CloudRenderer& operator=(CloudRenderer&& other) noexcept;

    // ========================================================================
    // 初始化与销毁
    // ========================================================================

    /**
     * @brief 初始化云渲染器
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param renderPass 渲染通道
     * @param extent 交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkExtent2D extent);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 窗口大小变化时重新创建资源
     * @param extent 新的交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> onResize(VkExtent2D extent);

    // ========================================================================
    // 更新与渲染
    // ========================================================================

    /**
     * @brief 更新云状态
     *
     * 计算云的位置偏移和颜色。
     *
     * @param dayTime 当前一天内时间 (0-23999)
     * @param gameTime 游戏总 tick 数
     * @param partialTick 部分 tick（用于插值）
     * @param cloudHeight 云高度（维度相关，NaN 表示无云）
     * @param cloudColor 云颜色（来自 SkyRenderer）
     */
    void update(i64 dayTime, i64 gameTime, f32 partialTick,
                f32 cloudHeight, const glm::vec4& cloudColor);

    /**
     * @brief 渲染云
     *
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置
     * @param mode 云渲染模式
     * @param frameIndex 当前帧索引
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                CloudMode mode,
                u32 frameIndex);

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 设置云渲染模式
     */
    void setCloudMode(CloudMode mode) { m_cloudMode = mode; }

    /**
     * @brief 获取当前云渲染模式
     */
    [[nodiscard]] CloudMode cloudMode() const { return m_cloudMode; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取当前云颜色
     */
    [[nodiscard]] const glm::vec4& cloudColor() const { return m_cloudColor; }

private:
    // ========================================================================
    // 资源创建
    // ========================================================================

    /**
     * @brief 创建云顶点缓冲区
     *
     * 生成 Fast 和 Fancy 两种模式的顶点数据。
     * 云网格覆盖 -3 到 +4 区块范围（每个区块 8 格）。
     */
    [[nodiscard]] Result<void> createCloudVBO();

    /**
     * @brief 创建云纹理
     *
     * 使用程序化生成或加载纹理文件。
     * 云纹理为 256x256 灰度透明度图。
     */
    [[nodiscard]] Result<void> createTexture();

    /**
     * @brief 创建 Uniform 缓冲区
     *
     * 为每帧创建一个 Uniform 缓冲区。
     */
    [[nodiscard]] Result<void> createUniformBuffers();

    /**
     * @brief 创建描述符集布局
     */
    [[nodiscard]] Result<void> createDescriptorSetLayout();

    /**
     * @brief 创建描述符池和描述符集
     */
    [[nodiscard]] Result<void> createDescriptorSets();

    /**
     * @brief 创建管线布局
     */
    [[nodiscard]] Result<void> createPipelineLayout();

    /**
     * @brief 创建图形管线
     *
     * 创建 Fast 和 Fancy 两种模式的管线。
     */
    [[nodiscard]] Result<void> createPipelines();

    /**
     * @brief 更新 Uniform 缓冲区
     * @param frameIndex 当前帧索引
     */
    void updateUniformBuffer(u32 frameIndex);

    /**
     * @brief 更新云网格
     *
     * 当云模式改变时，重新生成顶点数据。
     *
     * @param mode 云渲染模式
     */
    void updateCloudMesh(CloudMode mode);

    // ========================================================================
    // Vulkan 辅助函数
    // ========================================================================

    /**
     * @brief 查找合适的内存类型
     */
    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] Result<void> createBuffer(VkDeviceSize size,
                                            VkBufferUsageFlags usage,
                                            VkMemoryPropertyFlags properties,
                                            VkBuffer& buffer,
                                            VkDeviceMemory& memory);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    /**
     * @brief 生成程序化云纹理数据
     *
     * 使用 Perlin 噪声生成云图案。
     *
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 纹理像素数据（RGBA）
     */
    [[nodiscard]] std::vector<u8> generateCloudTexture(u32 width, u32 height);

    // ========================================================================
    // 渲染方法
    // ========================================================================

    /**
     * @brief 渲染 Fast 模式云
     */
    void renderFast(VkCommandBuffer cmd);

    /**
     * @brief 渲染 Fancy 模式云
     */
    void renderFancy(VkCommandBuffer cmd);

private:
    // Vulkan 设备
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;

    // 描述符
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // 管线
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_fastPipeline = VK_NULL_HANDLE;   // Fast 模式管线
    VkPipeline m_fancyPipeline = VK_NULL_HANDLE;  // Fancy 模式管线

    // 顶点缓冲区
    VkBuffer m_fastVBO = VK_NULL_HANDLE;          // Fast 模式顶点缓冲区
    VkDeviceMemory m_fastVBOMemory = VK_NULL_HANDLE;
    u32 m_fastVertexCount = 0;

    VkBuffer m_fancyVBO = VK_NULL_HANDLE;         // Fancy 模式顶点缓冲区
    VkDeviceMemory m_fancyVBOMemory = VK_NULL_HANDLE;
    u32 m_fancyVertexCount = 0;

    // 纹理
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    // Uniform 缓冲区（每帧一个）
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    VkBuffer m_uniformBuffers[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    void* m_uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT] = {nullptr, nullptr};
    VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    // 云状态
    CloudMode m_cloudMode = CloudMode::Fancy;
    i64 m_dayTime = 0;
    i64 m_gameTime = 0;
    f32 m_partialTick = 0.0f;
    f32 m_cloudHeight = 192.0f;    // 主世界云高度
    glm::vec4 m_cloudColor = glm::vec4(1.0f);
    glm::vec3 m_cameraPos = glm::vec3(0.0f);
    f32 m_cloudOffsetX = 0.0f;     // 云 X 偏移（用于动画）
    f32 m_cloudOffsetZ = 0.0f;     // 云 Z 偏移（用于动画）

    // 云网格是否需要更新
    bool m_cloudMeshDirty = true;
};

} // namespace mc::client::renderer::trident::cloud
