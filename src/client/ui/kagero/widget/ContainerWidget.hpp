#pragma once

#include "Widget.hpp"
#include "IWidgetContainer.hpp"
#include "PaintContext.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 通用容器控件
 */
class ContainerWidget : public Widget, public WidgetContainerMixin<ContainerWidget> {
public:
    ContainerWidget() = default;
    explicit ContainerWidget(String id)
        : Widget(std::move(id)) {}

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override;

    virtual void paint(PaintContext& ctx);
};

} // namespace mc::client::ui::kagero::widget
