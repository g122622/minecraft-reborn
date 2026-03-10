#pragma once

#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "screen/ScreenType.hpp"
#include <memory>

namespace mr {

class CraftingTableEntity;
class World;

/**
 * @brief 工作台容器菜单
 *
 * 管理3x3合成网格和结果槽位。
 * 槽位布局：
 * - 槽位 0-8: 合成网格 (3x3)
 * - 槽位 9: 结果槽位
 * - 槽位 10-36: 玩家主背包 (3x9)
 * - 槽位 37-45: 玩家快捷栏 (1x9)
 */
class CraftingMenu : public AbstractContainerMenu {
public:
    /**
     * @brief 合成网格宽度
     */
    static constexpr i32 GRID_WIDTH = 3;

    /**
     * @brief 合成网格高度
     */
    static constexpr i32 GRID_HEIGHT = 3;

    /**
     * @brief 构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param blockEntity 工作台方块实体（可为nullptr表示玩家背包合成）
     */
    CraftingMenu(ContainerId id, PlayerInventory* playerInventory, CraftingTableEntity* blockEntity);

    /**
     * @brief 构造函数（无方块实体，用于玩家背包2x2合成）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param width 网格宽度
     * @param height 网格高度
     */
    CraftingMenu(ContainerId id, PlayerInventory* playerInventory, i32 width, i32 height);

    /**
     * @brief 获取合成网格
     * @return 合成网格引用
     */
    [[nodiscard]] CraftingInventory& getCraftingGrid() { return m_craftingGrid; }
    [[nodiscard]] const CraftingInventory& getCraftingGrid() const { return m_craftingGrid; }

    /**
     * @brief 获取结果槽位
     * @return 结果背包引用
     */
    [[nodiscard]] CraftResultInventory& getResultInventory() { return m_result; }
    [[nodiscard]] const CraftResultInventory& getResultInventory() const { return m_result; }

    /**
     * @brief 容器内容变化时调用
     * @param inventory 变化的背包
     */
    void slotsChanged(IInventory* inventory) override;

    ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) override;

    /**
     * @brief 检查玩家是否可以访问容器
     * @param player 玩家
     * @return 如果可以访问返回true
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 快速移动物品
     * @param slotIndex 槽位索引
     * @param player 玩家
     * @return 移动后的物品堆
     */
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    /**
     * @brief 关闭容器
     * @param player 玩家
     */
    void removed(Player& player) override;

    /**
     * @brief 更新合成结果
     *
     * 查找匹配的配方并更新结果槽位。
     */
    void updateResult();

    /**
     * @brief 获取屏幕类型
     * @return 屏幕类型
     */
    [[nodiscard]] ScreenType getScreenType() const { return m_screenType; }

    [[nodiscard]] i32 getResultSlotIndex() const override { return RESULT_SLOT; }

    /**
     * @brief 槽位索引常量
     */
    static constexpr i32 GRID_SLOT_START = 0;
    static constexpr i32 GRID_SLOT_COUNT = 9;
    static constexpr i32 RESULT_SLOT = 9;
    static constexpr i32 PLAYER_INV_START = 10;

    /**
     * @brief 检查是否是合成网格槽位
     */
    [[nodiscard]] bool isGridSlot(i32 slotIndex) const {
        return slotIndex >= GRID_SLOT_START && slotIndex < GRID_SLOT_START + GRID_SLOT_COUNT;
    }

protected:
    /**
     * @brief 添加合成网格槽位
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     */
    void addCraftingGridSlots(i32 startX, i32 startY);

    /**
     * @brief 添加结果槽位
     * @param x X坐标
     * @param y Y坐标
     */
    void addResultSlot(i32 x, i32 y);

private:
    bool handleResultSlotClick();
    void consumeIngredients();

    CraftingInventory m_craftingGrid;
    CraftResultInventory m_result;
    CraftingTableEntity* m_blockEntity;
    ScreenType m_screenType;
};

/**
 * @brief 玩家背包合成菜单 (2x2)
 *
 * 管理玩家背包中的2x2合成网格。
 * 槽位布局：
 * - 槽位 0-3: 合成网格 (2x2)
 * - 槽位 4: 结果槽位
 * - 槽位 5-32: 玩家主背包 (3x9)
 * - 槽位 33-41: 玩家快捷栏 (1x9)
 */
class InventoryCraftingMenu : public AbstractContainerMenu {
public:
    /**
     * @brief 构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     */
    InventoryCraftingMenu(ContainerId id, PlayerInventory* playerInventory);

    /**
     * @brief 获取合成网格
     * @return 合成网格引用
     */
    [[nodiscard]] CraftingInventory& getCraftingGrid() { return m_craftingGrid; }
    [[nodiscard]] const CraftingInventory& getCraftingGrid() const { return m_craftingGrid; }

    /**
     * @brief 获取结果槽位
     * @return 结果背包引用
     */
    [[nodiscard]] CraftResultInventory& getResultInventory() { return m_result; }
    [[nodiscard]] const CraftResultInventory& getResultInventory() const { return m_result; }

    /**
     * @brief 容器内容变化时调用
     * @param inventory 变化的背包
     */
    void slotsChanged(IInventory* inventory) override;

    ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) override;

    /**
     * @brief 检查玩家是否可以访问容器
     * @param player 玩家
     * @return 总是返回true（玩家背包）
     */
    [[nodiscard]] bool stillValid(const Player& player) const override { return true; }

    /**
     * @brief 快速移动物品
     * @param slotIndex 槽位索引
     * @param player 玩家
     * @return 移动后的物品堆
     */
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    /**
     * @brief 更新合成结果
     */
    void updateResult();

    [[nodiscard]] i32 getResultSlotIndex() const override { return RESULT_SLOT; }

    /**
     * @brief 槽位索引常量
     */
    static constexpr i32 GRID_SLOT_START = 0;
    static constexpr i32 GRID_SLOT_COUNT = 4;
    static constexpr i32 RESULT_SLOT = 4;
    static constexpr i32 PLAYER_INV_START = 5;

private:
    bool handleResultSlotClick();
    void consumeIngredients();

    CraftingInventory m_craftingGrid;
    CraftResultInventory m_result;
};

} // namespace mr
