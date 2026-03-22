#include "ContainerWidget.hpp"

namespace mc::client::ui::kagero::widget {

void ContainerWidget::paint(PaintContext& ctx) {
    paintChildren(ctx);
}

} // namespace mc::client::ui::kagero::widget
