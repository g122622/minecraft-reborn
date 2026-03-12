#include "SlotRenderer.hpp"
#include "../GuiRenderer.hpp"
#include "../../../common/item/ItemStack.hpp"
#include "../../../common/item/Item.hpp"

namespace mc {
namespace client {

// ============================================================================
// 颜色常量
// ============================================================================

namespace SlotColors {
    constexpr u32 SLOT_BACKGROUND = 0xFF8B8B8B;      // 槽位背景
    constexpr u32 SLOT_BORDER = 0xFF373737;          // 槽位边框
    constexpr u32 SLOT_HIGHLIGHT = 0xFFFFFFFF;       // 选中高亮
    constexpr u32 COUNT_TEXT = 0xFFFFFFFF;           // 数量文字
    constexpr u32 DURABILITY_BG = 0xFF000000;        // 耐久度条背景
}

// ============================================================================
// 构造函数
// ============================================================================

SlotRenderer::SlotRenderer()
{
}

// ============================================================================
// 初始化
// ============================================================================

bool SlotRenderer::initialize() {
    if (m_initialized) {
        return true;
    }

    m_initialized = true;
    return true;
}

// ============================================================================
// 渲染
// ============================================================================

void SlotRenderer::renderSlot(GuiRenderer& gui, f32 x, f32 y,
                               const ItemStack& stack, bool selected) {
    // 绘制槽位背景
    gui.fillRect(x, y, SLOT_SIZE, SLOT_SIZE, SlotColors::SLOT_BACKGROUND);
    gui.drawRect(x, y, SLOT_SIZE, SLOT_SIZE, SlotColors::SLOT_BORDER);

    // 如果选中，绘制高亮边框
    if (selected) {
        gui.drawRect(x - 1, y - 1, SLOT_SIZE + 2, SLOT_SIZE + 2,
                     SlotColors::SLOT_HIGHLIGHT);
    }

    // 如果有物品，绘制物品
    if (!stack.isEmpty()) {
        renderItem(gui, x + 1, y + 1, stack);

        // 绘制数量
        if (stack.getCount() > 1) {
            renderCount(gui, x + ITEM_SIZE - 1, y + ITEM_SIZE - 8, stack.getCount());
        }

        // 绘制耐久度条
        if (stack.isDamaged()) {
            renderDurabilityBar(gui, x + 1, y + ITEM_SIZE - 2, stack);
        }
    }
}

void SlotRenderer::renderItem(GuiRenderer& gui, f32 x, f32 y, const ItemStack& stack) {
    if (stack.isEmpty()) {
        return;
    }

    // TODO: 实现真正的物品图标渲染
    // 目前绘制占位符（根据物品类型选择颜色）
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return;
    }

    // 根据物品ID生成不同颜色（简化处理）
    ItemId itemId = item->itemId();
    u32 hue = (itemId * 37) % 360;

    // HSV转RGB
    f32 h = static_cast<f32>(hue) / 60.0f;
    f32 c = 1.0f;
    f32 x1 = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
    f32 r = 0, g = 0, b = 0;

    if (h < 1) { r = c; g = x1; }
    else if (h < 2) { r = x1; g = c; }
    else if (h < 3) { g = c; b = x1; }
    else if (h < 4) { g = x1; b = c; }
    else if (h < 5) { r = x1; b = c; }
    else { r = c; b = x1; }

    u32 color = (0xFF << 24) |
                (static_cast<u32>(r * 255) << 16) |
                (static_cast<u32>(g * 255) << 8) |
                (static_cast<u32>(b * 255));

    gui.fillRect(x, y, ITEM_SIZE, ITEM_SIZE, color);
}

void SlotRenderer::renderCount(GuiRenderer& gui, f32 x, f32 y, i32 count) {
    if (count <= 1) {
        return;
    }

    std::string countText;
    if (count >= 64) {
        countText = std::to_string(count);
    } else {
        countText = std::to_string(count);
    }

    f32 textWidth = gui.getTextWidth(countText);
    gui.drawText(countText, x - textWidth, y, SlotColors::COUNT_TEXT, true);
}

void SlotRenderer::renderDurabilityBar(GuiRenderer& gui, f32 x, f32 y, const ItemStack& stack) {
    if (!stack.isDamageable()) {
        return;
    }

    i32 maxDamage = stack.getMaxDamage();
    i32 damage = stack.getDamage();
    if (maxDamage <= 0) {
        return;
    }

    // 计算耐久度百分比
    f32 durability = static_cast<f32>(maxDamage - damage) / static_cast<f32>(maxDamage);

    // 耐久度条宽度
    constexpr f32 BAR_WIDTH = 13.0f;
    constexpr f32 BAR_HEIGHT = 2.0f;

    // 背景
    gui.fillRect(x, y, BAR_WIDTH, BAR_HEIGHT, SlotColors::DURABILITY_BG);

    // 前景（根据耐久度变色）
    u32 color;
    if (durability > 0.7f) {
        color = 0xFF00FF00;  // 绿色
    } else if (durability > 0.4f) {
        color = 0xFFFFFF00;  // 黄色
    } else if (durability > 0.2f) {
        color = 0xFFFF8800;  // 橙色
    } else {
        color = 0xFFFF0000;  // 红色
    }

    gui.fillRect(x, y, BAR_WIDTH * durability, BAR_HEIGHT, color);
}

} // namespace client
} // namespace mc
