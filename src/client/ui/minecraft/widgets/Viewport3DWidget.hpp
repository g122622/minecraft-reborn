#pragma once

#include "../../kagero/widget/Viewport3DWidget.hpp"

namespace mc::client::ui::minecraft {

class Viewport3DWidget : public kagero::widget::Viewport3DWidget {
public:
    using kagero::widget::Viewport3DWidget::Viewport3DWidget;

    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
