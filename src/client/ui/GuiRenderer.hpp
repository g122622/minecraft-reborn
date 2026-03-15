#pragma once

#include "Glyph.hpp"
#include "Font.hpp"
#include "FontRenderer.hpp"
#include "../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc::client {

// 前向声明
class Font;

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
    // 物品纹理颜色（alpha=254）：
    // - alpha==255 时片段着色器走字体采样分支
    // - alpha<255 时走物品采样分支
    static constexpr u32 ITEM_TEXTURE_COLOR = 0xFEFFFFFF;

    GuiRenderer();
    ~GuiRenderer();

    // 禁止拷贝
    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer& operator=(const GuiRenderer&) = delete;

    /**
     * @brief 初始化GUI渲染器
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param renderPass 渲染通道
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkRenderPass renderPass);

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

    /**
     * @brief 获取字体对象
     */
    [[nodiscard]] Font* font() const { return m_font; }

    /**
     * @brief 设置字体缩放因子
     * @param scale 缩放因子（默认1.0）
     */
    void setFontScale(f32 scale);

    /**
     * @brief 获取字体缩放因子
     */
    [[nodiscard]] f32 getFontScale() const;

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
     * @brief 绘制纹理矩形（使用物品纹理图集）
     *
     * 绘制物品/图标纹理。UV坐标指定纹理图集中的区域。
     *
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param u0 纹理左上角U
     * @param v0 纹理左上角V
     * @param u1 纹理右下角U
     * @param v1 纹理右下角V
     * @param color 颜色（ARGB，默认白色）
     */
    void drawTexturedRect(f32 x, f32 y, f32 width, f32 height,
                          f32 u0, f32 v0, f32 u1, f32 v1,
                          u32 color = ITEM_TEXTURE_COLOR);

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

    /**
     * @brief 设置物品纹理图集
     *
     * 物品纹理图集用于渲染物品图标。binding=1的纹理采样器。
     *
     * @param itemView 物品纹理图集的图像视图
     * @param itemSampler 物品纹理采样器
     */
    void setItemTextureAtlas(VkImageView itemView, VkSampler itemSampler);

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
     * @brief 初始化纹理布局（在首次使用前调用）
     *
     * 由于纹理创建时布局为 UNDEFINED，需要在首次渲染前转换到 SHADER_READ_ONLY_OPTIMAL
     */
    void initializeTextureLayouts(VkCommandBuffer commandBuffer);

    /**
     * @brief 上传顶点和索引数据到GPU
     */
    void uploadBufferData(VkCommandBuffer commandBuffer);

    // ========== Vulkan 辅助函数 ==========

    /**
     * @brief 查找内存类型
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

    // Vulkan资源
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    // 缓冲区
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_vertexBufferSize = 64 * 1024;  // 64KB 初始大小
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_indexBufferSize = 128 * 1024;  // 128KB 初始大小
    VkBuffer m_fontStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_fontStagingMemory = VK_NULL_HANDLE;

    // 字体纹理
    VkImage m_fontTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_fontTextureMemory = VK_NULL_HANDLE;
    VkImageView m_fontTextureView = VK_NULL_HANDLE;

    // 物品纹理占位符
    VkImage m_itemPlaceholderTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_itemPlaceholderMemory = VK_NULL_HANDLE;
    VkImageView m_itemPlaceholderView = VK_NULL_HANDLE;

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

} // namespace mc::client
