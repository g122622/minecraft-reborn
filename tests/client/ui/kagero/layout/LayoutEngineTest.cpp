/**
 * @file LayoutEngineTest.cpp
 * @brief LayoutEngine 和 WidgetLayoutAdaptor 单元测试
 *
 * 测试覆盖率目标：95%+
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/layout/core/LayoutEngine.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/kagero/widget/IWidgetContainer.hpp"

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

    void setTestSize(i32 w, i32 h) {
        setSize(w, h);
    }
};

// ============================================================================
// LayoutResult 测试
// ============================================================================

class LayoutResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LayoutResultTest, DefaultConstruction) {
    LayoutResult result;

    EXPECT_EQ(result.bounds.x, 0);
    EXPECT_EQ(result.bounds.y, 0);
    EXPECT_EQ(result.bounds.width, 0);
    EXPECT_EQ(result.bounds.height, 0);
    EXPECT_TRUE(result.needsRepaint);
    EXPECT_FALSE(result.needsRelayout);
}

TEST_F(LayoutResultTest, ParameterizedConstruction) {
    LayoutResult result(10, 20, 100, 50);

    EXPECT_EQ(result.bounds.x, 10);
    EXPECT_EQ(result.bounds.y, 20);
    EXPECT_EQ(result.bounds.width, 100);
    EXPECT_EQ(result.bounds.height, 50);
}

TEST_F(LayoutResultTest, RectConstruction) {
    Rect rect(5, 10, 200, 100);
    LayoutResult result(rect);

    EXPECT_EQ(result.bounds.x, 5);
    EXPECT_EQ(result.bounds.y, 10);
    EXPECT_EQ(result.bounds.width, 200);
    EXPECT_EQ(result.bounds.height, 100);
}

TEST_F(LayoutResultTest, IsValid) {
    LayoutResult valid(0, 0, 100, 50);
    EXPECT_TRUE(valid.isValid());

    LayoutResult zeroSize(0, 0, 0, 0);
    EXPECT_TRUE(zeroSize.isValid());  // 零尺寸也是有效的

    LayoutResult negativeWidth(0, 0, -10, 50);
    EXPECT_FALSE(negativeWidth.isValid());

    LayoutResult negativeHeight(0, 0, 100, -5);
    EXPECT_FALSE(negativeHeight.isValid());
}

TEST_F(LayoutResultTest, CenterXY) {
    LayoutResult result(0, 0, 100, 50);

    EXPECT_EQ(result.centerX(), 50);
    EXPECT_EQ(result.centerY(), 25);
}

TEST_F(LayoutResultTest, RightBottom) {
    LayoutResult result(10, 20, 100, 50);

    EXPECT_EQ(result.right(), 110);   // x + width
    EXPECT_EQ(result.bottom(), 70);   // y + height
}

TEST_F(LayoutResultTest, Contains) {
    LayoutResult result(10, 20, 100, 50);

    EXPECT_TRUE(result.contains(10, 20));    // 左上角
    EXPECT_TRUE(result.contains(50, 45));    // 中心
    EXPECT_TRUE(result.contains(109, 69));   // 右下角（不含）
    EXPECT_FALSE(result.contains(10, 19));   // 上边界外
    EXPECT_FALSE(result.contains(9, 20));    // 左边界外
    EXPECT_FALSE(result.contains(110, 20));  // 右边界外
    EXPECT_FALSE(result.contains(10, 70));   // 下边界外
}

TEST_F(LayoutResultTest, Merged) {
    LayoutResult result1(0, 0, 100, 50);
    LayoutResult result2(50, 25, 100, 50);

    LayoutResult merged = result1.merged(result2);

    EXPECT_EQ(merged.bounds.x, 0);
    EXPECT_EQ(merged.bounds.y, 0);
    EXPECT_EQ(merged.bounds.width, 150);  // max(right) - min(x)
    EXPECT_EQ(merged.bounds.height, 75);  // max(bottom) - min(y)
}

TEST_F(LayoutResultTest, MergedWithInvalid) {
    LayoutResult valid(0, 0, 100, 50);
    LayoutResult invalid;
    invalid.bounds.width = -10;  // 无效

    LayoutResult merged1 = valid.merged(invalid);
    EXPECT_TRUE(merged1.isValid());
    EXPECT_EQ(merged1.bounds.width, 100);

    LayoutResult merged2 = invalid.merged(valid);
    EXPECT_TRUE(merged2.isValid());
    EXPECT_EQ(merged2.bounds.width, 100);
}

// ============================================================================
// LayoutContext 测试
// ============================================================================

class LayoutContextTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LayoutContextTest, DefaultConstruction) {
    LayoutContext ctx;

    EXPECT_EQ(ctx.availableWidth(), 0);
    EXPECT_EQ(ctx.availableHeight(), 0);
    EXPECT_EQ(ctx.depth(), 0);
}

TEST_F(LayoutContextTest, ParameterizedConstruction) {
    LayoutContext ctx(400, 300);

    EXPECT_EQ(ctx.availableWidth(), 400);
    EXPECT_EQ(ctx.availableHeight(), 300);
}

TEST_F(LayoutContextTest, Setters) {
    LayoutContext ctx;

    ctx.setAvailableWidth(800);
    ctx.setAvailableHeight(600);
    ctx.setDepth(2);

    EXPECT_EQ(ctx.availableWidth(), 800);
    EXPECT_EQ(ctx.availableHeight(), 600);
    EXPECT_EQ(ctx.depth(), 2);
}

TEST_F(LayoutContextTest, CreateChildContext) {
    LayoutContext parent(400, 300);
    parent.setDepth(1);

    LayoutContext child = parent.createChildContext(200, 150);

    EXPECT_EQ(child.availableWidth(), 200);
    EXPECT_EQ(child.availableHeight(), 150);
    EXPECT_EQ(child.depth(), 2);  // 深度+1
}

TEST_F(LayoutContextTest, IsValid) {
    LayoutContext valid(400, 300);
    EXPECT_TRUE(valid.isValid());

    LayoutContext invalid(-10, 300);
    EXPECT_FALSE(invalid.isValid());

    LayoutContext invalid2(400, -5);
    EXPECT_FALSE(invalid2.isValid());
}

// ============================================================================
// LayoutStats 测试
// ============================================================================

class LayoutStatsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LayoutStatsTest, DefaultValues) {
    LayoutStats stats;

    EXPECT_EQ(stats.totalWidgets, 0);
    EXPECT_EQ(stats.relayoutedWidgets, 0);
    EXPECT_EQ(stats.totalTimeMs, 0.0);
    EXPECT_EQ(stats.maxDepth, 0);
    EXPECT_EQ(stats.measureCount, 0);
    EXPECT_EQ(stats.layoutCount, 0);
}

TEST_F(LayoutStatsTest, Reset) {
    LayoutStats stats;
    stats.totalWidgets = 100;
    stats.relayoutedWidgets = 50;
    stats.totalTimeMs = 5.0;
    stats.maxDepth = 10;
    stats.measureCount = 200;
    stats.layoutCount = 100;

    stats.reset();

    EXPECT_EQ(stats.totalWidgets, 0);
    EXPECT_EQ(stats.relayoutedWidgets, 0);
    EXPECT_EQ(stats.totalTimeMs, 0.0);
    EXPECT_EQ(stats.maxDepth, 0);
    EXPECT_EQ(stats.measureCount, 0);
    EXPECT_EQ(stats.layoutCount, 0);
}

// ============================================================================
// WidgetLayoutAdaptor 测试
// ============================================================================

class WidgetLayoutAdaptorTest : public ::testing::Test {
protected:
    void SetUp() override {
        widget = std::make_unique<MockWidget>("testWidget");
        widget->setTestSize(100, 50);
        widget->setMargin(Margin(5, 10));  // 左右5，上下10
        widget->setPadding(Padding(8, 4));  // 左右8，上下4

        adaptor = std::make_unique<WidgetLayoutAdaptor>(widget.get());
    }

    void TearDown() override {
        adaptor.reset();
        widget.reset();
    }

    std::unique_ptr<MockWidget> widget;
    std::unique_ptr<WidgetLayoutAdaptor> adaptor;
};

TEST_F(WidgetLayoutAdaptorTest, Construction) {
    EXPECT_EQ(adaptor->getWidget(), widget.get());
    EXPECT_TRUE(adaptor->isValid());
    EXPECT_EQ(adaptor->id(), "testWidget");
}

TEST_F(WidgetLayoutAdaptorTest, NullWidget) {
    WidgetLayoutAdaptor nullAdaptor(nullptr);
    EXPECT_FALSE(nullAdaptor.isValid());
    EXPECT_EQ(nullAdaptor.id(), "");  // 空ID
}

TEST_F(WidgetLayoutAdaptorTest, CurrentSize) {
    Size size = adaptor->currentSize();
    EXPECT_EQ(size.width, 100);
    EXPECT_EQ(size.height, 50);
}

TEST_F(WidgetLayoutAdaptorTest, CurrentBounds) {
    widget->setBounds(Rect(10, 20, 100, 50));
    Rect bounds = adaptor->currentBounds();
    EXPECT_EQ(bounds.x, 10);
    EXPECT_EQ(bounds.y, 20);
    EXPECT_EQ(bounds.width, 100);
    EXPECT_EQ(bounds.height, 50);
}

TEST_F(WidgetLayoutAdaptorTest, MarginPadding) {
    Margin margin = adaptor->margin();
    EXPECT_EQ(margin.left, 5);
    EXPECT_EQ(margin.top, 10);
    EXPECT_EQ(margin.right, 5);
    EXPECT_EQ(margin.bottom, 10);

    Padding padding = adaptor->padding();
    EXPECT_EQ(padding.left, 8);
    EXPECT_EQ(padding.top, 4);
    EXPECT_EQ(padding.right, 8);
    EXPECT_EQ(padding.bottom, 4);
}

TEST_F(WidgetLayoutAdaptorTest, Constraints) {
    // 默认约束
    EXPECT_EQ(adaptor->constraints().minWidth, 0);
    EXPECT_EQ(adaptor->constraints().minHeight, 0);

    // 设置约束
    adaptor->constraints().minWidth = 50;
    adaptor->constraints().minHeight = 30;
    adaptor->constraints().preferredWidth = 200;
    adaptor->constraints().preferredHeight = 100;

    EXPECT_EQ(adaptor->constraints().minWidth, 50);
    EXPECT_EQ(adaptor->constraints().minHeight, 30);
    EXPECT_EQ(adaptor->constraints().preferredWidth, 200);
    EXPECT_EQ(adaptor->constraints().preferredHeight, 100);
}

TEST_F(WidgetLayoutAdaptorTest, FlexItem) {
    adaptor->flexItem().grow = 1.0f;
    adaptor->flexItem().shrink = 0.5f;
    adaptor->flexItem().basis = 100;

    EXPECT_FLOAT_EQ(adaptor->flexItem().grow, 1.0f);
    EXPECT_FLOAT_EQ(adaptor->flexItem().shrink, 0.5f);
    EXPECT_EQ(adaptor->flexItem().basis, 100);
}

TEST_F(WidgetLayoutAdaptorTest, MeasureExactly) {
    MeasureSpec widthSpec = MeasureSpec::MakeExactly(200);
    MeasureSpec heightSpec = MeasureSpec::MakeExactly(100);

    Size size = adaptor->measure(widthSpec, heightSpec);

    // Exactly模式应该返回规格尺寸
    EXPECT_EQ(size.width, 200);
    EXPECT_EQ(size.height, 100);
}

TEST_F(WidgetLayoutAdaptorTest, MeasureAtMost) {
    adaptor->constraints().preferredWidth = 150;
    adaptor->constraints().preferredHeight = 80;

    MeasureSpec widthSpec = MeasureSpec::MakeAtMost(200);
    MeasureSpec heightSpec = MeasureSpec::MakeAtMost(100);

    Size size = adaptor->measure(widthSpec, heightSpec);

    // AtMost模式，期望值小于规格，应返回期望值
    EXPECT_EQ(size.width, 150);
    EXPECT_EQ(size.height, 80);
}

TEST_F(WidgetLayoutAdaptorTest, MeasureAtMostClamp) {
    adaptor->constraints().preferredWidth = 250;  // 大于规格
    adaptor->constraints().preferredHeight = 120;

    MeasureSpec widthSpec = MeasureSpec::MakeAtMost(200);
    MeasureSpec heightSpec = MeasureSpec::MakeAtMost(100);

    Size size = adaptor->measure(widthSpec, heightSpec);

    // 期望值大于规格，应返回规格值
    EXPECT_EQ(size.width, 200);
    EXPECT_EQ(size.height, 100);
}

TEST_F(WidgetLayoutAdaptorTest, MeasureUnspecified) {
    adaptor->constraints().preferredWidth = -1;  // 无偏好
    adaptor->constraints().preferredHeight = -1;

    MeasureSpec widthSpec = MeasureSpec::MakeUnspecified();
    MeasureSpec heightSpec = MeasureSpec::MakeUnspecified();

    Size size = adaptor->measure(widthSpec, heightSpec);

    // 无偏好时应返回当前尺寸或最小尺寸
    EXPECT_GE(size.width, 0);
    EXPECT_GE(size.height, 0);
}

TEST_F(WidgetLayoutAdaptorTest, MeasureCache) {
    MeasureSpec widthSpec = MeasureSpec::MakeExactly(200);
    MeasureSpec heightSpec = MeasureSpec::MakeExactly(100);

    // 第一次测量
    Size size1 = adaptor->measure(widthSpec, heightSpec);

    // 第二次相同规格应该使用缓存
    Size size2 = adaptor->measure(widthSpec, heightSpec);

    EXPECT_EQ(size1.width, size2.width);
    EXPECT_EQ(size1.height, size2.height);

    // 缓存的结果
    Size cached = adaptor->lastMeasuredSize();
    EXPECT_EQ(cached.width, size1.width);
    EXPECT_EQ(cached.height, size1.height);
}

TEST_F(WidgetLayoutAdaptorTest, ApplyLayout) {
    LayoutResult result(10, 20, 150, 80);

    adaptor->applyLayout(result);

    Rect bounds = adaptor->currentBounds();
    EXPECT_EQ(bounds.x, 10);
    EXPECT_EQ(bounds.y, 20);
    EXPECT_EQ(bounds.width, 150);
    EXPECT_EQ(bounds.height, 80);

    // 应用后应该清除脏标记
    EXPECT_FALSE(adaptor->isLayoutDirty());
}

TEST_F(WidgetLayoutAdaptorTest, ApplyLayoutSimplified) {
    adaptor->applyLayout(30, 40, 200, 100);

    Rect bounds = adaptor->currentBounds();
    EXPECT_EQ(bounds.x, 30);
    EXPECT_EQ(bounds.y, 40);
    EXPECT_EQ(bounds.width, 200);
    EXPECT_EQ(bounds.height, 100);
}

TEST_F(WidgetLayoutAdaptorTest, DirtyFlags) {
    // 初始应该是脏的（需要布局）
    EXPECT_TRUE(adaptor->isLayoutDirty());

    // 应用布局后应该清除
    adaptor->applyLayout(0, 0, 100, 50);
    EXPECT_FALSE(adaptor->isLayoutDirty());

    // 请求重新布局
    adaptor->requestLayout();
    EXPECT_TRUE(adaptor->isLayoutDirty());

    // 渲染脏标记
    adaptor->requestRender();
    EXPECT_TRUE(adaptor->isRenderDirty());

    // 清除
    adaptor->clearLayoutDirty();
    adaptor->clearRenderDirty();
    EXPECT_FALSE(adaptor->isLayoutDirty());
    EXPECT_FALSE(adaptor->isRenderDirty());
}

TEST_F(WidgetLayoutAdaptorTest, CustomMeasureFunc) {
    adaptor->setMeasureFunc([](WidgetLayoutAdaptor* a, const MeasureSpec& w, const MeasureSpec& h) -> Size {
        (void)a;
        return Size(w.size / 2, h.size / 2);  // 返回规格的一半
    });

    MeasureSpec widthSpec = MeasureSpec::MakeExactly(200);
    MeasureSpec heightSpec = MeasureSpec::MakeExactly(100);

    Size size = adaptor->measure(widthSpec, heightSpec);

    EXPECT_EQ(size.width, 100);
    EXPECT_EQ(size.height, 50);

    adaptor->clearMeasureFunc();
}

TEST_F(WidgetLayoutAdaptorTest, ChildCount) {
    // MockWidget 不是容器
    EXPECT_EQ(adaptor->childCount(), 0u);
    EXPECT_FALSE(adaptor->isContainer());
}

TEST_F(WidgetLayoutAdaptorTest, Depth) {
    EXPECT_EQ(adaptor->depth(), 0);

    adaptor->setDepth(5);
    EXPECT_EQ(adaptor->depth(), 5);
}

TEST_F(WidgetLayoutAdaptorTest, ConstraintsClamp) {
    adaptor->constraints().minWidth = 50;
    adaptor->constraints().maxWidth = 200;
    adaptor->constraints().minHeight = 30;
    adaptor->constraints().maxHeight = 150;

    MeasureSpec widthSpec = MeasureSpec::MakeExactly(250);  // 超过最大值
    MeasureSpec heightSpec = MeasureSpec::MakeExactly(20);  // 低于最小值

    Size size = adaptor->measure(widthSpec, heightSpec);

    // 应该被约束到min/max范围
    // 注意：实际行为取决于实现细节
    EXPECT_GE(size.width, adaptor->constraints().minWidth);
    EXPECT_LE(size.width, adaptor->constraints().maxWidth);
    EXPECT_GE(size.height, adaptor->constraints().minHeight);
    EXPECT_LE(size.height, adaptor->constraints().maxHeight);
}

TEST_F(WidgetLayoutAdaptorTest, LayoutEnabled) {
    EXPECT_TRUE(adaptor->constraints().isLayoutEnabled());

    adaptor->constraints().enabled = false;
    EXPECT_FALSE(adaptor->constraints().isLayoutEnabled());

    // 禁用的元素不应该参与布局
    // 实际行为在布局算法中体现
}

TEST_F(WidgetLayoutAdaptorTest, AspectRatio) {
    adaptor->constraints().aspectRatio = 2.0f;  // 宽高比 2:1

    MeasureSpec widthSpec = MeasureSpec::MakeExactly(200);
    MeasureSpec heightSpec = MeasureSpec::MakeUnspecified();

    Size size = adaptor->measure(widthSpec, heightSpec);

    // 宽高比应该约为 2:1
    // 注意：实际实现可能有所不同
    EXPECT_FLOAT_EQ(static_cast<f32>(size.width) / static_cast<f32>(size.height), 2.0f);
}

// ============================================================================
// LayoutEngine 测试
// ============================================================================

class LayoutEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = &LayoutEngine::instance();
        engine->resetStats();
    }

    void TearDown() override {
        engine->resetStats();
    }

    LayoutEngine* engine;
};

TEST_F(LayoutEngineTest, Instance) {
    LayoutEngine& instance1 = LayoutEngine::instance();
    LayoutEngine& instance2 = LayoutEngine::instance();

    EXPECT_EQ(&instance1, &instance2);  // 单例
}

TEST_F(LayoutEngineTest, RegisterAlgorithm) {
    auto algorithm = std::make_unique<FlexLayoutAlgorithm>();
    engine->registerAlgorithm("test_algo", std::move(algorithm));

    EXPECT_TRUE(engine->hasAlgorithm("test_algo"));
    EXPECT_NE(engine->getAlgorithm("test_algo"), nullptr);
}

TEST_F(LayoutEngineTest, GetNonexistentAlgorithm) {
    ILayoutAlgorithm* algo = engine->getAlgorithm("nonexistent");
    EXPECT_EQ(algo, nullptr);

    EXPECT_FALSE(engine->hasAlgorithm("nonexistent"));
}

TEST_F(LayoutEngineTest, BuiltInAlgorithms) {
    EXPECT_TRUE(engine->hasAlgorithm("flex"));
    EXPECT_TRUE(engine->hasAlgorithm("flex-row"));
    EXPECT_TRUE(engine->hasAlgorithm("flex-column"));
    EXPECT_TRUE(engine->hasAlgorithm("flex-center"));
}

TEST_F(LayoutEngineTest, LayoutEmpty) {
    MockWidget widget("root");
    WidgetLayoutAdaptor adaptor(&widget);

    engine->layout(&adaptor, Rect(0, 0, 400, 300));

    // 布局应该成功完成
    EXPECT_GT(engine->getLastStats().totalTimeMs, 0.0);
}

TEST_F(LayoutEngineTest, LayoutStats) {
    MockWidget widget("root");
    widget.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor(&widget);

    engine->layout(&adaptor, Rect(0, 0, 400, 300));

    const LayoutStats& stats = engine->getLastStats();
    EXPECT_EQ(stats.totalWidgets, 1);
    EXPECT_GE(stats.measureCount, 1);
}

TEST_F(LayoutEngineTest, LayoutFlex) {
    MockWidget container("container");
    WidgetLayoutAdaptor adaptor(&container);

    FlexConfig config;
    config.direction = Direction::Row;
    config.justifyContent = JustifyContent::Center;
    config.gap = 10;

    engine->layoutFlex(&adaptor, Rect(0, 0, 400, 300), config);

    // 布局应该成功
    EXPECT_GE(engine->getLastStats().layoutCount, 1);
}

TEST_F(LayoutEngineTest, DebugVisualize) {
    engine->setDebugVisualize(true);
    EXPECT_TRUE(engine->isDebugVisualize());

    engine->setDebugVisualize(false);
    EXPECT_FALSE(engine->isDebugVisualize());
}

TEST_F(LayoutEngineTest, Profiling) {
    engine->setProfiling(true);
    EXPECT_TRUE(engine->isProfiling());

    engine->setProfiling(false);
    EXPECT_FALSE(engine->isProfiling());
}

// ============================================================================
// FlexLayoutAlgorithm 测试
// ============================================================================

class FlexLayoutAlgorithmTest : public ::testing::Test {
protected:
    void SetUp() override {
        algorithm = std::make_unique<FlexLayoutAlgorithm>();
    }

    void TearDown() override {
        algorithm.reset();
    }

    std::unique_ptr<FlexLayoutAlgorithm> algorithm;
};

TEST_F(FlexLayoutAlgorithmTest, Name) {
    EXPECT_EQ(algorithm->name(), "flex");
}

TEST_F(FlexLayoutAlgorithmTest, Compute) {
    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };
    Rect container(0, 0, 400, 300);

    auto results = algorithm->compute(container, children, LayoutConstraints{});

    EXPECT_EQ(results.size(), 2u);
}

TEST_F(FlexLayoutAlgorithmTest, Measure) {
    MockWidget widget1("w1");
    widget1.setTestSize(100, 50);
    WidgetLayoutAdaptor adaptor1(&widget1);

    MockWidget widget2("w2");
    widget2.setTestSize(80, 60);
    WidgetLayoutAdaptor adaptor2(&widget2);

    std::vector<WidgetLayoutAdaptor*> children = { &adaptor1, &adaptor2 };

    Size size = algorithm->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    // 宽度应该包含两个元素
    EXPECT_GE(size.width, 180);
    // 高度应该是最大元素高度
    EXPECT_GE(size.height, 60);
}

TEST_F(FlexLayoutAlgorithmTest, Config) {
    FlexConfig config;
    config.direction = Direction::Column;
    config.gap = 15;
    config.justifyContent = JustifyContent::SpaceAround;

    algorithm->setConfig(config);

    const FlexConfig& retrieved = algorithm->config();
    EXPECT_EQ(retrieved.direction, Direction::Column);
    EXPECT_EQ(retrieved.gap, 15);
    EXPECT_EQ(retrieved.justifyContent, JustifyContent::SpaceAround);
}

// ============================================================================
// LayoutType 枚举测试
// ============================================================================

TEST(LayoutTypeTest, EnumValues) {
    EXPECT_EQ(static_cast<int>(LayoutType::None), 0);
    EXPECT_EQ(static_cast<int>(LayoutType::Flex), 1);
    EXPECT_EQ(static_cast<int>(LayoutType::Grid), 2);
    EXPECT_EQ(static_cast<int>(LayoutType::Anchor), 3);
    EXPECT_EQ(static_cast<int>(LayoutType::Stack), 4);
}
