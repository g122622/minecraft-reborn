#include "PaintContext.hpp"
#include <memory>

namespace mc::client::ui::kagero::widget {

namespace {

/**
 * @brief 简单的纯色画笔实现
 *
 * 实现 IPaint 接口，用于 PaintContext 内部缓存。
 */
class SimplePaint final : public paint::IPaint {
public:
    SimplePaint() = default;

    void setColor(const paint::Color& color) override { m_color = color; }
    [[nodiscard]] paint::Color color() const override { return m_color; }

    void setStyle(paint::PaintStyle style) override { m_style = style; }
    [[nodiscard]] paint::PaintStyle style() const override { return m_style; }

    void setStrokeWidth(f32 width) override { m_strokeWidth = width; }
    [[nodiscard]] f32 strokeWidth() const override { return m_strokeWidth; }

    void setStrokeCap(paint::StrokeCap cap) override { m_cap = cap; }
    [[nodiscard]] paint::StrokeCap strokeCap() const override { return m_cap; }

    void setStrokeJoin(paint::StrokeJoin join) override { m_join = join; }
    [[nodiscard]] paint::StrokeJoin strokeJoin() const override { return m_join; }

    void setAntiAlias(bool enabled) override { m_aa = enabled; }
    [[nodiscard]] bool antiAlias() const override { return m_aa; }

    void setAlpha(f32 alpha) override { m_alpha = alpha; }
    [[nodiscard]] f32 alpha() const override { return m_alpha; }

private:
    paint::Color m_color = paint::WHITE;
    paint::PaintStyle m_style = paint::PaintStyle::Fill;
    paint::StrokeCap m_cap = paint::StrokeCap::Butt;
    paint::StrokeJoin m_join = paint::StrokeJoin::Miter;
    f32 m_strokeWidth = 1.0f;
    f32 m_alpha = 1.0f;
    bool m_aa = true;
};

} // namespace

PaintContext::PaintContext(paint::ICanvas& canvas)
    : m_canvas(canvas)
    , m_fillPaint(std::make_unique<SimplePaint>())
    , m_strokePaint(std::make_unique<SimplePaint>()) {
    // 初始化画笔默认样式
    m_fillPaint->setStyle(paint::PaintStyle::Fill);
    m_strokePaint->setStyle(paint::PaintStyle::Stroke);
}

paint::ICanvas& PaintContext::canvas() {
    return m_canvas;
}

const paint::ICanvas& PaintContext::canvas() const {
    return m_canvas;
}

void PaintContext::drawTextCentered(const String& text, const Rect& bounds, u32 color) {
    // 使用缓存的画笔
    m_fillPaint->setColor(paint::Color::fromARGB(color));
    m_canvas.drawText(text, static_cast<f32>(bounds.centerX()), static_cast<f32>(bounds.centerY()), *m_fillPaint);
}

void PaintContext::drawBorder(const Rect& bounds, f32 width, u32 color) {
    m_strokePaint->setColor(paint::Color::fromARGB(color));
    m_strokePaint->setStrokeWidth(width);
    m_canvas.drawRect(bounds, *m_strokePaint);
}

void PaintContext::drawFilledRect(const Rect& bounds, u32 color) {
    m_fillPaint->setColor(paint::Color::fromARGB(color));
    m_canvas.drawRect(bounds, *m_fillPaint);
}

void PaintContext::drawNinePatch(const paint::IImage& image, const Rect& center, const Rect& dst, u32 tint) {
    m_fillPaint->setColor(paint::Color::fromARGB(tint));
    m_canvas.drawImageNine(image, center, dst, m_fillPaint.get());
}

i32 PaintContext::save() {
    return m_canvas.save();
}

void PaintContext::restore() {
    m_canvas.restore();
}

void PaintContext::translate(f32 dx, f32 dy) {
    m_canvas.translate(dx, dy);
}

} // namespace mc::client::ui::kagero::widget
