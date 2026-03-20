#pragma once

#include "../../paint/ISurface.hpp"
#include "TridentCanvas.hpp"

namespace mc::client::ui::kagero::backends::trident {

class TridentSurface final : public paint::ISurface {
public:
    TridentSurface(i32 width, i32 height);

    [[nodiscard]] paint::ICanvas& canvas() override;
    [[nodiscard]] const paint::ICanvas& canvas() const override;

    void flush() override;
    void resize(i32 width, i32 height) override;

    [[nodiscard]] i32 width() const override;
    [[nodiscard]] i32 height() const override;

private:
    TridentCanvas m_canvas;
};

} // namespace mc::client::ui::kagero::backends::trident
