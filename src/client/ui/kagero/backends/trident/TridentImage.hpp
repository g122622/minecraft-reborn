#pragma once

#include "../../paint/IImage.hpp"

namespace mc::client::ui::kagero::backends::trident {

class TridentImage final : public paint::IImage {
public:
    TridentImage(i32 width, i32 height, paint::ImageFormat format, String name);

    [[nodiscard]] i32 width() const override;
    [[nodiscard]] i32 height() const override;
    [[nodiscard]] paint::ImageFormat format() const override;
    [[nodiscard]] const String& debugName() const override;

private:
    i32 m_width = 0;
    i32 m_height = 0;
    paint::ImageFormat m_format = paint::ImageFormat::RGBA8;
    String m_name;
};

} // namespace mc::client::ui::kagero::backends::trident
