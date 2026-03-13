#include "client/ui/screen/CraftingScreen.hpp"
#include "entity/Player.hpp"
#include "entity/inventory/Slot.hpp"

namespace mc::client {

// ========== CraftingScreen 实现 ==========

CraftingScreen::CraftingScreen(std::unique_ptr<mc::CraftingMenu> menu,
                               ContainerClickSender clickSender,
                               ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::CraftingMenu>(std::move(menu), std::move(clickSender), std::move(closeSender)) {
    setImageSize(GUI_WIDTH, GUI_HEIGHT);
}

void CraftingScreen::onInit() {
    // 计算居中位置
    // TODO: 获取实际屏幕尺寸
    m_leftPos = 0;
    m_topPos = 0;
}

void CraftingScreen::renderContainerBackground() {
    // 渲染工作台GUI背景
    // TODO: 使用 GuiTextureAtlas 加载纹理
    // 纹理路径: minecraft:textures/gui/container/crafting_table.png

    // 合成网格区域: (30, 17) - 3x3 格子
    // 结果槽位区域: (123, 35) - 1 格子
    // 玩家背包区域: (8, 84) - 3x9 格子
    // 快捷栏区域: (8, 142) - 1x9 格子

    renderCraftingGrid();
    renderResultSlot();
    renderPlayerInventory();
}

void CraftingScreen::renderContainerForeground(i32 mouseX, i32 mouseY) {
    (void)mouseX;
    (void)mouseY;

    // 渲染标题 "Crafting"
    // TODO: 使用字体渲染器
    // drawText(TITLE_X, TITLE_Y, "Crafting", 0x404040);
}

void CraftingScreen::renderCraftingGrid() {
    // 渲染3x3合成网格
    if (m_menu == nullptr) {
        return;
    }

    // TODO: 使用 SlotRenderer 渲染槽位
    // 遍历合成网格槽位 (0-8)
    for (i32 i = 0; i < mc::CraftingMenu::GRID_SLOT_COUNT; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            // 计算位置
            i32 gridX = i % 3;
            i32 gridY = i / 3;
            i32 screenX = m_leftPos + GRID_X + gridX * 18;
            i32 screenY = m_topPos + GRID_Y + gridY * 18;

            // TODO: 调用 SlotRenderer::renderSlot(guiRenderer, screenX, screenY, slot->getItem())
            (void)screenX;
            (void)screenY;
            (void)slot;
        }
    }
}

void CraftingScreen::renderResultSlot() {
    // 渲染结果槽位
    if (m_menu == nullptr) {
        return;
    }

    mc::Slot* resultSlot = m_menu->getSlot(mc::CraftingMenu::RESULT_SLOT);
    if (resultSlot != nullptr) {
        i32 screenX = m_leftPos + RESULT_X;
        i32 screenY = m_topPos + RESULT_Y;

        // TODO: 渲染结果槽位（可能有特殊效果）
        (void)screenX;
        (void)screenY;
        (void)resultSlot;
    }
}

void CraftingScreen::renderPlayerInventory() {
    // 渲染玩家背包区域
    if (m_menu == nullptr) {
        return;
    }

    // 主背包槽位 (10-36)
    for (i32 i = mc::CraftingMenu::PLAYER_INV_START;
         i < mc::CraftingMenu::PLAYER_INV_START + 27; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            // 计算位置 (3行9列)
            i32 invIndex = i - mc::CraftingMenu::PLAYER_INV_START;
            i32 row = invIndex / 9;
            i32 col = invIndex % 9;
            i32 screenX = m_leftPos + PLAYER_INV_X + col * 18;
            i32 screenY = m_topPos + PLAYER_INV_Y + row * 18;

            // TODO: 渲染槽位
            (void)screenX;
            (void)screenY;
            (void)slot;
        }
    }
}

bool CraftingScreen::onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) {
    // 特殊处理结果槽位
    if (isResultSlot(slotIndex)) {
        // 结果槽位点击逻辑
        // 1. 检查是否有结果
        // 2. 检查玩家是否持有物品
        // 3. 如果持有物品且能合并，合并
        // 4. 否则取出结果
        // 5. 消耗原料（由菜单处理）
    }

    // 普通槽位处理
    return AbstractContainerScreen::onSlotClick(slot, slotIndex, button);
}

