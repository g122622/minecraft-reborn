#include "HudRenderer.hpp"
#include "../GuiRenderer.hpp"
#include "../../../common/entity/Player.hpp"
#include "../../../common/entity/inventory/PlayerInventory.hpp"
#include "../../../common/item/ItemStack.hpp"
#include "../../../common/item/Item.hpp"

namespace mr {
namespace client {

// ============================================================================
// 常量
// ============================================================================

namespace {
    // 快捷栏尺寸
    constexpr f32 HOTBAR_WIDTH = 182.0f;
    constexpr f32 HOTBAR_HEIGHT = 22.0f;
    constexpr f32 SLOT_SIZE = 18.0f;
    constexpr f32 SLOT_SPACING = 20.0f;
    constexpr f32 HOTBAR_OFFSET_Y = 3.0f;

    // 心形图标尺寸
    constexpr f32 HEART_SIZE = 9.0f;
    constexpr f32 HEART_SPACING = 10.0f;
    constexpr f32 HEALTH_OFFSET_Y = 12.0f;

    // 饥饿图标尺寸
    constexpr f32 HUNGER_SIZE = 9.0f;
    constexpr f32 HUNGER_SPACING = 10.0f;

    // 盔甲图标尺寸
    constexpr f32 ARMOR_SIZE = 9.0f;
    constexpr f32 ARMOR_SPACING = 10.0f;

    // 经验条尺寸
    constexpr f32 XP_BAR_WIDTH = 182.0f;
    constexpr f32 XP_BAR_HEIGHT = 5.0f;
    constexpr f32 XP_TEXT_OFFSET_Y = -7.0f;

    // 物品提示
    constexpr f32 TOOLTIP_PADDING = 4.0f;
    constexpr f32 TOOLTIP_OFFSET_Y = 12.0f;
}

// ============================================================================
// 构造函数
// ============================================================================

HudRenderer::HudRenderer()
{
}

// ============================================================================
// 初始化
// ============================================================================

bool HudRenderer::initialize() {
    if (m_initialized) {
        return true;
    }

    m_initialized = true;
    return true;
}

// ============================================================================
// 渲染
// ============================================================================

void HudRenderer::render(GuiRenderer& gui, const Player& player,
                         const PlayerInventory& inventory,
                         f32 screenWidth, f32 screenHeight) {
    if (!m_visible || !m_initialized) {
        return;
    }

    // 渲染经验条（在快捷栏下方）
    renderExperience(gui, player, screenWidth, screenHeight);

    // 渲染快捷栏
    renderHotbar(gui, inventory, screenWidth, screenHeight);

    // 渲染生命值和盔甲
    renderHealth(gui, player, screenWidth, screenHeight);

    // 渲染饥饿值
    renderHunger(gui, player, screenWidth, screenHeight);
}

void HudRenderer::renderHotbar(GuiRenderer& gui, const PlayerInventory& inventory,
                                f32 screenWidth, f32 screenHeight) {
    // 快捷栏位置（底部居中）
    f32 hotbarX = (screenWidth - HOTBAR_WIDTH) / 2.0f;
    f32 hotbarY = screenHeight - HOTBAR_HEIGHT - HOTBAR_OFFSET_Y;

    // 绘制背景
    gui.fillRect(hotbarX, hotbarY, HOTBAR_WIDTH, HOTBAR_HEIGHT,
                 HudColors::HOTBAR_BACKGROUND);

    // 绘制槽位
    i32 selectedSlot = inventory.getSelectedSlot();
    for (i32 i = 0; i < 9; ++i) {
        f32 slotX = hotbarX + i * SLOT_SPACING + 1.0f;
        f32 slotY = hotbarY + 2.0f;

        // 选中槽位高亮
        if (i == selectedSlot) {
            gui.fillRect(slotX - 1, slotY - 1, SLOT_SIZE + 2, SLOT_SIZE + 2,
                         HudColors::HOTBAR_SLOT_HIGHLIGHT);
        } else {
            gui.fillRect(slotX, slotY, SLOT_SIZE, SLOT_SIZE,
                         HudColors::HOTBAR_SLOT);
        }

        // 绘制物品（如果有）
        const ItemStack& stack = inventory.getItem(i);
        if (!stack.isEmpty()) {
            // 绘制物品数量
            if (stack.getCount() > 1) {
                std::string countText = std::to_string(stack.getCount());
                f32 textWidth = gui.getTextWidth(countText);
                gui.drawText(countText,
                            slotX + SLOT_SIZE - textWidth - 1.0f,
                            slotY + SLOT_SIZE - 8.0f,
                            HudColors::TOOLTIP_TEXT, true);
            }
        }
    }
}

void HudRenderer::renderHealth(GuiRenderer& gui, const Player& player,
                                f32 screenWidth, f32 screenHeight) {
    // 生命值位置（快捷栏左上方）
    f32 healthX = (screenWidth - HOTBAR_WIDTH) / 2.0f;
    f32 healthY = screenHeight - HOTBAR_HEIGHT - HEALTH_OFFSET_Y - HEART_SIZE;

    // 获取生命值
    f32 health = player.health();
    f32 maxHealth = player.maxHealth();
    i32 absorption = static_cast<i32>(player.absorptionAmount());
    i32 armor = player.armorValue();

    // 绘制盔甲值（在生命值上方）
    if (armor > 0) {
        f32 armorX = healthX;
        f32 armorY = healthY - ARMOR_SIZE - 2.0f;
        for (i32 i = 0; i < 10; ++i) {
            drawArmor(gui, armorX + i * ARMOR_SPACING, armorY, i < armor / 2);
        }
    }

    // 绘制心形（每行最多10颗）
    for (i32 i = 0; i < 10; ++i) {
        f32 heartX = healthX + i * HEART_SPACING;
        bool full = (i * 2 + 2) <= health;
        bool half = !full && (i * 2 + 1) <= health;

        if (absorption > 0 && i < absorption / 2) {
            drawHeart(gui, heartX, healthY, true, true);
        } else {
            drawHeart(gui, heartX, healthY, full || half, false);
        }
    }
}

void HudRenderer::renderHunger(GuiRenderer& gui, const Player& player,
                                f32 screenWidth, f32 screenHeight) {
    // 饥饿值位置（快捷栏右上方）
    f32 hungerX = (screenWidth + HOTBAR_WIDTH) / 2.0f - HUNGER_SPACING * 10;
    f32 hungerY = screenHeight - HOTBAR_HEIGHT - HEALTH_OFFSET_Y - HUNGER_SIZE;

    // 获取饥饿值
    i32 food = player.foodStats().foodLevel;

    // 绘制饥饿图标（从右到左）
    for (i32 i = 0; i < 10; ++i) {
        f32 hungerIconX = hungerX + (9 - i) * HUNGER_SPACING;
        drawHunger(gui, hungerIconX, hungerY, i < food / 2);
    }
}

void HudRenderer::renderExperience(GuiRenderer& gui, const Player& player,
                                    f32 screenWidth, f32 screenHeight) {
    // 经验条位置（快捷栏下方）
    f32 xpX = (screenWidth - XP_BAR_WIDTH) / 2.0f;
    f32 xpY = screenHeight - HOTBAR_HEIGHT - XP_BAR_HEIGHT - 4.0f;

    // 获取经验值
    i32 level = player.experienceLevel();
    f32 progress = player.experienceProgress();

    // 绘制背景
    gui.fillRect(xpX, xpY, XP_BAR_WIDTH, XP_BAR_HEIGHT, HudColors::XP_BACKGROUND);

    // 绘制进度条
    if (progress > 0.0f) {
        gui.fillRect(xpX, xpY, XP_BAR_WIDTH * progress, XP_BAR_HEIGHT,
                     HudColors::XP_FOREGROUND);
    }

    // 绘制等级文字（在经验条上方居中）
    if (level > 0) {
        std::string levelText = std::to_string(level);
        f32 textWidth = gui.getTextWidth(levelText);
        gui.drawText(levelText,
                     (screenWidth - textWidth) / 2.0f,
                     xpY + XP_TEXT_OFFSET_Y,
                     HudColors::XP_TEXT, true);
    }
}

void HudRenderer::renderItemTooltip(GuiRenderer& gui, const ItemStack& stack,
                                     f32 mouseX, f32 mouseY) {
    if (stack.isEmpty()) {
        return;
    }

    // 获取物品名称
    std::string itemName;
    if (stack.hasCustomName()) {
        itemName = stack.getDisplayName();
    } else {
        // 使用物品的默认名称
        itemName = stack.getItem()->getName();
    }

    // 计算提示框尺寸
    f32 textWidth = gui.getTextWidth(itemName);
    f32 tooltipWidth = textWidth + TOOLTIP_PADDING * 2;
    f32 tooltipHeight = gui.getFontHeight() + TOOLTIP_PADDING * 2;

    // 提示框位置（鼠标右上方）
    f32 tooltipX = mouseX + TOOLTIP_OFFSET_Y;
    f32 tooltipY = mouseY - tooltipHeight - TOOLTIP_OFFSET_Y;

    // 确保不超出屏幕
    // TODO: 获取屏幕尺寸进行边界检查

    // 绘制背景
    gui.fillRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight,
                 HudColors::TOOLTIP_BACKGROUND);

