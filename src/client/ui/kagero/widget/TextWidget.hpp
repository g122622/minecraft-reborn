#pragma once

#include "Widget.hpp"
#include "PaintContext.hpp"
#include "../../Glyph.hpp"
#include <string>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 文本对齐方式
 */
enum class TextAlignment : u8 {
    Left,       ///< 左对齐
    Center,     ///< 居中
    Right       ///< 右对齐
};

/**
 * @brief 文本组件
 *
 * 显示文本的组件，支持：
 * - 单行和多行文本
 * - 文本对齐
 * - 文本阴影
 * - 文本颜色
 * - 文本换行
 *
 * 使用示例：
 * @code
 * auto text = std::make_unique<TextWidget>("lbl_title", 10, 10, 200, 20);
 * text->setText("Hello World!");
 * text->setColor(Colors::WHITE);
 * text->setShadow(true);
 * @endcode
 */
class TextWidget : public Widget {
public:
    /**
     * @brief 默认构造函数
     */
    TextWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    TextWidget(String id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 构造函数（带文本）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param text 文本内容
     */
    TextWidget(String id, i32 x, i32 y, i32 width, i32 height, String text)
        : Widget(std::move(id))
        , m_text(std::move(text)) {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)ctx;
        (void)mouseX;
        (void)mouseY;
        (void)partialTick;

        if (!isVisible() || m_text.empty()) return;

        // TODO: 实际渲染逻辑（需要FontRenderer）
        // renderText(ctx, mouseX, mouseY, partialTick);
    }

    void paint(PaintContext& ctx) override {
        if (!isVisible() || m_text.empty()) return;

        class TextPaint final : public paint::IPaint {
        public:
            explicit TextPaint(u32 argb) : m_color(paint::Color::fromARGB(argb)) {}
            void setColor(const paint::Color& color) override { m_color = color; }
            [[nodiscard]] paint::Color color() const override { return m_color; }
            void setStyle(paint::PaintStyle style) override { m_style = style; }
            [[nodiscard]] paint::PaintStyle style() const override { return m_style; }
            void setStrokeWidth(f32 width) override { m_width = width; }
            [[nodiscard]] f32 strokeWidth() const override { return m_width; }
            void setStrokeCap(paint::StrokeCap cap) override { m_cap = cap; }
            [[nodiscard]] paint::StrokeCap strokeCap() const override { return m_cap; }
            void setStrokeJoin(paint::StrokeJoin join) override { m_join = join; }
            [[nodiscard]] paint::StrokeJoin strokeJoin() const override { return m_join; }
            void setAntiAlias(bool enabled) override { m_aa = enabled; }
            [[nodiscard]] bool antiAlias() const override { return m_aa; }
            void setAlpha(f32 alpha) override { m_alpha = alpha; }
            [[nodiscard]] f32 alpha() const override { return m_alpha; }
        private:
            paint::Color m_color;
            paint::PaintStyle m_style = paint::PaintStyle::Fill;
            paint::StrokeCap m_cap = paint::StrokeCap::Butt;
            paint::StrokeJoin m_join = paint::StrokeJoin::Miter;
            f32 m_width = 1.0f;
            f32 m_alpha = 1.0f;
            bool m_aa = true;
        } textPaint(m_color);

        ctx.drawTextCentered(m_text, bounds(), textPaint);
    }

    // ==================== 文本属性 ====================

    /**
     * @brief 设置文本
     */
    void setText(const String& text) {
        if (m_text != text) {
            m_text = text;
            m_lines.clear(); // 清除缓存的行
        }
    }

    /**
     * @brief 获取文本
     */
    [[nodiscard]] const String& text() const { return m_text; }

    /**
     * @brief 设置文本颜色
     */
    void setColor(u32 color) {
        m_color = color;
    }

    /**
     * @brief 获取文本颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 设置阴影
     */
    void setShadow(bool shadow) {
        m_shadow = shadow;
    }

    /**
     * @brief 是否有阴影
     */
    [[nodiscard]] bool hasShadow() const { return m_shadow; }

    /**
     * @brief 设置阴影颜色
     */
    void setShadowColor(u32 color) {
        m_shadowColor = color;
    }

