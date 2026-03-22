/**
 * @file ButtonWidgetTest.cpp
 * @brief ButtonWidget单元测试
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/Glyph.hpp"

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(ButtonWidgetTest, DefaultConstructor) {
    ButtonWidget button;
    EXPECT_TRUE(button.id().empty());
    EXPECT_TRUE(button.text().empty());
}

TEST(ButtonWidgetTest, ConstructorWithParams) {
    ButtonWidget button("btn_test", 10, 20, 100, 40, "Click Me");

    EXPECT_EQ("btn_test", button.id());
    EXPECT_EQ("Click Me", button.text());
    EXPECT_EQ(10, button.x());
    EXPECT_EQ(20, button.y());
    EXPECT_EQ(100, button.width());
    EXPECT_EQ(40, button.height());
}

TEST(ButtonWidgetTest, ConstructorWithCallback) {
    bool clicked = false;
    ButtonWidget button("btn_test", 10, 20, 100, 40, "Click Me",
        [&clicked](ButtonWidget&) { clicked = true; });

    EXPECT_EQ("btn_test", button.id());
    EXPECT_EQ("Click Me", button.text());

    // 触发点击
    button.onClick(50, 30, 0);
    EXPECT_TRUE(clicked);
}

// ==================== 文本测试 ====================

TEST(ButtonWidgetTest, SetText) {
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Initial");
    EXPECT_EQ("Initial", button.text());

    button.setText("Updated");
    EXPECT_EQ("Updated", button.text());
}

// ==================== 样式测试 ====================

TEST(ButtonWidgetTest, SetStyle) {
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test");

    ButtonWidget::Style style;
    style.normalColor = fromARGB(255, 60, 120, 200);
    style.hoverColor = fromARGB(255, 80, 140, 220);
    style.textColor = WHITE;
    style.cornerRadius = 8;

    button.setStyle(style);

    EXPECT_EQ(style.normalColor, button.style().normalColor);
    EXPECT_EQ(style.hoverColor, button.style().hoverColor);
    EXPECT_EQ(style.textColor, button.style().textColor);
    EXPECT_EQ(8, button.style().cornerRadius);
}

// ==================== 状态测试 ====================

TEST(ButtonWidgetTest, GetRenderState) {
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test");

    // 正常状态
    EXPECT_EQ(1, button.getRenderState());

    // 禁用状态
    button.setActive(false);
    EXPECT_EQ(0, button.getRenderState());

    // 悬停状态
    button.setActive(true);
    button.setHovered(true);
    EXPECT_EQ(2, button.getRenderState());
}

TEST(ButtonWidgetTest, GetBackgroundColor) {
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test");

    ButtonWidget::Style style;
    style.normalColor = fromARGB(255, 60, 60, 60);
    style.hoverColor = fromARGB(255, 80, 80, 80);
    style.disabledColor = fromARGB(255, 40, 40, 40);
    button.setStyle(style);

    // 正常状态
    EXPECT_EQ(style.normalColor, button.getBackgroundColor());

    // 悬停状态
    button.setHovered(true);
    EXPECT_EQ(style.hoverColor, button.getBackgroundColor());

    // 禁用状态
    button.setActive(false);
    EXPECT_EQ(style.disabledColor, button.getBackgroundColor());
}

TEST(ButtonWidgetTest, GetTextColor) {
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test");

    ButtonWidget::Style style;
    style.textColor = WHITE;
    style.disabledTextColor = fromARGB(255, 128, 128, 128);
    button.setStyle(style);

    // 正常状态
    EXPECT_EQ(style.textColor, button.getTextColor());

    // 禁用状态
    button.setActive(false);
    EXPECT_EQ(style.disabledTextColor, button.getTextColor());
}

// ==================== 点击测试 ====================

TEST(ButtonWidgetTest, ClickTriggersCallback) {
    int clickCount = 0;
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test",
        [&clickCount](ButtonWidget&) { ++clickCount; });

    button.setActive(true);
    button.setVisible(true);

    // 左键点击
    bool result = button.onClick(50, 20, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(1, clickCount);

    // 右键点击不触发
    result = button.onClick(50, 20, 1);
    EXPECT_FALSE(result);
    EXPECT_EQ(1, clickCount);
}

TEST(ButtonWidgetTest, ClickDisabledNoTrigger) {
    int clickCount = 0;
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test",
        [&clickCount](ButtonWidget&) { ++clickCount; });

    button.setActive(false);

    bool result = button.onClick(50, 20, 0);
    EXPECT_FALSE(result);
    EXPECT_EQ(0, clickCount);
}

TEST(ButtonWidgetTest, ClickInvisibleNoTrigger) {
    int clickCount = 0;
    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test",
        [&clickCount](ButtonWidget&) { ++clickCount; });

    button.setVisible(false);

    bool result = button.onClick(50, 20, 0);
    EXPECT_FALSE(result);
    EXPECT_EQ(0, clickCount);
}

// ==================== ImageButtonWidget测试 ====================

TEST(ImageButtonWidgetTest, Constructor) {
    ImageButtonWidget button("img_btn", 10, 20, 40, 40, 0, 0, 40, "textures/gui/buttons.png");

    EXPECT_EQ("img_btn", button.id());
    EXPECT_EQ(10, button.x());
    EXPECT_EQ(20, button.y());
    EXPECT_EQ(40, button.width());
    EXPECT_EQ(40, button.height());
}

TEST(ImageButtonWidgetTest, SetTextureCoords) {
    ImageButtonWidget button("img_btn", 0, 0, 40, 40, 0, 0, 40, "texture.png");
    button.setTextureCoords(10, 20, 50);

    // 验证设置成功（无法直接访问私有成员，但可以验证不崩溃）
    EXPECT_TRUE(button.isActive());
}
