#include "PaintContext.hpp"

namespace mc::client::ui::kagero::widget {

namespace {

class SolidColorPaint final : public paint::IPaint {
public:
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
    : m_canvas(canvas) {}

paint::ICanvas& PaintContext::canvas() {
    return m_canvas;
}

const paint::ICanvas& PaintContext::canvas() const {
    return m_canvas;
}

void PaintContext::drawTextCentered(const String& text, const Rect& bounds, const paint::IPaint& paint) {
    const f32 x = static_cast<f32>(bounds.centerX());
    const f32 y = static_cast<f32>(bounds.centerY());
    m_canvas.drawText(text, x, y, paint);
}

void PaintContext::drawBorder(const Rect& bounds, f32 width, u32 color) {
    SolidColorPaint borderPaint;
    borderPaint.setStyle(paint::PaintStyle::Stroke);
    borderPaint.setStrokeWidth(width);
    borderPaint.setColor(paint::Color::fromARGB(color));
    m_canvas.drawRect(bounds, borderPaint);
}

void PaintContext::drawFilledRect(const Rect& bounds, u32 color) {
    SolidColorPaint fillPaint;
    fillPaint.setStyle(paint::PaintStyle::Fill);
    fillPaint.setColor(paint::Color::fromARGB(color));
    m_canvas.drawRect(bounds, fillPaint);
}

void PaintContext::drawNinePatch(const paint::IImage& image, const Rect& center, const Rect& dst, u32 tint) {
    SolidColorPaint tintPaint;
    tintPaint.setStyle(paint::PaintStyle::Fill);
    tintPaint.setColor(paint::Color::fromARGB(tint));
    m_canvas.drawImageNine(image, center, dst, &tintPaint);
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
