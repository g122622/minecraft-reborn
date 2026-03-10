#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/Player.hpp"

namespace mr {

AbstractContainerMenu::AbstractContainerMenu(ContainerId id, PlayerInventory* playerInventory)
    : m_id(id)
    , m_playerInventory(playerInventory)
    , m_carried() {
}

Slot* AbstractContainerMenu::getSlot(i32 index) {
    if (index >= 0 && index < static_cast<i32>(m_slots.size())) {
        return m_slots[index].get();
    }
    return nullptr;
}

const Slot* AbstractContainerMenu::getSlot(i32 index) const {
    if (index >= 0 && index < static_cast<i32>(m_slots.size())) {
        return m_slots[index].get();
    }
    return nullptr;
}

i32 AbstractContainerMenu::addSlot(std::unique_ptr<Slot> slot) {
    i32 index = static_cast<i32>(m_slots.size());
    m_slots.push_back(std::move(slot));
    return index;
}

void AbstractContainerMenu::addPlayerInventorySlots(i32 startX, i32 startY) {
    m_playerInvStart = static_cast<i32>(m_slots.size());
    // TODO: 创建槽位
    (void)startX;
    (void)startY;
    m_playerInvEnd = static_cast<i32>(m_slots.size());
}

void AbstractContainerMenu::addPlayerHotbarSlots(i32 startX, i32 startY) {
    m_hotbarStart = static_cast<i32>(m_slots.size());
    // TODO: 创建槽位
    (void)startX;
    (void)startY;
    m_hotbarEnd = static_cast<i32>(m_slots.size());
}

void AbstractContainerMenu::setCarriedItem(const ItemStack& stack) {
    m_carried = stack;
}

void AbstractContainerMenu::broadcastChanges() {
    for (i32 i = 0; i < static_cast<i32>(m_slots.size()); ++i) {
        const Slot* slot = m_slots[i].get();
        if (slot) {
            ItemStack stack = slot->getItem();
            notifySlotChanged(i, stack);
        }
    }
}

i32 AbstractContainerMenu::addListener(std::function<void(i32, ItemStack)> listener) {
    i32 id = m_nextListenerId++;
    m_listeners[id] = std::move(listener);
    return id;
}

void AbstractContainerMenu::removeListener(i32 listenerId) {
    m_listeners.erase(listenerId);
}

void AbstractContainerMenu::notifySlotChanged(i32 slotIndex, const ItemStack& stack) {
    for (auto& pair : m_listeners) {
        pair.second(slotIndex, stack);
    }
}

ItemStack AbstractContainerMenu::clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) {
    (void)button;
    (void)clickType;
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr) {
        return ItemStack();
    }

    // 简化版实现：左键拾取/放置
    ItemStack slotStack = slot->getItem();
    if (m_carried.isEmpty()) {
        // 拾取物品
        if (!slotStack.isEmpty()) {
            m_carried = slotStack;
            slot->set(ItemStack());
        }
    } else {
        // 放置物品
        if (slotStack.isEmpty()) {
            slot->set(m_carried);
            m_carried = ItemStack();
        } else if (slotStack.isSameItem(m_carried)) {
            // 合并物品
            i32 space = slotStack.getMaxStackSize() - slotStack.getCount();
            i32 carriedCount = m_carried.getCount();
            i32 toAdd = (space < carriedCount) ? space : carriedCount;
            slotStack.grow(toAdd);
            slot->set(slotStack);
            m_carried.shrink(toAdd);
        }
    }

    broadcastChanges();
    return m_carried;
}

ItemStack AbstractContainerMenu::quickMoveStack(i32 slotIndex, Player& player) {
    (void)slotIndex;
    (void)player;
    return ItemStack();
}

void AbstractContainerMenu::removed(Player& player) {
    (void)player;
    if (!m_carried.isEmpty() && m_playerInventory) {
        m_carried = ItemStack();
    }
}

} // namespace mr
