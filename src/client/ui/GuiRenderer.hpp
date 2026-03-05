#pragma once

#include "Glyph.hpp"
#include "Font.hpp"
#include "FontRenderer.hpp"
#include "../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mr::client {

// 前向声明
class VulkanContext;
class VulkanBuffer;
class VulkanPipeline;
class VulkanTexture;

/**
 * @brief GUI渲染器
 *
 * 负责在屏幕上渲染2D GUI元素，包括：
 * - 文本
 * - 矩形
 * - 纹理
 *
 * 使用正交投影渲染到屏幕空间。
 */
class GuiRenderer {
public:
    GuiRenderer();
    ~GuiRenderer();

    // 禁止拷贝
    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer& operator=(const GuiRenderer&) = delete;

    /**
     * @brief 初始化GUI渲染器
     * @param context Vulkan上下文
     * @param renderPass 渲染通道
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VulkanContext* context, VkRenderPass renderPass);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 设置字体
     * @param font 字体对象
     */
    void setFont(Font* font);

    /**
     * @brief 开始帧
     * @param screenW 屏幕宽度
     * @param screenH 屏幕高度
     */
    void beginFrame(f32 screenW, f32 screenH);

    /**
     * @brief 准备帧数据（在渲染通道外调用）
     * 用于更新字体纹理等需要在渲染通道外执行的操作
     * @param commandBuffer 命令缓冲区
     */
    void prepareFrame(VkCommandBuffer commandBuffer);

    /**
     * @brief 渲染GUI（在渲染通道内调用）
     * 绘制所有累积的元素，并上传顶点/索引数据
     * @param commandBuffer 命令缓冲区
     */
    void render(VkCommandBuffer commandBuffer);

    // ==================== 文本绘制 ====================
    /**
     * @brief 绘制文本
     * @param text 文本内容（UTF-8）
     * @param x X坐标
     * @param y Y坐标
     * @param color 颜色（ARGB）
     * @param shadow 是否绘制阴影
     * @return 文本宽度
     */
    f32 drawText(const std::string& text, f32 x, f32 y,
                 u32 color = Colors::WHITE, bool shadow = true);

    /**
     * @brief 绘制居中文本
     * @param text 文本内容
     * @param x 中心X坐标
     * @param y Y坐标
     * @param color 颜色
     * @return 文本宽度
     */
    f32 drawTextCentered(const std::string& text, f32 x, f32 y,
                         u32 color = Colors::WHITE);

    /**
     * @brief 获取文本宽度
     */
    [[nodiscard]] f32 getTextWidth(const std::string& text);

    /**
     * @brief 获取字体高度
     */
    [[nodiscard]] u32 getFontHeight() const;

    // ==================== 矩形绘制 ====================

    /**
     * @brief 绘制填充矩形
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param color 颜色（ARGB）
     */
    void fillRect(f32 x, f32 y, f32 width, f32 height, u32 color);

    /**
     * @brief 绘制渐变矩形
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param colorTop 顶部颜色
     * @param colorBottom 底部颜色
     */
    void fillGradientRect(f32 x, f32 y, f32 width, f32 height,
                          u32 colorTop, u32 colorBottom);

    /**
     * @brief 绘制矩形边框
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param color 颜色
     */
    void drawRect(f32 x, f32 y, f32 width, f32 height, u32 color);

    // ==================== 获取器 ====================

    /**
     * @brief 获取屏幕宽度
     */
    [[nodiscard]] f32 screenWidth() const { return m_screenWidth; }

    /**
     * @brief 获取屏幕高度
     */
    [[nodiscard]] f32 screenHeight() const { return m_screenHeight; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 创建管线布局
     */
    [[nodiscard]] Result<void> createPipelineLayout();

    /**
     * @brief 创建图形管线
     */
    [[nodiscard]] Result<void> createPipeline(VkRenderPass renderPass);

    /**
     * @brief 创建描述符
     */
    [[nodiscard]] Result<void> createDescriptors();

    /**
     * @brief 创建顶点缓冲和索引缓冲
     */
    [[nodiscard]] Result<void> createBuffers();

    /**
     * @brief 创建字体纹理
     */
    [[nodiscard]] Result<void> createFontTexture();

    /**
     * @brief 更新字体纹理
     */
    void updateFontTexture(VkCommandBuffer commandBuffer);

    /**
     * @brief 上传顶点和索引数据到GPU
     */
    void uploadBufferData(VkCommandBuffer commandBuffer);

    // Vulkan资源
    VulkanContext* m_context = nullptr;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    // 缓冲区
    std::unique_ptr<VulkanBuffer> m_vertexBuffer;
    std::unique_ptr<VulkanBuffer> m_indexBuffer;
    std::unique_ptr<VulkanBuffer> m_fontStagingBuffer;

    // 纹理
    std::unique_ptr<VulkanTexture> m_fontTexture;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // 字体
    Font* m_font = nullptr;
    FontRenderer m_fontRenderer;

    // 屏幕尺寸
    f32 m_screenWidth = 0.0f;
    f32 m_screenHeight = 0.0f;

    // 顶点/索引数据
    std::vector<GuiVertex> m_vertices;
    std::vector<u32> m_indices;
    bool m_needsTextureUpdate = false;

    // 状态
    bool m_initialized = false;
    bool m_inFrame = false;
};

} // namespace mr::client
