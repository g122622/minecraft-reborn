#include "Screen.hpp"

namespace mc::client::ui::minecraft {

Screen::Screen(String id)
    : ContainerWidget(std::move(id)) {}

void Screen::onOpen() {}

void Screen::onClose() {}

void Screen::render(kagero::RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) {
    ContainerWidget::render(ctx, mouseX, mouseY, partialTick);
}

bool Screen::isModal() const {
    return m_modal;
}

void Screen::setModal(bool modal) {
    m_modal = modal;
}

} // namespace mc::client::ui::minecraft
