#include "server/menu/CraftingMenu.hpp"
#include "world/blockentity/CraftingTableEntity.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/inventory/CraftingInventory.hpp"
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
            addSlot(std::make_unique<Slot>(&m_craftingGrid, index, posX, posY));
        }
    }
}

void CraftingMenu::addResultSlot(i32 x, i32 y) {
    // 结果槽位：不能放入物品，只能取出
    addSlot(std::make_unique<ResultSlot>(&m_result, 0, x, y, &m_craftingGrid));
}

void CraftingMenu::slotsChanged(IInventory* inventory) {
    if (inventory == &m_craftingGrid) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

bool CraftingMenu::stillValid(const Player& player) const {
    // TODO: 检查玩家是否仍在工作台附近
    // 需要检查玩家与方块的距离是否在范围内（通常是3格）
    // 暂时返回true
    (void)player;
    return true;
}

ItemStack CraftingMenu::quickMoveStack(i32 slotIndex, Player& player) {
    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack originalStack = slot->getItem();
    ItemStack resultStack = originalStack.copy();

    // 结果槽位（槽位9）：Shift+点击移动到玩家背包
    if (slotIndex == RESULT_SLOT) {
        // 尝试移动到玩家背包
        if (!moveItemToRange(resultStack, PLAYER_INV_START, getSlotCount() - 1)) {
            return ItemStack();
        }
    }
    // 合成网格槽位（槽位0-8）：Shift+点击移动到玩家背包
    else if (isGridSlot(slotIndex)) {
        if (!moveItemToRange(resultStack, PLAYER_INV_START, getSlotCount() - 1)) {
            return ItemStack();
        }
    }
    // 玩家背包槽位：Shift+点击移动到合成网格
    else {
        // 优先移动到合成网格
        if (!moveItemToRange(resultStack, GRID_SLOT_START, GRID_SLOT_START + GRID_SLOT_COUNT - 1)) {
            return ItemStack();
        }
    }

    // 更新槽位
    if (resultStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->set(resultStack);
    }

    return originalStack;
}

void CraftingMenu::removed(Player& player) {
    // 将持有物品返回玩家背包
    if (!m_carried.isEmpty()) {
        player.inventory().add(m_carried);
        m_carried = ItemStack();
    }

    // 将合成网格中的物品返回玩家
    for (i32 i = 0; i < m_craftingGrid.getContainerSize(); ++i) {
        ItemStack stack = m_craftingGrid.removeItemNoUpdate(i);
        if (!stack.isEmpty()) {
            player.inventory().add(stack);
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
