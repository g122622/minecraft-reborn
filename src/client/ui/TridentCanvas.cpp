#include "TridentCanvas.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/FontRenderer.hpp"
#include <algorithm>
#include <cmath>

namespace mc::client::ui {

TridentCanvas::TridentCanvas(renderer::trident::gui::GuiRenderer& renderer, Font& font)
    : m_renderer(renderer)
    , m_font(font)
    , m_clipBounds{0, 0, 0, 0}
    , m_matrix(kagero::paint::Matrix::identity()) {
}

void TridentCanvas::beginFrame() {
    m_clipBounds = kagero::Rect{0, 0, m_width, m_height};
    m_matrix = kagero::paint::Matrix::identity();
    m_clipStack.clear();
    m_matrixStack.clear();
    m_alphaStack.clear();
}

void TridentCanvas::endFrame() {
    // 所有绘制操作已直接提交到 GuiRenderer
    // 无需额外处理
}

void TridentCanvas::drawRect(const kagero::Rect& rect, const kagero::paint::IPaint& paint) {
    const u32 color = extractColor(paint);
    const kagero::paint::PaintStyle style = paint.style();

    f32 x1 = static_cast<f32>(rect.x);
    f32 y1 = static_cast<f32>(rect.y);
    f32 x2 = x1 + static_cast<f32>(rect.width);
    f32 y2 = y1 + static_cast<f32>(rect.height);

    // 应用变换
    transformPoint(x1, y1);
    transformPoint(x2, y2);

    // 确保坐标顺序正确
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (style == kagero::paint::PaintStyle::Fill) {
        m_renderer.fillRect(x1, y1, x2 - x1, y2 - y1, color);
    } else if (style == kagero::paint::PaintStyle::Stroke) {
        const f32 strokeWidth = paint.strokeWidth();
        m_renderer.drawRect(x1, y1, x2 - x1, y2 - y1, color);
    }
}

void TridentCanvas::drawRRect(const kagero::paint::RRect& roundRect, const kagero::paint::IPaint& paint) {
    // 简化实现：暂时退化为普通矩形
    // TODO: 实现真正的圆角矩形绘制
    drawRect(roundRect.rect, paint);
}

void TridentCanvas::drawCircle(f32 cx, f32 cy, f32 radius, const kagero::paint::IPaint& paint) {
    // 简化实现：使用近似多边形
    // TODO: 实现真正的圆形绘制
    const u32 color = extractColor(paint);
    transformPoint(cx, cy);

    // 目前退化为边界矩形
    kagero::Rect bounds{
        static_cast<i32>(cx - radius),
        static_cast<i32>(cy - radius),
        static_cast<i32>(radius * 2),
        static_cast<i32>(radius * 2)
    };

    if (paint.style() == kagero::paint::PaintStyle::Fill) {
        m_renderer.fillRect(
            static_cast<f32>(bounds.x),
            static_cast<f32>(bounds.y),
            static_cast<f32>(bounds.width),
            static_cast<f32>(bounds.height),
            color
        );
    }
}

void TridentCanvas::drawOval(const kagero::Rect& bounds, const kagero::paint::IPaint& paint) {
    // 简化实现：退化为普通矩形
    // TODO: 实现真正的椭圆绘制
    drawRect(bounds, paint);
}

void TridentCanvas::drawPath(const kagero::paint::IPath& path, const kagero::paint::IPaint& paint) {
    // 路径绘制暂不实现
    // TODO: 实现路径绘制
    (void)path;
    (void)paint;
}

void TridentCanvas::drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const kagero::paint::IPaint& paint) {
    const u32 color = extractColor(paint);
    const f32 strokeWidth = paint.strokeWidth();

    transformPoint(x0, y0);
    transformPoint(x1, y1);

    // 计算线条方向的垂直方向
    const f32 dx = x1 - x0;
    const f32 dy = y1 - y0;
    const f32 len = std::sqrt(dx * dx + dy * dy);

    if (len < 0.001f) {
        // 长度太短，绘制一个点
        m_renderer.fillRect(x0 - strokeWidth / 2, y0 - strokeWidth / 2, strokeWidth, strokeWidth, color);
        return;
    }

    // 垂直方向的半宽
    const f32 halfWidth = strokeWidth / 2.0f;
    const f32 nx = -dy / len * halfWidth;
    const f32 ny = dx / len * halfWidth;

    // 绘制一个四边形作为线条
    // 顶点: (x0+nx, y0+ny), (x0-nx, y0-ny), (x1-nx, y1-ny), (x1+nx, y1+ny)
    // 简化：使用 fillRect 近似
    const f32 minX = std::min(x0, x1) - halfWidth;
    const f32 minY = std::min(y0, y1) - halfWidth;
    const f32 maxX = std::max(x0, x1) + halfWidth;
    const f32 maxY = std::max(y0, y1) + halfWidth;

    m_renderer.fillRect(minX, minY, maxX - minX, maxY - minY, color);
}

