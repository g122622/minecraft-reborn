#pragma once

#include "Screen.hpp"

namespace mc::client::ui::minecraft {

class OptionsScreen : public Screen {
public:
    OptionsScreen();
    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
