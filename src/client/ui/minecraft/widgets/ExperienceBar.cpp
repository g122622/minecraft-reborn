#include "ExperienceBar.hpp"

namespace mc::client::ui::minecraft {

ExperienceBar::ExperienceBar()
    : Widget("experienceBar") {}

void ExperienceBar::setProgress(f32 progress) {
    m_progress = std::max(0.0f, std::min(1.0f, progress));
}

f32 ExperienceBar::progress() const {
    return m_progress;
}

void ExperienceBar::render(kagero::RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) {
    (void)ctx;
    (void)mouseX;
    (void)mouseY;
    (void)partialTick;
}

void ExperienceBar::paint(kagero::widget::PaintContext& ctx) {
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 20, 40, 0));
    const i32 fillWidth = static_cast<i32>(static_cast<f32>(width()) * m_progress);
    ctx.drawFilledRect(kagero::Rect{x(), y(), fillWidth, height()}, Colors::fromARGB(255, 120, 255, 60));
}

} // namespace mc::client::ui::minecraft