    // 绘制边框
    gui.drawRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight,
                 HudColors::TOOLTIP_BORDER);

    // 绘制物品名称
    gui.drawText(itemName,
                 tooltipX + TOOLTIP_PADDING,
                 tooltipY + TOOLTIP_PADDING,
                 HudColors::TOOLTIP_TEXT, false);
}

// ============================================================================
// 私有方法
// ============================================================================

void HudRenderer::drawHeart(GuiRenderer& gui, f32 x, f32 y, bool full, bool absorbing) {
    // 使用文本绘制心形（简化版本）
    // 实际版本应该使用纹理图标
    u32 color = absorbing ? HudColors::HEALTH_YELLOW : HudColors::HEALTH_RED;
    if (full) {
        gui.fillRect(x, y, HEART_SIZE, HEART_SIZE, color);
    } else {
        gui.fillRect(x, y, HEART_SIZE, HEART_SIZE, HudColors::HEALTH_EMPTY);
    }
}

void HudRenderer::drawHunger(GuiRenderer& gui, f32 x, f32 y, bool full) {
    // 使用文本绘制饥饿图标（简化版本）
    u32 color = full ? HudColors::HUNGER_FULL : HudColors::HUNGER_EMPTY;
    gui.fillRect(x, y, HUNGER_SIZE, HUNGER_SIZE, color);
}

void HudRenderer::drawArmor(GuiRenderer& gui, f32 x, f32 y, bool full) {
    // 使用文本绘制盔甲图标（简化版本）
    u32 color = full ? HudColors::HEALTH_RED : HudColors::HEALTH_EMPTY;
    gui.fillRect(x, y, ARMOR_SIZE, ARMOR_SIZE, color);
}

} // namespace client
} // namespace mr
