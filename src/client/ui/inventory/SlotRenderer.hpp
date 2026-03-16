#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/item/ItemStack.hpp"
#include <memory>

namespace mc {

// Forward declarations
class ItemStack;

namespace client::renderer::trident::gui {
class GuiRenderer;
}

namespace client::renderer::trident::item {
class ItemRenderer;
}

namespace client {
// 引入GuiRenderer和ItemRenderer到mc::client命名空间（向后兼容）
using GuiRenderer = renderer::trident::gui::GuiRenderer;
using ItemRenderer = renderer::trident::item::ItemRenderer;

/**
 * @brief 槽位渲染器
 *
 * 负责渲染背包槽位和物品图标。
 *
 * 参考: net.minecraft.client.gui.AbstractGui (renderItemAndEffectIntoGUI)
 */
class SlotRenderer {
public:
    SlotRenderer();
    ~SlotRenderer() = default;

    // 禁止拷贝
    SlotRenderer(const SlotRenderer&) = delete;
    SlotRenderer& operator=(const SlotRenderer&) = delete;

    /**
     * @brief 初始化槽位渲染器
     * @param itemRenderer 物品渲染器
     * @return 成功或错误
     */
    [[nodiscard]] bool initialize(ItemRenderer* itemRenderer);

    /**
     * @brief 渲染槽位
     * @param gui GUI渲染器
     * @param x 槽位X位置
     * @param y 槽位Y位置
     * @param stack 物品堆
     * @param selected 是否被选中
     */
    void renderSlot(GuiRenderer& gui, f32 x, f32 y, const ItemStack& stack, bool selected = false);

    /**
     * @brief 渲染物品图标
     * @param gui GUI渲染器
     * @param x 图标X位置
     * @param y 图标Y位置
     * @param stack 物品堆
     */
    void renderItem(GuiRenderer& gui, f32 x, f32 y, const ItemStack& stack);

    /**
     * @brief 渲染物品数量
     * @param gui GUI渲染器
     * @param x 数量X位置
     * @param y 数量Y位置
     * @param count 数量
     */
    void renderCount(GuiRenderer& gui, f32 x, f32 y, i32 count);

    /**
     * @brief 渲染耐久度条
     * @param gui GUI渲染器
     * @param x 耐久度条X位置
     * @param y 耐久度条Y位置
     * @param stack 物品堆
     */
    void renderDurabilityBar(GuiRenderer& gui, f32 x, f32 y, const ItemStack& stack);

    // 槽位尺寸常量
    static constexpr f32 SLOT_SIZE = 18.0f;
    static constexpr f32 ITEM_SIZE = 16.0f;

private:
    ItemRenderer* m_itemRenderer = nullptr;
    bool m_initialized = false;
};

} // namespace client
} // namespace mc
