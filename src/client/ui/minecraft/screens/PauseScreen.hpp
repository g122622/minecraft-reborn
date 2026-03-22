#pragma once

#include "Screen.hpp"

namespace mc::client::ui::minecraft {

class PauseScreen : public Screen {
public:
    PauseScreen();
    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
