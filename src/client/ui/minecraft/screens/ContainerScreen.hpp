#pragma once

#include "Screen.hpp"

namespace mc::client::ui::minecraft {

class ContainerScreen : public Screen {
public:
    ContainerScreen();
    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
