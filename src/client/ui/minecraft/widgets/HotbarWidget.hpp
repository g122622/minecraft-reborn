#pragma once

#include "../../kagero/widget/ContainerWidget.hpp"

namespace mc::client::ui::minecraft {

class HotbarWidget : public kagero::widget::ContainerWidget {
public:
    HotbarWidget();

    void paint(kagero::widget::PaintContext& ctx) override;
};

} // namespace mc::client::ui::minecraft
