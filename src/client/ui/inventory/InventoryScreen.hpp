#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/entity/inventory/ContainerTypes.hpp"
#include "../../../common/item/ItemStack.hpp"
#include <memory>

namespace mr {

// Forward declarations
class Player;
class PlayerInventory;
class Container;
class ItemStack;

namespace client {

// Forward declarations
class GuiRenderer;

/**
 * @brief 背包界面
 *
 * 显示玩家背包和交互容器。
 *
 * 参考: net.minecraft.client.gui.screen.inventory.InventoryScreen
 */
class InventoryScreen {
public:
    InventoryScreen();
    ~InventoryScreen() = default;

    // 禁止拷贝
    InventoryScreen(const InventoryScreen&) = delete;
    InventoryScreen& operator=(const InventoryScreen&) = delete;

    /**
     * @brief 初始化背包界面
     * @return 成功或错误
     */
    [[nodiscard]] bool initialize();

    /**
     * @brief 渲染背包界面
     * @param gui GUI渲染器
     * @param player 玩家
     * @param mouseX 鼠标X位置
     * @param mouseY 鼠标Y位置
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     */
    void render(GuiRenderer& gui, const Player& player,
                f32 mouseX, f32 mouseY, f32 screenWidth, f32 screenHeight);

    /**
     * @brief 处理鼠标点击
     * @param container 容器
     * @param mouseX 鼠标X位置
     * @param mouseY 鼠标Y位置
     * @param button 鼠标按钮（0=左键，1=右键）
     * @return 是否处理了点击
     */
    bool onClick(Container& container, f32 mouseX, f32 mouseY, i32 button);

    /**
     * @brief 处理按键
     * @param key 按键代码
     * @return 是否处理了按键
     */
    bool onKeyPress(i32 key);

    /**
     * @brief 处理滚轮
     * @param delta 滚轮增量
     * @return 是否处理了滚轮
     */
    bool onScroll(f32 delta);

    /**
     * @brief 获取鼠标持有的物品
     */
    [[nodiscard]] const ItemStack& getCursorItem() const { return m_cursorItem; }
    ItemStack& getCursorItem() { return m_cursorItem; }

    /**
     * @brief 设置鼠标持有的物品
     */
    void setCursorItem(const ItemStack& stack) { m_cursorItem = stack; }

    /**
     * @brief 是否打开背包界面
     */
    [[nodiscard]] bool isOpen() const { return m_open; }

    /**
     * @brief 打开/关闭背包界面
     */
    void setOpen(bool open) { m_open = open; }

    /**
     * @brief 切换背包界面开关
     */
    void toggle() { m_open = !m_open; }

private:
    /**
     * @brief 渲染玩家模型
     */
    void renderPlayerModel(GuiRenderer& gui, const Player& player,
                           f32 x, f32 y, f32 scale);

    /**
     * @brief 渲染槽位
     */
    void renderSlot(GuiRenderer& gui, i32 slotIndex, f32 x, f32 y,
                    const ItemStack& stack, f32 mouseX, f32 mouseY);

    /**
     * @brief 获取槽位位置
     */
    void getSlotPosition(i32 slotIndex, f32& x, f32& y) const;

    /**
     * @brief 检测鼠标是否在槽位上
     * @return 槽位索引，-1表示不在任何槽位上
     */
    i32 getSlotAtPosition(f32 mouseX, f32 mouseY) const;

    /**
     * @brief 渲染物品提示
     */
    void renderItemTooltip(GuiRenderer& gui, const ItemStack& stack,
                           f32 mouseX, f32 mouseY);

    // 界面尺寸常量
    static constexpr f32 INVENTORY_WIDTH = 176.0f;
    static constexpr f32 INVENTORY_HEIGHT = 166.0f;
    static constexpr f32 SLOT_SIZE = 18.0f;
    static constexpr f32 SLOT_SPACING = 18.0f;

    // 槽位起始位置
    static constexpr f32 HOTBAR_X = 8.0f;
    static constexpr f32 HOTBAR_Y = 142.0f;
    static constexpr f32 MAIN_X = 8.0f;
    static constexpr f32 MAIN_Y = 84.0f;
    static constexpr f32 ARMOR_X = 8.0f;
    static constexpr f32 ARMOR_Y = 8.0f;
    static constexpr f32 OFFHAND_X = 152.0f;
    static constexpr f32 OFFHAND_Y = 84.0f;

    ItemStack m_cursorItem;     // 鼠标持有的物品
    bool m_open = false;        // 是否打开
    bool m_initialized = false; // 是否已初始化
};

} // namespace client
} // namespace mr
