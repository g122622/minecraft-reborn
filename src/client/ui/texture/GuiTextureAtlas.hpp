#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace mc::client {

// 前向声明
class VulkanContext;
class VulkanTexture;
class VulkanBuffer;
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
 * atlas.initialize(context);
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
     * @param context Vulkan上下文
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VulkanContext* context);

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
     * @brief 获取纹理图集
     */
    [[nodiscard]] VulkanTexture* getAtlasTexture() { return m_atlasTexture.get(); }
    [[nodiscard]] const VulkanTexture* getAtlasTexture() const { return m_atlasTexture.get(); }

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

private:
    VulkanContext* m_context = nullptr;
    std::unique_ptr<VulkanTexture> m_atlasTexture;
    std::unordered_map<String, GuiTextureRegion> m_regions;

    // 默认纹理尺寸
    static constexpr i32 DEFAULT_SLOT_SIZE = 18;
    static constexpr i32 DEFAULT_CONTAINER_WIDTH = 176;
    static constexpr i32 DEFAULT_CONTAINER_HEIGHT = 166;

    bool m_initialized = false;
};

} // namespace mc::client
