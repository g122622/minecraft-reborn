#pragma once

#include "../../paint/IPaint.hpp"

namespace mc::client::ui::kagero::backends::trident {

class TridentPaint final : public paint::IPaint {
public:
    TridentPaint() = default;
    ~TridentPaint() override = default;

    void setColor(const paint::Color& color) override;
    [[nodiscard]] paint::Color color() const override;

    void setStyle(paint::PaintStyle style) override;
    [[nodiscard]] paint::PaintStyle style() const override;

    void setStrokeWidth(f32 width) override;
    [[nodiscard]] f32 strokeWidth() const override;

    void setStrokeCap(paint::StrokeCap cap) override;
    [[nodiscard]] paint::StrokeCap strokeCap() const override;

    void setStrokeJoin(paint::StrokeJoin join) override;
    [[nodiscard]] paint::StrokeJoin strokeJoin() const override;

    void setAntiAlias(bool enabled) override;
    [[nodiscard]] bool antiAlias() const override;

    void setAlpha(f32 alpha) override;
    [[nodiscard]] f32 alpha() const override;

private:
    paint::Color m_color = paint::WHITE;
    paint::PaintStyle m_style = paint::PaintStyle::Fill;
    paint::StrokeCap m_cap = paint::StrokeCap::Butt;
    paint::StrokeJoin m_join = paint::StrokeJoin::Miter;
    f32 m_strokeWidth = 1.0f;
    f32 m_alpha = 1.0f;
    bool m_antiAlias = true;
};

} // namespace mc::client::ui::kagero::backends::trident
