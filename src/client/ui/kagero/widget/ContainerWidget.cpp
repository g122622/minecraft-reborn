#include "ContainerWidget.hpp"

namespace mc::client::ui::kagero::widget {

void ContainerWidget::render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) {
    (void)ctx;
    // 兼容旧渲染路径：这里只分发给子控件。
    renderChildren(ctx, mouseX, mouseY, partialTick);
}

void ContainerWidget::paint(PaintContext& ctx) {
    // 新绘制路径：由调用方组织遍历。
    (void)ctx;
}

} // namespace mc::client::ui::kagero::widget
