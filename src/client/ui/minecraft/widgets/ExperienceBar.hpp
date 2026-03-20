#pragma once

#include "../../kagero/widget/Widget.hpp"
#include "../../kagero/paint/PaintContext.hpp"

namespace mc::client::ui::minecraft {

class ExperienceBar : public kagero::widget::Widget {
public:
    ExperienceBar();

    void setProgress(f32 progress);
    [[nodiscard]] f32 progress() const;

    void render(kagero::RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override;
    void paint(kagero::widget::PaintContext& ctx) override;

private:
    f32 m_progress = 0.0f;
};

} // namespace mc::client::ui::minecraft
