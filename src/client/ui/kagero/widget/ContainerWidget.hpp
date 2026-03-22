#pragma once

#include "Widget.hpp"
#include "IWidgetContainer.hpp"
#include "../paint/PaintContext.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 通用容器控件
 */
class ContainerWidget : public Widget, public WidgetContainerMixin<ContainerWidget> {
public:
    ContainerWidget() = default;
    explicit ContainerWidget(String id)
        : Widget(std::move(id)) {}

    void paint(PaintContext& ctx) override;
};

} // namespace mc::client::ui::kagero::widget
