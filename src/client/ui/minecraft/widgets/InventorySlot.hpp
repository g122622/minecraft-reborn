#pragma once

#include "SlotWidget.hpp"

namespace mc::client::ui::minecraft {

class InventorySlot : public SlotWidget {
public:
    using SlotWidget::SlotWidget;

    void setSlotGroup(String group);
    [[nodiscard]] const String& slotGroup() const;

private:
    String m_slotGroup = "inventory";
};

} // namespace mc::client::ui::minecraft
