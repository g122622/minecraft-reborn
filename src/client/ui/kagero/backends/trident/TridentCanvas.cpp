#include "TridentCanvas.hpp"

#include <algorithm>

namespace mc::client::ui::kagero::backends::trident {

TridentCanvas::TridentCanvas(i32 width, i32 height)
    : m_width(width),
      m_height(height),
      m_clipBounds{0, 0, width, height},
      m_matrix(paint::Matrix::identity()) {}

void TridentCanvas::drawRect(const Rect& rect, const paint::IPaint& paint) {
    (void)paint;
    pushCommand("drawRect", rect);
}

void TridentCanvas::drawRRect(const paint::RRect& roundRect, const paint::IPaint& paint) {
    (void)paint;
    pushCommand("drawRRect", roundRect.rect);
}

void TridentCanvas::drawCircle(f32 cx, f32 cy, f32 radius, const paint::IPaint& paint) {
    (void)paint;
    const i32 left = static_cast<i32>(cx - radius);
    const i32 top = static_cast<i32>(cy - radius);
    const i32 size = static_cast<i32>(radius * 2.0f);
    pushCommand("drawCircle", Rect{left, top, size, size});
}

void TridentCanvas::drawOval(const Rect& bounds, const paint::IPaint& paint) {
    (void)paint;
    pushCommand("drawOval", bounds);
}

void TridentCanvas::drawPath(const paint::IPath& path, const paint::IPaint& paint) {
    (void)path;
    (void)paint;
    pushCommand("drawPath", m_clipBounds);
}

void TridentCanvas::drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const paint::IPaint& paint) {
    (void)paint;
    const i32 minX = static_cast<i32>(std::min(x0, x1));
    const i32 minY = static_cast<i32>(std::min(y0, y1));
    const i32 maxX = static_cast<i32>(std::max(x0, x1));
    const i32 maxY = static_cast<i32>(std::max(y0, y1));
    pushCommand("drawLine", Rect{minX, minY, maxX - minX, maxY - minY});
}

void TridentCanvas::drawImage(const paint::IImage& image, f32 x, f32 y) {
    pushCommand("drawImage:" + image.debugName(), Rect{static_cast<i32>(x), static_cast<i32>(y), image.width(), image.height()});
}

void TridentCanvas::drawImageRect(const paint::IImage& image, const Rect& src, const Rect& dst) {
    (void)src;
    pushCommand("drawImageRect:" + image.debugName(), dst);
}

void TridentCanvas::drawImageNine(const paint::IImage& image, const Rect& center, const Rect& dst, const paint::IPaint* paint) {
    (void)center;
    (void)paint;
    pushCommand("drawImageNine:" + image.debugName(), dst);
}

void TridentCanvas::drawText(const String& text, f32 x, f32 y, const paint::IPaint& paint) {
    (void)paint;
    pushCommand("drawText:" + text, Rect{static_cast<i32>(x), static_cast<i32>(y), 0, 0});
}

void TridentCanvas::drawTextBlob(const paint::ITextBlob& blob, f32 x, f32 y, const paint::IPaint& paint) {
    (void)paint;
    pushCommand("drawTextBlob:" + blob.text(), Rect{static_cast<i32>(x), static_cast<i32>(y), 0, 0});
}

void TridentCanvas::clipRect(const Rect& rect) {
    m_clipBounds = m_clipBounds.intersection(rect);
}

void TridentCanvas::clipRRect(const paint::RRect& roundRect) {
    clipRect(roundRect.rect);
}

void TridentCanvas::clipPath(const paint::IPath& path) {
    (void)path;
}

void TridentCanvas::clipOutRect(const Rect& rect) {
    (void)rect;
}

bool TridentCanvas::clipIsEmpty() const {
    return !m_clipBounds.isValid();
}

Rect TridentCanvas::getClipBounds() const {
    return m_clipBounds;
}

void TridentCanvas::translate(f32 dx, f32 dy) {
    m_matrix.m[2] += dx;
    m_matrix.m[5] += dy;
}

void TridentCanvas::scale(f32 sx, f32 sy) {
    m_matrix.m[0] *= sx;
    m_matrix.m[4] *= sy;
}

void TridentCanvas::rotate(f32 degrees) {
    (void)degrees;
}

void TridentCanvas::concat(const paint::Matrix& matrix) {
    m_matrix = matrix;
}

void TridentCanvas::setMatrix(const paint::Matrix& matrix) {
    m_matrix = matrix;
}

paint::Matrix TridentCanvas::getTotalMatrix() const {
    return m_matrix;
}

i32 TridentCanvas::save() {
    m_clipStack.push_back(m_clipBounds);
    m_matrixStack.push_back(m_matrix);
    return static_cast<i32>(m_clipStack.size());
}

void TridentCanvas::restore() {
    if (!m_clipStack.empty()) {
        m_clipBounds = m_clipStack.back();
        m_clipStack.pop_back();
    }
    if (!m_matrixStack.empty()) {
        m_matrix = m_matrixStack.back();
        m_matrixStack.pop_back();
    }
}

void TridentCanvas::restoreToCount(i32 saveCount) {
    while (static_cast<i32>(m_clipStack.size()) > saveCount) {
        restore();
    }
}

i32 TridentCanvas::saveLayer(const Rect* bounds, const paint::IPaint* paint) {
    (void)paint;
    if (bounds != nullptr) {
        clipRect(*bounds);
    }
    return save();
}

i32 TridentCanvas::saveLayerAlpha(const Rect* bounds, u8 alpha) {
    (void)alpha;
    if (bounds != nullptr) {
        clipRect(*bounds);
    }
    return save();
}

i32 TridentCanvas::width() const {
    return m_width;
}

i32 TridentCanvas::height() const {
    return m_height;
}

void TridentCanvas::resize(i32 width, i32 height) {
    m_width = width;
    m_height = height;
    m_clipBounds = Rect{0, 0, width, height};
}

void TridentCanvas::clearCommands() {
    m_commands.clear();
}

const std::vector<TridentCanvas::DrawCommand>& TridentCanvas::commands() const {
    return m_commands;
}

void TridentCanvas::pushCommand(String name, const Rect& bounds) {
    m_commands.push_back(DrawCommand{std::move(name), bounds});
}

} // namespace mc::client::ui::kagero::backends::trident
