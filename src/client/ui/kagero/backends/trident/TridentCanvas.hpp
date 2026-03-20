#pragma once

#include "../../paint/ICanvas.hpp"
#include <vector>

namespace mc::client::ui::kagero::backends::trident {

/**
 * @brief Trident画布（命令记录实现）
 */
class TridentCanvas final : public paint::ICanvas {
public:
    struct DrawCommand {
        String name;
        Rect bounds;
    };

    TridentCanvas(i32 width, i32 height);

    void drawRect(const Rect& rect, const paint::IPaint& paint) override;
    void drawRRect(const paint::RRect& roundRect, const paint::IPaint& paint) override;
    void drawCircle(f32 cx, f32 cy, f32 radius, const paint::IPaint& paint) override;
    void drawOval(const Rect& bounds, const paint::IPaint& paint) override;
    void drawPath(const paint::IPath& path, const paint::IPaint& paint) override;
    void drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const paint::IPaint& paint) override;

    void drawImage(const paint::IImage& image, f32 x, f32 y) override;
    void drawImageRect(const paint::IImage& image, const Rect& src, const Rect& dst) override;
    void drawImageNine(const paint::IImage& image, const Rect& center, const Rect& dst, const paint::IPaint* paint) override;

    void drawText(const String& text, f32 x, f32 y, const paint::IPaint& paint) override;
    void drawTextBlob(const paint::ITextBlob& blob, f32 x, f32 y, const paint::IPaint& paint) override;

    void clipRect(const Rect& rect) override;
    void clipRRect(const paint::RRect& roundRect) override;
    void clipPath(const paint::IPath& path) override;
    void clipOutRect(const Rect& rect) override;
    [[nodiscard]] bool clipIsEmpty() const override;
    [[nodiscard]] Rect getClipBounds() const override;

    void translate(f32 dx, f32 dy) override;
    void scale(f32 sx, f32 sy) override;
    void rotate(f32 degrees) override;
    void concat(const paint::Matrix& matrix) override;
    void setMatrix(const paint::Matrix& matrix) override;
    [[nodiscard]] paint::Matrix getTotalMatrix() const override;

    i32 save() override;
    void restore() override;
    void restoreToCount(i32 saveCount) override;

    i32 saveLayer(const Rect* bounds, const paint::IPaint* paint) override;
    i32 saveLayerAlpha(const Rect* bounds, u8 alpha) override;

    [[nodiscard]] i32 width() const override;
    [[nodiscard]] i32 height() const override;

    void resize(i32 width, i32 height);
    void clearCommands();
    [[nodiscard]] const std::vector<DrawCommand>& commands() const;

private:
    void pushCommand(String name, const Rect& bounds);

    i32 m_width = 0;
    i32 m_height = 0;
    Rect m_clipBounds;
    paint::Matrix m_matrix;
    std::vector<Rect> m_clipStack;
    std::vector<paint::Matrix> m_matrixStack;
    std::vector<DrawCommand> m_commands;
};

} // namespace mc::client::ui::kagero::backends::trident
