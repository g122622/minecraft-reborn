#include "MainMenuScreen.hpp"

namespace mc::client::ui::minecraft {

MainMenuScreen::MainMenuScreen()
    : Screen("mainMenu") {}

void MainMenuScreen::paint(kagero::widget::PaintContext& ctx) {
    ctx.drawFilledRect(bounds(), Colors::fromARGB(220, 10, 10, 10));
}

} // namespace mc::client::ui::minecraft
