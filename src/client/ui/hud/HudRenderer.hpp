#pragma once

#include "../../../common/core/Types.hpp"
#include <vector>

namespace mc {

// Forward declarations
class Player;
class PlayerInventory;
class ItemStack;

namespace client {

// Forward declarations
class GuiRenderer;
class ItemRenderer;

/**
 * @brief HUD元素颜色常量
 */
namespace HudColors {
    // 快捷栏
    constexpr u32 HOTBAR_SLOT = 0xFF8B8B8B;        // 槽位背景
    constexpr u32 HOTBAR_SLOT_HIGHLIGHT = 0xFFFFFFFF;  // 选中槽位高亮
    constexpr u32 HOTBAR_BACKGROUND = 0xFF000000;   // 背景
    constexpr u32 HOTBAR_BORDER = 0xFF373737;       // 边框

    // 生命值
    constexpr u32 HEALTH_RED = 0xFFFF0000;          // 红心
    constexpr u32 HEALTH_YELLOW = 0xFFFFF600;       // 黄心（吸收）
    constexpr u32 HEALTH_EMPTY = 0xFF2A0A0A;        // 空心

    // 饥饿值
    constexpr u32 HUNGER_FULL = 0xFFE0A010;         // 满饥饿
    constexpr u32 HUNGER_EMPTY = 0xFF1A0A00;        // 空饥饿

    // 经验条
    constexpr u32 XP_BACKGROUND = 0xFF202020;       // 背景
    constexpr u32 XP_FOREGROUND = 0xFF7FFF00;       // 前景
    constexpr u32 XP_TEXT = 0xFF7FFF00;             // 文字

    // 物品提示
    constexpr u32 TOOLTIP_BACKGROUND = 0xF0100010;  // 背景带透明
    constexpr u32 TOOLTIP_BORDER = 0xFF5000FF;      // 边框
    constexpr u32 TOOLTIP_TEXT = 0xFFFFFFFF;        // 文字
}

/**
 * @brief HUD渲染器
 *
 * 渲染游戏内HUD元素：
 * - 快捷栏
 * - 生命值
 * - 饥饿值
 * - 盔甲值
 * - 经验条
 * - 物品提示
 *
 * 参考: net.minecraft.client.gui.IngameGui
 */
class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer() = default;

    // 禁止拷贝
    HudRenderer(const HudRenderer&) = delete;
    HudRenderer& operator=(const HudRenderer&) = delete;

    /**
     * @brief 初始化HUD渲染器
     * @param itemRenderer 物品渲染器
     * @return 成功或错误
     */
    [[nodiscard]] bool initialize(ItemRenderer* itemRenderer);

    /**
     * @brief 渲染HUD
     * @param gui GUI渲染器
     * @param player 玩家
     * @param inventory 玩家背包
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     */
    void render(GuiRenderer& gui, const Player& player,
                const PlayerInventory& inventory,
                f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染快捷栏
     */
    void renderHotbar(GuiRenderer& gui, const PlayerInventory& inventory,
                      f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染生命值和盔甲
     */
    void renderHealth(GuiRenderer& gui, const Player& player,
                      f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染饥饿值
     */
    void renderHunger(GuiRenderer& gui, const Player& player,
                      f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染经验条
     */
    void renderExperience(GuiRenderer& gui, const Player& player,
                          f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染物品提示
     * @param stack 物品堆
     * @param mouseX 鼠标X
     * @param mouseY 鼠标Y
     */
    void renderItemTooltip(GuiRenderer& gui, const ItemStack& stack,
                           f32 mouseX, f32 mouseY);

    /**
     * @brief 设置是否显示HUD
     */
    void setVisible(bool visible) { m_visible = visible; }

    /**
     * @brief 获取是否显示HUD
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }

private:
    /**
     * @brief 绘制心形图标
     */
    void drawHeart(GuiRenderer& gui, f32 x, f32 y, bool full, bool absorbing);

    /**
     * @brief 绘制饥饿图标
     */
    void drawHunger(GuiRenderer& gui, f32 x, f32 y, bool full);

    /**
     * @brief 绘制盔甲图标
     */
    void drawArmor(GuiRenderer& gui, f32 x, f32 y, bool full);

    ItemRenderer* m_itemRenderer = nullptr;
    bool m_visible = true;
    bool m_initialized = false;
};

} // namespace client
} // namespace mc
