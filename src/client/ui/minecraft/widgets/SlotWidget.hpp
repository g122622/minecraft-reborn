#pragma once

#include "../../kagero/widget/SlotWidget.hpp"

namespace mc::client::ui::minecraft {

class SlotWidget : public kagero::widget::SlotWidget {
public:
    using kagero::widget::SlotWidget::SlotWidget;

    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