void TridentCanvas::drawImage(const kagero::paint::IImage& image, f32 x, f32 y) {
    transformPoint(x, y);
    // TODO: 实现图像绘制
    // 目前 GuiRenderer 需要 TextureRegion 信息
    (void)image;
}

void TridentCanvas::drawImageRect(const kagero::paint::IImage& image, const kagero::Rect& src, const kagero::Rect& dst) {
    f32 x = static_cast<f32>(dst.x);
    f32 y = static_cast<f32>(dst.y);
    transformPoint(x, y);

    // TODO: 实现图像绘制
    (void)image;
    (void)src;
}

void TridentCanvas::drawImageNine(const kagero::paint::IImage& image, const kagero::Rect& center, const kagero::Rect& dst, const kagero::paint::IPaint* paint) {
    // TODO: 实现 Nine-patch 绘制
    (void)image;
    (void)center;
    (void)dst;
    (void)paint;
}

void TridentCanvas::drawText(const String& text, f32 x, f32 y, const kagero::paint::IPaint& paint) {
    const u32 color = extractColor(paint);
    transformPoint(x, y);

    // 转换 String (std::u32string) 到 UTF-8
    std::string utf8Text;
    utf8Text.reserve(text.size());
    for (char32_t c : text) {
        if (c < 0x80) {
            utf8Text.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            utf8Text.push_back(static_cast<char>(0xC0 | (c >> 6)));
            utf8Text.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            utf8Text.push_back(static_cast<char>(0xE0 | (c >> 12)));
            utf8Text.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8Text.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            utf8Text.push_back(static_cast<char>(0xF0 | (c >> 18)));
            utf8Text.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            utf8Text.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8Text.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }

    m_renderer.drawText(utf8Text, x, y, color, false);
}

void TridentCanvas::drawTextBlob(const kagero::paint::ITextBlob& blob, f32 x, f32 y, const kagero::paint::IPaint& paint) {
    const u32 color = extractColor(paint);
    transformPoint(x, y);

    // 将 TextBlob 的文本转换为 UTF-8 并绘制
    const String& text = blob.text();
    std::string utf8Text;
    utf8Text.reserve(text.size());
    for (char32_t c : text) {
        if (c < 0x80) {
            utf8Text.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            utf8Text.push_back(static_cast<char>(0xC0 | (c >> 6)));
            utf8Text.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            utf8Text.push_back(static_cast<char>(0xE0 | (c >> 12)));
            utf8Text.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8Text.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            utf8Text.push_back(static_cast<char>(0xF0 | (c >> 18)));
            utf8Text.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            utf8Text.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8Text.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }

    m_renderer.drawText(utf8Text, x, y, color, false);
}

void TridentCanvas::clipRect(const kagero::Rect& rect) {
    m_clipBounds = m_clipBounds.intersection(rect);
}

void TridentCanvas::clipRRect(const kagero::paint::RRect& roundRect) {
    // 简化实现：退化为矩形裁剪
    clipRect(roundRect.rect);
}

void TridentCanvas::clipPath(const kagero::paint::IPath& path) {
    // 路径裁剪暂不实现
    (void)path;
}

void TridentCanvas::clipOutRect(const kagero::Rect& rect) {
    // 反向裁剪暂不实现
    (void)rect;
}

bool TridentCanvas::clipIsEmpty() const {
    return !m_clipBounds.isValid();
}

kagero::Rect TridentCanvas::getClipBounds() const {
    return m_clipBounds;
}

void TridentCanvas::translate(f32 dx, f32 dy) {
    // 3x3 仿射矩阵的平移
    m_matrix.m[2] += dx;
    m_matrix.m[5] += dy;
}

void TridentCanvas::scale(f32 sx, f32 sy) {
    // 3x3 仿射矩阵的缩放
    m_matrix.m[0] *= sx;
    m_matrix.m[4] *= sy;
}

void TridentCanvas::rotate(f32 degrees) {
    // 旋转暂不实现
    // TODO: 实现完整的矩阵旋转
    (void)degrees;
}

void TridentCanvas::concat(const kagero::paint::Matrix& matrix) {
    // 矩阵乘法暂不实现
    // TODO: 实现完整的矩阵乘法
    m_matrix = matrix;
}

void TridentCanvas::setMatrix(const kagero::paint::Matrix& matrix) {
    m_matrix = matrix;
}

kagero::paint::Matrix TridentCanvas::getTotalMatrix() const {
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
    if (!m_alphaStack.empty()) {
        m_alphaStack.pop_back();
    }
}

void TridentCanvas::restoreToCount(i32 saveCount) {
    while (static_cast<i32>(m_clipStack.size()) > saveCount) {
        restore();
    }
}

i32 TridentCanvas::saveLayer(const kagero::Rect* bounds, const kagero::paint::IPaint* paint) {
    if (bounds != nullptr) {
        clipRect(*bounds);
    }
    // TODO: 实现图层保存
    (void)paint;
    return save();
}

i32 TridentCanvas::saveLayerAlpha(const kagero::Rect* bounds, u8 alpha) {
    if (bounds != nullptr) {
        clipRect(*bounds);
    }
    m_alphaStack.push_back(static_cast<f32>(alpha) / 255.0f);
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
    m_clipBounds = kagero::Rect{0, 0, width, height};
}

void TridentCanvas::transformPoint(f32& x, f32& y) const {
    // 应用 3x3 仿射矩阵变换
    // | m[0] m[1] m[2] |   | x |   | x' |
    // | m[3] m[4] m[5] | * | y | = | y' |
    // | m[6] m[7] m[8] |   | 1 |   | 1  |
    const f32 nx = m_matrix.m[0] * x + m_matrix.m[1] * y + m_matrix.m[2];
    const f32 ny = m_matrix.m[3] * x + m_matrix.m[4] * y + m_matrix.m[5];
    x = nx;
    y = ny;
}

u32 TridentCanvas::extractColor(const kagero::paint::IPaint& paint) const {
    kagero::paint::Color color = paint.color();

    // 应用 alpha 堆栈
    f32 alpha = color.a;
    for (f32 a : m_alphaStack) {
        alpha *= a;
    }

    // 转换为 ARGB
    const u32 aa = static_cast<u32>(alpha * 255.0f) & 0xFF;
    const u32 rr = static_cast<u32>(color.r * 255.0f) & 0xFF;
    const u32 gg = static_cast<u32>(color.g * 255.0f) & 0xFF;
    const u32 bb = static_cast<u32>(color.b * 255.0f) & 0xFF;

    return (aa << 24) | (rr << 16) | (gg << 8) | bb;
}

} // namespace mc::client::ui
