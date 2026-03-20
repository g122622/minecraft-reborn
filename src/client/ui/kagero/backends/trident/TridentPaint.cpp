#include "TridentPaint.hpp"

#include <algorithm>

namespace mc::client::ui::kagero::backends::trident {

void TridentPaint::setColor(const paint::Color& color) {
    m_color = color;
}

paint::Color TridentPaint::color() const {
    return m_color;
}

void TridentPaint::setStyle(paint::PaintStyle style) {
    m_style = style;
}

paint::PaintStyle TridentPaint::style() const {
    return m_style;
}

void TridentPaint::setStrokeWidth(f32 width) {
    m_strokeWidth = std::max(0.0f, width);
}

f32 TridentPaint::strokeWidth() const {
    return m_strokeWidth;
}

void TridentPaint::setStrokeCap(paint::StrokeCap cap) {
    m_cap = cap;
}

paint::StrokeCap TridentPaint::strokeCap() const {
    return m_cap;
}

void TridentPaint::setStrokeJoin(paint::StrokeJoin join) {
    m_join = join;
}

paint::StrokeJoin TridentPaint::strokeJoin() const {
    return m_join;
}

void TridentPaint::setAntiAlias(bool enabled) {
    m_antiAlias = enabled;
}

bool TridentPaint::antiAlias() const {
    return m_antiAlias;
}

void TridentPaint::setAlpha(f32 alpha) {
    m_alpha = std::clamp(alpha, 0.0f, 1.0f);
}

f32 TridentPaint::alpha() const {
    return m_alpha;
}

} // namespace mc::client::ui::kagero::backends::trident
