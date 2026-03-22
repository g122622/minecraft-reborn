#pragma once

#include "Screen.hpp"

namespace mc::client::ui::minecraft {

class InventoryScreen : public Screen {
public:
    InventoryScreen();
    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
