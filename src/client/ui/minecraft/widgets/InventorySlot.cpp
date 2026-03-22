#include "InventorySlot.hpp"

namespace mc::client::ui::minecraft {

void InventorySlot::setSlotGroup(String group) {
    m_slotGroup = std::move(group);
}

const String& InventorySlot::slotGroup() const {
    return m_slotGroup;
}

} // namespace mc::client::ui::minecraft
