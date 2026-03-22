#pragma once

#include "Screen.hpp"

namespace mc::client::ui::minecraft {

class MainMenuScreen : public Screen {
public:
    MainMenuScreen();
    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
