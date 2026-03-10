#include "server/menu/CraftingMenu.hpp"
#include "world/blockentity/CraftingTableEntity.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "entity/Player.hpp"

namespace mr {

// ========== CraftingMenu 实现 ==========

CraftingMenu::CraftingMenu(ContainerId id, PlayerInventory* playerInventory, CraftingTableEntity* blockEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_craftingGrid(GRID_WIDTH, GRID_HEIGHT)
    , m_blockEntity(blockEntity)
    , m_screenType(ScreenType::CraftingTable) {

    // 添加合成网格槽位 (槽位 0-8)
    addCraftingGridSlots(98, 18);

    // 添加结果槽位 (槽位 9)
    addResultSlot(154, 28);

    // 添加玩家主背包 (槽位 10-36)
    addPlayerInventorySlots(8, 84);

    // 添加玩家快捷栏 (槽位 37-45)
    addPlayerHotbarSlots(8, 142);
}

CraftingMenu::CraftingMenu(ContainerId id, PlayerInventory* playerInventory, i32 width, i32 height)
    : AbstractContainerMenu(id, playerInventory)
    , m_craftingGrid(width, height)
    , m_blockEntity(nullptr)
    , m_screenType(ScreenType::Inventory) {

    // 注意：玩家背包合成不创建单独的槽位
    // 槽位由 PlayerInventory 管理
}

void CraftingMenu::addCraftingGridSlots(i32 startX, i32 startY) {
    for (i32 y = 0; y < GRID_HEIGHT; ++y) {
        for (i32 x = 0; x < GRID_WIDTH; ++x) {
            i32 index = y * GRID_WIDTH + x;
            i32 posX = startX + x * 18;
            i32 posY = startY + y * 18;
            // TODO: 创建槽位
            // addSlot(std::make_unique<Slot>(m_craftingGrid, index, posX, posY));
        }
    }
}

void CraftingMenu::addResultSlot(i32 x, i32 y) {
    // TODO: 创建结果槽位
    // addSlot(std::make_unique<ResultSlot>(m_result, 0, x, y));
}

void CraftingMenu::slotsChanged(IInventory* inventory) {
    if (inventory == &m_craftingGrid) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

bool CraftingMenu::stillValid(const Player& player) const {
    // TODO: 检查玩家是否仍在工作台附近
    return true;
}

ItemStack CraftingMenu::quickMoveStack(i32 slotIndex, Player& player) {
    // TODO: 实现Shift+点击快速移动
    (void)slotIndex;
    (void)player;
    return ItemStack();
}

void CraftingMenu::removed(Player& player) {
    // 将持有物品返回玩家背包
    if (!m_carried.isEmpty()) {
        // TODO: 将物品放入玩家背包或掉落
        m_carried = ItemStack();
    }

    // 将合成网格中的物品返回玩家
    for (i32 i = 0; i < m_craftingGrid.getContainerSize(); ++i) {
        ItemStack stack = m_craftingGrid.removeItemNoUpdate(i);
        if (!stack.isEmpty()) {
            // TODO: 将物品放入玩家背包或掉落
        }
    }

    AbstractContainerMenu::removed(player);
}

void CraftingMenu::updateResult() {
    const crafting::CraftingRecipe* recipe =
        crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);

    if (recipe != nullptr) {
        m_result.setResultItem(recipe->assemble(m_craftingGrid));
    } else {
        m_result.clear();
    }

    broadcastChanges();
}

// ========== InventoryCraftingMenu 实现 ==========

InventoryCraftingMenu::InventoryCraftingMenu(ContainerId id, PlayerInventory* playerInventory)
    : AbstractContainerMenu(id, playerInventory)
    , m_craftingGrid(2, 2) {

    // 注意：玩家背包合成槽位由 PlayerInventory 直接管理
    // 这里只是创建独立的网格对象用于配方匹配
}

void InventoryCraftingMenu::slotsChanged(IInventory* inventory) {
    if (inventory == &m_craftingGrid) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack InventoryCraftingMenu::quickMoveStack(i32 slotIndex, Player& player) {
    // TODO: 实现Shift+点击快速移动
    (void)slotIndex;
    (void)player;
    return ItemStack();
}

void InventoryCraftingMenu::updateResult() {
    const crafting::CraftingRecipe* recipe =
        crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);

    if (recipe != nullptr) {
        m_result.setResultItem(recipe->assemble(m_craftingGrid));
    } else {
        m_result.clear();
    }

    broadcastChanges();
}

} // namespace mr
