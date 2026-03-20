#pragma once

#include "../../kagero/widget/Widget.hpp"
#include "../../kagero/paint/PaintContext.hpp"

namespace mc::client::ui::minecraft {

class HungerBarWidget : public kagero::widget::Widget {
public:
    HungerBarWidget();

    void setHunger(i32 hunger);
    [[nodiscard]] i32 hunger() const;

    void render(kagero::RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override;
    void paint(kagero::widget::PaintContext& ctx) override;

private:
    i32 m_hunger = 20;
};

} // namespace mc::client::ui::minecraft
