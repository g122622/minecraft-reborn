#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/core/Result.hpp"
#include "../../../common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {

class Item;
class ItemStack;
struct TextureRegion;
class ResourceManager;

namespace client {

class GuiRenderer;
class ItemTextureAtlas;

/**
 * @brief 物品渲染器
 *
 * 负责在GUI中渲染物品图标。
 *
 * 纹理来源：
 * - 方块物品：使用方块纹理图集（ResourceManager.getTextureRegion()）
 * - 非方块物品：使用物品纹理图集（ItemTextureAtlas）
 *
 * 用法：
 * @code
 * ItemRenderer itemRenderer;
 * itemRenderer.initialize(context, resourceManager, itemTextureAtlas);
 *
 * // 在GUI渲染中
 * itemRenderer.renderItem(gui, itemStack, x, y);
 * @endcode
 */
class ItemRenderer {
public:
    ItemRenderer();
    ~ItemRenderer() = default;

    // 禁止拷贝
    ItemRenderer(const ItemRenderer&) = delete;
    ItemRenderer& operator=(const ItemRenderer&) = delete;

    /**
     * @brief 初始化物品渲染器
     *
     * @param context Vulkan上下文
     * @param resourceManager 资源管理器（用于获取方块纹理）
     * @param itemTextureAtlas 物品纹理图集（用于非方块物品）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        ResourceManager* resourceManager,
        ItemTextureAtlas* itemTextureAtlas);

    /**
     * @brief 渲染物品图标
     *
     * @param gui GUI渲染器
     * @param stack 物品堆
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param size 渲染尺寸（默认16像素）
     */
    void renderItem(GuiRenderer& gui, const ItemStack& stack, f32 x, f32 y, f32 size = 16.0f);

    /**
     * @brief 渲染物品图标（无数量显示）
     *
     * @param gui GUI渲染器
     * @param item 物品指针
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param size 渲染尺寸（默认16像素）
     */
    void renderItem(GuiRenderer& gui, const Item* item, f32 x, f32 y, f32 size = 16.0f);

    /**
     * @brief 渲染物品图标（使用纹理区域）
     *
     * 直接使用纹理区域渲染，用于自定义纹理。
     *
     * @param gui GUI渲染器
     * @param region 纹理区域
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param size 渲染尺寸（默认16像素）
     */
    void renderItem(GuiRenderer& gui, const TextureRegion& region, f32 x, f32 y, f32 size = 16.0f);

    /**
     * @brief 检查物品是否为方块物品
     *
     * 方块物品使用方块纹理图集，非方块物品使用物品纹理图集。
     *
     * @param item 物品指针
     * @return 是否为方块物品
     */
    [[nodiscard]] bool isBlockItem(const Item* item) const;

    /**
     * @brief 获取物品的纹理区域
     *
     * @param item 物品指针
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTextureRegion(const Item* item) const;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    ResourceManager* m_resourceManager = nullptr;
    ItemTextureAtlas* m_itemTextureAtlas = nullptr;
    bool m_initialized = false;
};

} // namespace client
} // namespace mc
