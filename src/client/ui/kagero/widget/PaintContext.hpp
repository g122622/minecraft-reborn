#pragma once

#include "../paint/ICanvas.hpp"
#include "../paint/IImage.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 绘图上下文
 */
class PaintContext {
public:
    explicit PaintContext(paint::ICanvas& canvas);

    [[nodiscard]] paint::ICanvas& canvas();
    [[nodiscard]] const paint::ICanvas& canvas() const;

    void drawTextCentered(const String& text, const Rect& bounds, const paint::IPaint& paint);
    void drawBorder(const Rect& bounds, f32 width, u32 color);
    void drawFilledRect(const Rect& bounds, u32 color);
    void drawNinePatch(const paint::IImage& image, const Rect& center, const Rect& dst, u32 tint = 0xFFFFFFFF);

    i32 save();
    void restore();
    void translate(f32 dx, f32 dy);

private:
    paint::ICanvas& m_canvas;
};

} // namespace mc::client::ui::kagero::widget
