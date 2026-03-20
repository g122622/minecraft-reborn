#include "PauseScreen.hpp"

namespace mc::client::ui::minecraft {

PauseScreen::PauseScreen()
    : Screen("pause") {}

void PauseScreen::paint(kagero::widget::PaintContext& ctx) {
    ctx.drawFilledRect(bounds(), Colors::fromARGB(180, 0, 0, 0));
}

} // namespace mc::client::ui::minecraft
