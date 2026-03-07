#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/core/Result.hpp"
#include "../VulkanContext.hpp"
#include "CelestialCalculations.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>

namespace mr::client {

// 前向声明
class VulkanBuffer;
class Camera;

/**
 * @brief 天空 Uniform 缓冲区数据结构
 */
struct SkyUBO {
    alignas(16) glm::vec4 skyColor;         // 天空颜色
    alignas(16) glm::vec4 fogColor;         // 雾颜色
    alignas(4)  float celestialAngle;       // 天体角度 (0.0-1.0)
    alignas(4)  float starBrightness;       // 星星亮度
    alignas(4)  int32_t moonPhase;          // 月相 (0-7)
    alignas(4)  float padding;
};

/**
 * @brief 天空渲染器
 *
 * 负责渲染天空、太阳、月亮和星星。
 * 参考 MC 1.16.5 WorldRenderer.renderSky()
 */
class SkyRenderer {
public:
    SkyRenderer();
    ~SkyRenderer();

    // 禁止拷贝
    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    /**
     * @brief 初始化天空渲染器
     * @param ctx Vulkan 上下文
     * @param renderPass 渲染通道
     * @param extent 交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VulkanContext* ctx, VkRenderPass renderPass, VkExtent2D extent);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 窗口大小变化时重新创建资源
     * @param extent 新的交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> onResize(VkExtent2D extent);

    /**
     * @brief 更新天空状态
     * @param dayTime 当前一天内的时间 (0-23999)
     * @param gameTime 游戏总 tick 数
     * @param partialTick 部分 tick (用于插值)
     * @param rainStrength 雨强度 (0.0-1.0)
     * @param thunderStrength 雷暴强度 (0.0-1.0)
     */
    void update(i64 dayTime, i64 gameTime, f32 partialTick,
                f32 rainStrength = 0.0f, f32 thunderStrength = 0.0f);

    /**
     * @brief 渲染天空
     * @param cmd 命令缓冲区
     * @param viewProjection 相机视图投影矩阵
     * @param cameraPos 相机位置 (用于雾效果)
     */
    void render(VkCommandBuffer cmd, const glm::mat4& viewProjection, const glm::vec3& cameraPos, u32 frameIndex);

    // ========== 状态查询 ==========

    /**
     * @brief 获取当前天空颜色
     */
    [[nodiscard]] const glm::vec4& skyColor() const { return m_skyColor; }

    /**
     * @brief 获取当前雾颜色
     */
    [[nodiscard]] const glm::vec4& fogColor() const { return m_fogColor; }

    /**
     * @brief 获取太阳方向
     */
    [[nodiscard]] const glm::vec3& sunDirection() const { return m_sunDirection; }

    /**
     * @brief 获取太阳强度
     */
    [[nodiscard]] f32 sunIntensity() const { return m_sunIntensity; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    // ========== 资源创建 ==========

    /**
     * @brief 创建天空穹顶 VBO
     */
    [[nodiscard]] Result<void> createSkyDomeVBO();

    /**
     * @brief 创建星星 VBO
     *
     * 星星位置使用固定种子 (10842L) 生成，保证一致性
     * 约 1500 颗星星分布在单位球面上
     */
    [[nodiscard]] Result<void> createStarVBO();

    /**
     * @brief 创建太阳/月亮 VBO
     */
    [[nodiscard]] Result<void> createSunMoonVBO();

    /**
     * @brief 创建 Uniform 缓冲区
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
     */
    [[nodiscard]] Result<void> createPipelines();

    /**
     * @brief 更新 Uniform 缓冲区
     * @param frameIndex 当前帧索引
     */
    void updateUniformBuffer(u32 frameIndex);

    // ========== 渲染方法 ==========

    /**
     * @brief 渲染天空穹顶
     */
    void renderSkyDome(VkCommandBuffer cmd);

    /**
     * @brief 渲染太阳
     */
    void renderSun(VkCommandBuffer cmd);

    /**
     * @brief 渲染月亮
     */
    void renderMoon(VkCommandBuffer cmd);

    /**
     * @brief 渲染星星
     */
    void renderStars(VkCommandBuffer cmd);

private:
    VulkanContext* m_ctx = nullptr;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;

    // Vulkan 资源
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    // 管线
    VkPipeline m_skyPipeline = VK_NULL_HANDLE;       // 天空穹顶管线
    VkPipeline m_skyStarPipeline = VK_NULL_HANDLE;   // 天空+星星管线 (夜晚)
    VkPipeline m_sunPipeline = VK_NULL_HANDLE;       // 太阳管线
    VkPipeline m_moonPipeline = VK_NULL_HANDLE;      // 月亮管线
    VkPipeline m_starPipeline = VK_NULL_HANDLE;      // 星星管线

    // 顶点缓冲区
    std::unique_ptr<VulkanBuffer> m_skyDomeVBO;      // 天空穹顶
    std::unique_ptr<VulkanBuffer> m_skyDomeIBO;      // 天空穹顶索引
    std::unique_ptr<VulkanBuffer> m_starVBO;         // 星星
    std::unique_ptr<VulkanBuffer> m_sunMoonVBO;      // 太阳/月亮 (共享)

    u32 m_skyDomeIndexCount = 0;
    u32 m_starVertexCount = 0;

    // Uniform 缓冲区 (每帧一个)
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::unique_ptr<VulkanBuffer> m_uniformBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    u32 m_currentFrame = 0;

    // 天空状态
    i64 m_dayTime = 0;
    i64 m_gameTime = 0;
    f32 m_celestialAngle = 0.0f;
    i32 m_moonPhase = 0;
    f32 m_starBrightness = 0.0f;
    glm::vec4 m_skyColor = glm::vec4(0.53f, 0.81f, 0.92f, 1.0f);
    glm::vec4 m_fogColor = glm::vec4(0.7f, 0.75f, 0.8f, 1.0f);
    glm::vec3 m_sunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    f32 m_sunIntensity = 1.0f;
    f32 m_rainStrength = 0.0f;
    f32 m_thunderStrength = 0.0f;
};

} // namespace mr::client
