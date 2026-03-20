#pragma once

#include "../../kagero/widget/Widget.hpp"
#include "../../kagero/paint/PaintContext.hpp"

namespace mc::client::ui::minecraft {

class HungerBarWidget : public kagero::widget::Widget {
public:
    HungerBarWidget();

    void setHunger(i32 hunger);
    [[nodiscard]] i32 hunger() const;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    i32 m_hunger = 20;
};

} // namespace mc::client::ui::minecraft
