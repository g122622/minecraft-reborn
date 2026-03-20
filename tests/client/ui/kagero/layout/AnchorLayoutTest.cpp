#include <gtest/gtest.h>
#include "client/ui/kagero/layout/algorithms/AnchorLayout.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;

class AnchorTestWidget : public Widget {
public:
    explicit AnchorTestWidget(String id) : Widget(std::move(id)) {
        setSize(20, 10);
    }
    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)ctx;
        (void)mouseX;
        (void)mouseY;
        (void)partialTick;
    }
};

TEST(AnchorLayoutTest, LeftTopAnchor) {
    AnchorLayout layout;

    AnchorTestWidget widget("w");
    WidgetLayoutAdaptor adaptor(&widget);
    adaptor.constraints().anchor.left = 10;
    adaptor.constraints().anchor.top = 5;

    std::vector<WidgetLayoutAdaptor*> children{&adaptor};
    const auto result = layout.compute(Rect{0, 0, 200, 100}, children);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].bounds.x, 10);
    EXPECT_EQ(result[0].bounds.y, 5);
}
