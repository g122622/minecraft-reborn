#pragma once

#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "core/Types.hpp"

namespace mc::client {

/**
 * @brief 工作台屏幕 (3x3合成)
 *
 * 显示工作台的合成网格、结果槽位和玩家背包。
 *
 * 槽位布局：
 * - 槽位 0-8: 合成网格 (3x3)
 * - 槽位 9: 结果槽位
 * - 槽位 10-36: 玩家主背包 (3x9)
 * - 槽位 37-45: 玩家快捷栏 (1x9)
 *
 * GUI尺寸：
 * - 宽度: 176 像素
 * - 高度: 166 像素
 */
class CraftingScreen : public AbstractContainerScreen<mc::CraftingMenu> {
public:
    /**
     * @brief 构造函数
     * @param menu 工作台菜单
     */
    explicit CraftingScreen(std::unique_ptr<mc::CraftingMenu> menu,
                            ContainerClickSender clickSender = {},
                            ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] String getTitle() const override {
        return "Crafting";
    }

protected:
    /**
     * @brief 子类初始化
     */
    void onInit() override;

    /**
     * @brief 渲染容器背景
     */
    void renderContainerBackground() override;

    /**
     * @brief 渲染容器前景
     */
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;

    /**
     * @brief 槽位点击处理
     */
    bool onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) override;

private:
    /**
     * @brief 渲染合成网格
     */
    void renderCraftingGrid();

    /**
     * @brief 渲染结果槽位
     */
    void renderResultSlot();

    /**
     * @brief 渲染玩家背包区域
     */
    void renderPlayerInventory();

    /**
     * @brief 检查是否是结果槽位
     */
    [[nodiscard]] bool isResultSlot(i32 slotIndex) const {
        return slotIndex == mc::CraftingMenu::RESULT_SLOT;
    }

    /**
     * @brief 检查是否是合成网格槽位
     */
    [[nodiscard]] bool isGridSlot(i32 slotIndex) const {
        return slotIndex >= mc::CraftingMenu::GRID_SLOT_START &&
               slotIndex < mc::CraftingMenu::GRID_SLOT_START + mc::CraftingMenu::GRID_SLOT_COUNT;
    }

    // GUI尺寸常量
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;

    // 合成网格位置（相对于GUI左上角）
    static constexpr i32 GRID_X = 30;
    static constexpr i32 GRID_Y = 17;

    // 结果槽位位置
    static constexpr i32 RESULT_X = 123;
    static constexpr i32 RESULT_Y = 35;

    // 玩家背包位置
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;

    // 标题位置
    static constexpr i32 TITLE_X = 28;
    static constexpr i32 TITLE_Y = 6;
};

/**
 * @brief 玩家背包合成屏幕 (2x2合成)
 *
 * 显示玩家背包内的2x2合成网格和结果槽位。
 *
 * 槽位布局：
 * - 槽位 0-3: 合成网格 (2x2)
 * - 槽位 4: 结果槽位
 * - 槽位 5-32: 玩家主背包 (3x9)
 * - 槽位 33-41: 玩家快捷栏 (1x9)
 *
 * GUI尺寸：
 * - 宽度: 176 像素
 * - 高度: 166 像素
 */
class InventoryCraftingScreen : public AbstractContainerScreen<mc::InventoryCraftingMenu> {
public:
    /**
     * @brief 构造函数
     * @param menu 玩家背包合成菜单
     */
    explicit InventoryCraftingScreen(std::unique_ptr<mc::InventoryCraftingMenu> menu,
                                     ContainerClickSender clickSender = {},
                                     ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] String getTitle() const override {
        return "Inventory";
    }

protected:
    /**
     * @brief 子类初始化
     */
    void onInit() override;

    /**
     * @brief 渲染容器背景
     */
    void renderContainerBackground() override;

    /**
     * @brief 渲染容器前景
     */
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;

    /**
     * @brief 槽位点击处理
     */
    bool onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) override;

private:
    /**
     * @brief 渲染合成网格
     */
    void renderCraftingGrid();

    /**
     * @brief 渲染结果槽位
     */
    void renderResultSlot();

    /**
     * @brief 渲染玩家背包区域
     */
    void renderPlayerInventory();

    /**
     * @brief 检查是否是结果槽位
     */
    [[nodiscard]] bool isResultSlot(i32 slotIndex) const {
        return slotIndex == mc::InventoryCraftingMenu::RESULT_SLOT;
    }

    /**
     * @brief 检查是否是合成网格槽位
     */
    [[nodiscard]] bool isGridSlot(i32 slotIndex) const {
        return slotIndex >= mc::InventoryCraftingMenu::GRID_SLOT_START &&
               slotIndex < mc::InventoryCraftingMenu::GRID_SLOT_START + mc::InventoryCraftingMenu::GRID_SLOT_COUNT;
    }

    // GUI尺寸常量
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;

    // 合成网格位置（相对于GUI左上角）
    static constexpr i32 GRID_X = 97;
    static constexpr i32 GRID_Y = 18;

    // 结果槽位位置
    static constexpr i32 RESULT_X = 143;
    static constexpr i32 RESULT_Y = 28;

    // 玩家背包位置
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;

    // 标题位置
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;
};

} // namespace mc::client
