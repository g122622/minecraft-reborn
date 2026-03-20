#pragma once

#include "../../kagero/widget/Widget.hpp"
#include "../../kagero/paint/PaintContext.hpp"

namespace mc::client::ui::minecraft {

class HealthBarWidget : public kagero::widget::Widget {
public:
    HealthBarWidget();

    void setHealth(i32 health);
    [[nodiscard]] i32 health() const;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    i32 m_health = 20;
};

} // namespace mc::client::ui::minecraft