    /**
     * @brief 获取阴影颜色
     */
    [[nodiscard]] u32 shadowColor() const { return m_shadowColor; }

    /**
     * @brief 设置对齐方式
     */
    void setAlignment(TextAlignment alignment) {
        m_alignment = alignment;
    }

    /**
     * @brief 获取对齐方式
     */
    [[nodiscard]] TextAlignment alignment() const { return m_alignment; }

    /**
     * @brief 设置最大行数（0表示无限制）
     */
    void setMaxLines(i32 maxLines) {
        m_maxLines = maxLines;
        m_lines.clear();
    }

    /**
     * @brief 获取最大行数
     */
    [[nodiscard]] i32 maxLines() const { return m_maxLines; }

    /**
     * @brief 设置是否启用自动换行
     */
    void setWordWrap(bool wrap) {
        m_wordWrap = wrap;
        m_lines.clear();
    }

    /**
     * @brief 是否启用自动换行
     */
    [[nodiscard]] bool wordWrap() const { return m_wordWrap; }

    /**
     * @brief 设置行高
     */
    void setLineHeight(i32 lineHeight) {
        m_lineHeight = lineHeight;
    }

    /**
     * @brief 获取行高
     */
    [[nodiscard]] i32 lineHeight() const { return m_lineHeight; }

    /**
     * @brief 设置文本缩放
     */
    void setScale(f32 scale) {
        m_scale = scale;
    }

    /**
     * @brief 获取文本缩放
     */
    [[nodiscard]] f32 scale() const { return m_scale; }

    /**
     * @brief 获取文本宽度
     *
     * 计算文本实际渲染宽度
     */
    [[nodiscard]] f32 getTextWidth() const {
        // TODO: 实现字体宽度计算
        // return m_font ? m_font->getStringWidthUTF8(m_text) * m_scale : m_text.size() * 8.0f * m_scale;
        return static_cast<f32>(m_text.size()) * 8.0f * m_scale;
    }

    /**
     * @brief 获取文本高度
     */
    [[nodiscard]] f32 getTextHeight() const {
        i32 lines = m_maxLines > 0 ? std::min(getLineCount(), m_maxLines) : getLineCount();
        return static_cast<f32>(lines * m_lineHeight) * m_scale;
    }

    /**
     * @brief 获取行数
     */
    [[nodiscard]] i32 getLineCount() const {
        if (m_text.empty()) return 0;
        if (!m_wordWrap) {
            return static_cast<i32>(std::count(m_text.begin(), m_text.end(), '\n') + 1);
        }
        // TODO: 实现自动换行的行数计算
        return 1;
    }

    /**
     * @brief 获取指定行的文本
     */
    [[nodiscard]] String getLine(i32 lineIndex) const {
        if (!m_wordWrap) {
            size_t start = 0;
            size_t end = m_text.find('\n');

            for (i32 i = 0; i < lineIndex; ++i) {
                if (end == String::npos) return "";
                start = end + 1;
                end = m_text.find('\n', start);
            }

            if (start >= m_text.size()) return "";
            return m_text.substr(start, end == String::npos ? String::npos : end - start);
        }
        // TODO: 实现自动换行的行获取
        return m_text;
    }

    /**
     * @brief 设置字体
     */
    void setFont(void* font) {
        m_font = font;
    }

    /**
     * @brief 获取字体
     */
    [[nodiscard]] void* font() const { return m_font; }

protected:
    String m_text;                      ///< 文本内容
    u32 m_color = Colors::WHITE;        ///< 文本颜色
    bool m_shadow = true;               ///< 是否有阴影
    u32 m_shadowColor = Colors::MC_DARK_GRAY; ///< 阴影颜色
    TextAlignment m_alignment = TextAlignment::Left; ///< 对齐方式
    i32 m_maxLines = 0;                 ///< 最大行数（0=无限制）
    bool m_wordWrap = false;            ///< 是否启用自动换行
    i32 m_lineHeight = 9;               ///< 行高（MC默认字体高度为9）
    f32 m_scale = 1.0f;                 ///< 文本缩放
    void* m_font = nullptr;             ///< 字体（Font*）
    mutable std::vector<String> m_lines; ///< 缓存的行
};

} // namespace mc::client::ui::kagero::widget