// ========== InventoryCraftingScreen 实现 ==========

InventoryCraftingScreen::InventoryCraftingScreen(std::unique_ptr<mc::InventoryCraftingMenu> menu,
                                                 ContainerClickSender clickSender,
                                                 ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::InventoryCraftingMenu>(std::move(menu), std::move(clickSender), std::move(closeSender)) {
    setImageSize(GUI_WIDTH, GUI_HEIGHT);
}

void InventoryCraftingScreen::onInit() {
    // 计算居中位置
    m_leftPos = 0;
    m_topPos = 0;
}

void InventoryCraftingScreen::renderContainerBackground() {
    // TODO: 渲染玩家背包GUI纹理
    // 纹理路径: minecraft:textures/gui/container/inventory.png

    // 合成网格区域: (97, 18) - 2x2 格子
    // 结果槽位区域: (143, 28) - 1 格子
    // 玩家背包区域: (8, 84) - 3x9 格子
    // 快捷栏区域: (8, 142) - 1x9 格子
    // 护甲区域: (8, 8) - 4格

    renderCraftingGrid();
    renderResultSlot();
    renderPlayerInventory();
}

void InventoryCraftingScreen::renderContainerForeground(i32 mouseX, i32 mouseY) {
    (void)mouseX;
    (void)mouseY;

    // 渲染标题 "Inventory"
    // TODO: 使用字体渲染器
    // drawText(TITLE_X, TITLE_Y, "Inventory", 0x404040);
}

void InventoryCraftingScreen::renderCraftingGrid() {
    // 渲染2x2合成网格
    if (m_menu == nullptr) {
        return;
    }

    // 遍历合成网格槽位 (0-3)
    for (i32 i = 0; i < mc::InventoryCraftingMenu::GRID_SLOT_COUNT; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            // 计算位置
            i32 gridX = i % 2;
            i32 gridY = i / 2;
            i32 screenX = m_leftPos + GRID_X + gridX * 18;
            i32 screenY = m_topPos + GRID_Y + gridY * 18;

            // TODO: 渲染槽位
            (void)screenX;
            (void)screenY;
        }
    }
}

void InventoryCraftingScreen::renderResultSlot() {
    // 渲染结果槽位
    if (m_menu == nullptr) {
        return;
    }

    mc::Slot* resultSlot = m_menu->getSlot(mc::InventoryCraftingMenu::RESULT_SLOT);
    if (resultSlot != nullptr) {
        i32 screenX = m_leftPos + RESULT_X;
        i32 screenY = m_topPos + RESULT_Y;

        // TODO: 渲染结果槽位
        (void)screenX;
        (void)screenY;
    }
}

void InventoryCraftingScreen::renderPlayerInventory() {
    // 渲染玩家背包区域
    if (m_menu == nullptr) {
        return;
    }

    // 主背包槽位 (5-32)
    for (i32 i = mc::InventoryCraftingMenu::PLAYER_INV_START;
         i < mc::InventoryCraftingMenu::PLAYER_INV_START + 27; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            // 计算位置
            i32 invIndex = i - mc::InventoryCraftingMenu::PLAYER_INV_START;
            i32 row = invIndex / 9;
            i32 col = invIndex % 9;
            i32 screenX = m_leftPos + PLAYER_INV_X + col * 18;
            i32 screenY = m_topPos + PLAYER_INV_Y + row * 18;

            // TODO: 渲染槽位
            (void)screenX;
            (void)screenY;
        }
    }
}

bool InventoryCraftingScreen::onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) {
    // 特殊处理结果槽位
    if (isResultSlot(slotIndex)) {
        // 结果槽位点击逻辑（同工作台）
    }

    if (m_menu != nullptr) {
        Player fakePlayer(0, "InventoryClient");
        const ClickType clickType = (button == 0) ? ClickType::Pick : ClickType::PlaceSome;
        m_menu->clicked(slotIndex, button, clickType, fakePlayer);
    }

    // 普通槽位处理
    return AbstractContainerScreen::onSlotClick(slot, slotIndex, button);
}

} // namespace mc::client
