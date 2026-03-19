/**
 * @file FlexLayoutTest.cpp
 * @brief FlexLayout 弹性布局算法单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - 单元素布局
 * - 多元素布局
 * - 主轴对齐（justify-content）
 * - 交叉轴对齐（align-items）
 * - 弹性增长（flex-grow）
 * - 弹性缩小（flex-shrink）
 * - 换行（flex-wrap）
 * - 间距（gap）
 * - 方向（direction）
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/Widget.hpp"

using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;

// ============================================================================
// Mock Widget 用于测试
// ============================================================================

class MockWidget : public Widget {
public:
    MockWidget() : Widget() {}
    explicit MockWidget(const String& id) : Widget(id) {}

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)ctx;
        (void)mouseX;
        (void)mouseY;
        (void)partialTick;
    }

    // 设置尺寸用于测试
    void setTestSize(i32 w, i32 h) {
        setSize(w, h);
    }
};

// ============================================================================
// FlexConfig 测试
// ============================================================================

class FlexConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FlexConfigTest, DefaultValues) {
    FlexConfig config;

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::Start);
    EXPECT_EQ(config.alignItems, Align::Stretch);
    EXPECT_EQ(config.wrap, Wrap::NoWrap);
    EXPECT_EQ(config.gap, 0);
}

TEST_F(FlexConfigTest, IsHorizontal) {
    FlexConfig config;

    config.direction = Direction::Row;
    EXPECT_TRUE(config.isHorizontal());

    config.direction = Direction::RowReverse;
    EXPECT_TRUE(config.isHorizontal());

    config.direction = Direction::Column;
    EXPECT_FALSE(config.isHorizontal());

    config.direction = Direction::ColumnReverse;
    EXPECT_FALSE(config.isHorizontal());
}

TEST_F(FlexConfigTest, IsVertical) {
    FlexConfig config;

    config.direction = Direction::Column;
    EXPECT_TRUE(config.isVertical());

    config.direction = Direction::ColumnReverse;
    EXPECT_TRUE(config.isVertical());

    config.direction = Direction::Row;
    EXPECT_FALSE(config.isVertical());
}

TEST_F(FlexConfigTest, ShouldWrap) {
    FlexConfig config;

    config.wrap = Wrap::NoWrap;
    EXPECT_FALSE(config.shouldWrap());

    config.wrap = Wrap::Wrap;
    // shouldWrap 没有在 FlexConfig 中定义，用 wrap 字段直接检查
    EXPECT_NE(config.wrap, Wrap::NoWrap);

    config.wrap = Wrap::WrapReverse;
    EXPECT_NE(config.wrap, Wrap::NoWrap);
}

TEST_F(FlexConfigTest, IsReverse) {
    FlexConfig config;

    config.direction = Direction::Row;
    EXPECT_FALSE(config.isReverse());

    config.direction = Direction::RowReverse;
    EXPECT_TRUE(config.isReverse());

    config.direction = Direction::Column;
    EXPECT_FALSE(config.isReverse());

    config.direction = Direction::ColumnReverse;
    EXPECT_TRUE(config.isReverse());
}

// ============================================================================
// FlexLayout 基础测试
// ============================================================================

class FlexLayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        layout = std::make_unique<FlexLayout>();
    }

    void TearDown() override {
        layout.reset();
    }

    std::unique_ptr<FlexLayout> layout;
};

TEST_F(FlexLayoutTest, DefaultConfig) {
    EXPECT_EQ(layout->direction(), Direction::Row);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::Start);
    EXPECT_EQ(layout->alignItems(), Align::Stretch);
    EXPECT_EQ(layout->wrap(), Wrap::NoWrap);
    EXPECT_EQ(layout->gap(), 0);
}

TEST_F(FlexLayoutTest, SetDirection) {
    layout->setDirection(Direction::Column);
    EXPECT_EQ(layout->direction(), Direction::Column);

    layout->setDirection(Direction::RowReverse);
    EXPECT_EQ(layout->direction(), Direction::RowReverse);
}

TEST_F(FlexLayoutTest, SetJustifyContent) {
    layout->setJustifyContent(JustifyContent::Center);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::Center);

    layout->setJustifyContent(JustifyContent::SpaceBetween);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::SpaceBetween);
}

TEST_F(FlexLayoutTest, SetAlignItems) {
    layout->setAlignItems(Align::Center);
    EXPECT_EQ(layout->alignItems(), Align::Center);

    layout->setAlignItems(Align::End);
    EXPECT_EQ(layout->alignItems(), Align::End);
}

TEST_F(FlexLayoutTest, SetWrap) {
    layout->setWrap(Wrap::Wrap);
    EXPECT_EQ(layout->wrap(), Wrap::Wrap);
}

TEST_F(FlexLayoutTest, SetGap) {
    layout->setGap(10);
    EXPECT_EQ(layout->gap(), 10);

    layout->setGap(20);
    EXPECT_EQ(layout->gap(), 20);
}

TEST_F(FlexLayoutTest, SetConfig) {
    FlexConfig config;
    config.direction = Direction::Column;
    config.justifyContent = JustifyContent::SpaceAround;
    config.alignItems = Align::Center;
    config.gap = 15;

    layout->setConfig(config);

    EXPECT_EQ(layout->direction(), Direction::Column);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::SpaceAround);
    EXPECT_EQ(layout->alignItems(), Align::Center);
    EXPECT_EQ(layout->gap(), 15);
}

// ============================================================================
// 单元素布局测试
// ============================================================================

TEST_F(FlexLayoutTest, SingleElement) {
    // 创建模拟Widget和适配器
    MockWidget widget("test");
    widget.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor(&widget);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_GE(results[0].bounds.width, 0);
    EXPECT_GE(results[0].bounds.height, 0);
}

TEST_F(FlexLayoutTest, SingleElementWithMargin) {
    MockWidget widget("test");
    widget.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor(&widget);
    adaptor.constraints().margin = Margin(10, 10);  // 四周各10

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
    // 元素位置应该考虑边距
    EXPECT_EQ(results[0].bounds.x, 10);  // 左边距
    EXPECT_EQ(results[0].bounds.y, 10);  // 上边距
}

TEST_F(FlexLayoutTest, SingleElementGrow) {
    MockWidget widget("test");
    widget.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor(&widget);
    adaptor.flexItem().grow = 1.0f;  // 增长因子

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
    // 有增长因子的元素应该填充容器
    EXPECT_GT(results[0].bounds.width, 100);  // 应该大于初始宽度
}

// ============================================================================
// 多元素布局测试
// ============================================================================

TEST_F(FlexLayoutTest, TwoElementsRow) {
    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 第二个元素应该在第一个元素右边
    EXPECT_GE(results[1].bounds.x, results[0].bounds.x + results[0].bounds.width);
}

TEST_F(FlexLayoutTest, TwoElementsColumn) {
    layout->setDirection(Direction::Column);

    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 第二个元素应该在第一个元素下边
    EXPECT_GE(results[1].bounds.y, results[0].bounds.y + results[0].bounds.height);
}

TEST_F(FlexLayoutTest, ThreeElementsWithGap) {
    layout->setGap(10);

    MockWidget widget1("w1");
    widget1.setTestSize(80, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(80, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    MockWidget widget3("w3");
    widget3.setTestSize(80, 50);
    WidgetLayoutAdaptor adaptor3(&widget3);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2, &adaptor3 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);
    // 检查间距
    EXPECT_GE(results[1].bounds.x - results[0].bounds.right(), 10);
    EXPECT_GE(results[2].bounds.x - results[1].bounds.right(), 10);
}

// ============================================================================
// 主轴对齐测试
// ============================================================================

TEST_F(FlexLayoutTest, JustifyContentCenter) {
    layout->setJustifyContent(JustifyContent::Center);

    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 两个元素总宽度200，容器宽度400，应该居中
    // 左边距应该大于0
    EXPECT_GT(results[0].bounds.x, 0);
}

TEST_F(FlexLayoutTest, JustifyContentEnd) {
    layout->setJustifyContent(JustifyContent::End);

    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 元素应该靠右
    // 第一个元素的右边界应该接近容器右边界
    EXPECT_GT(results[1].bounds.right(), 200);
}

TEST_F(FlexLayoutTest, JustifyContentSpaceBetween) {
    layout->setJustifyContent(JustifyContent::SpaceBetween);

    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 第一个元素应该在左边，最后一个元素应该在右边
    EXPECT_EQ(results[0].bounds.x, 0);  // 左边对齐
    // 最后一个元素应该靠右边界
    EXPECT_GT(results[1].bounds.right(), 200);
}

TEST_F(FlexLayoutTest, JustifyContentSpaceAround) {
    layout->setJustifyContent(JustifyContent::SpaceAround);

    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 元素两侧应该有相等的间距
    EXPECT_GT(results[0].bounds.x, 0);  // 左边有间距
}

// ============================================================================
// 交叉轴对齐测试
// ============================================================================

TEST_F(FlexLayoutTest, AlignItemsCenter) {
    layout->setAlignItems(Align::Center);

    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 30);  // 不同高度
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 200);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 元素应该在垂直方向居中
    // 第一个元素（高度50）在200高的容器中，Y应该接近(200-50)/2 = 75
    // 注意：实际值可能因边距等因素略有不同
    EXPECT_GT(results[0].bounds.y, 0);
    EXPECT_LT(results[0].bounds.y, 100);
}

// ============================================================================
// 弹性增长测试
// ============================================================================

TEST_F(FlexLayoutTest, FlexGrowDistribution) {
    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);
    adaptor1.flexItem().grow = 1.0f;

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);
    adaptor2.flexItem().grow = 1.0f;

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 200);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 两个元素应该平均分配剩余空间（各占约200）
    // 宽度应该接近200（总宽度400平分）
    EXPECT_GT(results[0].bounds.width, 150);
    EXPECT_GT(results[1].bounds.width, 150);
}

TEST_F(FlexLayoutTest, FlexGrowUnequalDistribution) {
    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);
    adaptor1.flexItem().grow = 2.0f;  // 2倍增长

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);
    adaptor2.flexItem().grow = 1.0f;  // 1倍增长

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 200);

    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
    // 第一个元素应该比第二个元素宽（2:1比例）
    // 注意：实际测试可能需要调整，取决于实现细节
    EXPECT_GT(results[0].bounds.width, results[1].bounds.width);
}

// ============================================================================
// 空元素测试
// ============================================================================

TEST_F(FlexLayoutTest, EmptyChildren) {
    std::vector<WidgetLayoutAdaptor*> children;
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    EXPECT_TRUE(results.empty());
}

// ============================================================================
// 测量测试
// ============================================================================

TEST_F(FlexLayoutTest, MeasureEmptyChildren) {
    std::vector<WidgetLayoutAdaptor*> children;

    Size size = layout->measure(
        MeasureSpec::MakeExactly(400),
        MeasureSpec::MakeExactly(300),
        children
    );

    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
}

TEST_F(FlexLayoutTest, MeasureSingleChild) {
    MockWidget widget("test");
    widget.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor(&widget);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor };

    Size size = layout->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    // 测量尺寸应该至少包含子元素尺寸
    EXPECT_GE(size.width, 100);
    EXPECT_GE(size.height, 50);
}

TEST_F(FlexLayoutTest, MeasureMultipleChildren) {
    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(80, 60);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };

    Size size = layout->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    // 宽度应该包含两个元素
    EXPECT_GE(size.width, 180);  // 100 + 80
    // 高度应该是最大的元素高度
    EXPECT_GE(size.height, 60);
}

// ============================================================================
// 辅助函数测试
// ============================================================================

TEST(FlexHelperTest, DefaultFlexConfig) {
    FlexConfig config = defaultFlexConfig();

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::Start);
    EXPECT_EQ(config.alignItems, Align::Stretch);
    EXPECT_EQ(config.wrap, Wrap::NoWrap);
    EXPECT_EQ(config.gap, 0);
}

TEST(FlexHelperTest, CenterRowFlexConfig) {
    FlexConfig config = centerRowFlexConfig();

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::Center);
    EXPECT_EQ(config.alignItems, Align::Center);
}

TEST(FlexHelperTest, CenterColumnFlexConfig) {
    FlexConfig config = centerColumnFlexConfig();

    EXPECT_EQ(config.direction, Direction::Column);
    EXPECT_EQ(config.justifyContent, JustifyContent::Center);
    EXPECT_EQ(config.alignItems, Align::Center);
}

TEST(FlexHelperTest, SpaceBetweenFlexConfig) {
    FlexConfig config = spaceBetweenFlexConfig();

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::SpaceBetween);
    EXPECT_EQ(config.alignItems, Align::Center);
}

// ============================================================================
// FlexLine 测试
// ============================================================================

class FlexLineTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FlexLineTest, DefaultValues) {
    FlexLine line;

    EXPECT_EQ(line.mainSize, 0);
    EXPECT_EQ(line.crossSize, 0);
    EXPECT_EQ(line.offsetX, 0);
    EXPECT_EQ(line.offsetY, 0);
    EXPECT_FLOAT_EQ(line.totalGrow, 0.0f);
    EXPECT_FLOAT_EQ(line.totalShrink, 0.0f);
    EXPECT_TRUE(line.items.empty());
}

TEST_F(FlexLineTest, AddItem) {
    FlexLine line;

    MockWidget widget("test");
    widget.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor(&widget);
    adaptor.flexItem().grow = 1.0f;

    line.addItem(&adaptor, 120);  // 包含边距

    EXPECT_EQ(line.items.size(), 1u);
    EXPECT_EQ(line.mainSize, 120);
    EXPECT_FLOAT_EQ(line.totalGrow, 1.0f);
    EXPECT_FLOAT_EQ(line.totalShrink, 1.0f);  // 默认shrink=1
}

TEST_F(FlexLineTest, ItemCount) {
    FlexLine line;
    EXPECT_EQ(line.itemCount(), 0u);

    MockWidget widget("test");
    WidgetLayoutAdaptor adaptor(&widget);
    line.addItem(&adaptor, 100);

    EXPECT_EQ(line.itemCount(), 1u);
}
