#include "Slot.hpp"
#include "IInventory.hpp"

namespace mc {

Slot::Slot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
    : m_inventory(inventory)
    , m_slotIndex(slotIndex)
    , m_x(x)
    , m_y(y)
{
}

ItemStack Slot::getItem() const {
    if (m_inventory == nullptr) {
        return ItemStack::EMPTY;
    }
    return m_inventory->getItem(m_slotIndex);
}

void Slot::set(const ItemStack& stack) {
    if (m_inventory != nullptr) {
        m_inventory->setItem(m_slotIndex, stack);
    }
}

bool Slot::isEmpty() const {
    return getItem().isEmpty();
}

ItemStack Slot::remove(i32 amount) {
    if (m_inventory == nullptr) {
        return ItemStack::EMPTY;
    }
    return m_inventory->removeItem(m_slotIndex, amount);
}

bool Slot::mayPlace(const ItemStack& stack) const {
    if (m_inventory == nullptr) {
        return false;
    }
    return m_inventory->canPlaceItem(m_slotIndex, stack);
}

i32 Slot::getMaxStackSize() const {
    if (m_inventory == nullptr) {
        return 64;
    }
    return m_inventory->getMaxStackSize();
}

i32 Slot::getMaxStackSize(const ItemStack& stack) const {
    if (stack.isEmpty()) {
        return getMaxStackSize();
    }
    return std::min(stack.getMaxStackSize(), getMaxStackSize());
}

// ============================================================================
// ArmorSlot
// ============================================================================

ArmorSlot::ArmorSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, ArmorType armorType)
    : Slot(inventory, slotIndex, x, y)
    , m_armorType(armorType)
{
}

bool ArmorSlot::mayPlace(const ItemStack& stack) const {
    // TODO: 检查物品是否是对应类型的护甲
    // 目前简单实现：所有物品都可以放入
    // 后续需要添加 ArmorItem 类并检查 armorType
    return Slot::mayPlace(stack);
}

// ============================================================================
// ResultSlot
// ============================================================================

ResultSlot::ResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y,
                       CraftingInventory* craftingGrid)
    : Slot(inventory, slotIndex, x, y)
    , m_craftingGrid(craftingGrid)
{
}

} // namespace mc
