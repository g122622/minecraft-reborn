#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace mc::client {

// 前向声明
class GuiRenderer;

/**
 * @brief GUI纹理区域
 *
 * 定义GUI纹理图集中的单个纹理区域。
 */
struct GuiTextureRegion {
    f32 u0, v0;     ///< 左上角UV坐标
    f32 u1, v1;     ///< 右下角UV坐标
    i32 width;      ///< 纹理宽度（像素）
    i32 height;     ///< 纹理高度（像素）
};

/**
 * @brief GUI纹理图集
 *
 * 管理所有GUI纹理的图集，包括：
 * - 容器背景纹理
 * - 槽位背景
 * - 图标
 * - 按钮纹理
 *
 * 使用示例：
 * @code
 * GuiTextureAtlas atlas;
 * atlas.initialize(device, physicalDevice, commandPool, graphicsQueue);
 * atlas.loadGuiTexture("minecraft:textures/gui/container/crafting_table");
 *
 * // 渲染
 * atlas.drawTexture(gui, "minecraft:textures/gui/container/crafting_table",
 *                   x, y, 176, 166);
 * @endcode
 */
class GuiTextureAtlas {
public:
    GuiTextureAtlas();
    ~GuiTextureAtlas();

    // 禁止拷贝
    GuiTextureAtlas(const GuiTextureAtlas&) = delete;
    GuiTextureAtlas& operator=(const GuiTextureAtlas&) = delete;

    // 允许移动
    GuiTextureAtlas(GuiTextureAtlas&&) noexcept = default;
    GuiTextureAtlas& operator=(GuiTextureAtlas&&) noexcept = default;

    /**
     * @brief 初始化GUI纹理图集
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
                                           VkPhysicalDevice physicalDevice,
                                           VkCommandPool commandPool,
                                           VkQueue graphicsQueue);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 加载GUI纹理
     * @param location 纹理资源位置
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadGuiTexture(const ResourceLocation& location);

    /**
     * @brief 加载默认GUI纹理
     *
     * 加载内置的GUI纹理：
     * - 容器背景
     * - 槽位背景
     * - 图标
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadDefaultTextures();

    /**
     * @brief 绘制完整纹理
     * @param gui GUI渲染器
     * @param textureId 纹理ID
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param width 绘制宽度
     * @param height 绘制高度
     */
    void drawTexture(GuiRenderer& gui, const String& textureId,
                     f32 x, f32 y, f32 width, f32 height);

    /**
     * @brief 绘制纹理区域
     * @param gui GUI渲染器
     * @param textureId 纹理ID
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param region 纹理区域（像素）
     * @param width 绘制宽度
     * @param height 绘制高度
     */
    void drawTextureRegion(GuiRenderer& gui, const String& textureId,
                           f32 x, f32 y,
                           i32 regionX, i32 regionY, i32 regionWidth, i32 regionHeight,
                           f32 width, f32 height);

    /**
     * @brief 检查纹理是否已加载
     */
    [[nodiscard]] bool hasTexture(const String& textureId) const;

    /**
     * @brief 获取纹理区域
     * @param textureId 纹理ID
     * @return 纹理区域，如果不存在返回nullptr
     */
    [[nodiscard]] const GuiTextureRegion* getRegion(const String& textureId) const;

    /**
     * @brief 获取纹理图集图像视图
     */
    [[nodiscard]] VkImageView imageView() const { return m_imageView; }

    /**
     * @brief 获取纹理图集采样器
     */
    [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 创建默认纹理（用于无纹理资源包时）
     */
    [[nodiscard]] Result<void> createDefaultTextures();

    /**
     * @brief 创建槽位背景纹理
     */
    void createSlotBackground(u8* data, i32 width, i32 height);

    /**
     * @brief 创建容器背景纹理
     */
    void createContainerBackground(u8* data, i32 width, i32 height);

    /**
     * @brief 创建图像
     */
    [[nodiscard]] Result<void> createImage(u32 width, u32 height);

    /**
     * @brief 创建图像视图
     */
    [[nodiscard]] Result<void> createImageView();

    /**
     * @brief 创建采样器
     */
    [[nodiscard]] Result<void> createSampler();

    /**
     * @brief 上传纹理数据
     */
    [[nodiscard]] Result<void> uploadTextureData(const std::vector<u8>& data);

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void endSingleTimeCommands(VkCommandBuffer cmd);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 纹理资源
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // 图集尺寸
    u32 m_width = 0;
    u32 m_height = 0;

    // 纹理区域映射
    std::unordered_map<String, GuiTextureRegion> m_regions;

    // 默认纹理尺寸
    static constexpr i32 DEFAULT_SLOT_SIZE = 18;
    static constexpr i32 DEFAULT_CONTAINER_WIDTH = 176;
    static constexpr i32 DEFAULT_CONTAINER_HEIGHT = 166;

    bool m_initialized = false;
};

} // namespace mc::client
