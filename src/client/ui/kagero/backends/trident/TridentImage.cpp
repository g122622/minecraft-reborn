#include "TridentImage.hpp"

namespace mc::client::ui::kagero::backends::trident {

TridentImage::TridentImage(i32 width, i32 height, paint::ImageFormat format, String name)
    : m_width(width), m_height(height), m_format(format), m_name(std::move(name)) {}

i32 TridentImage::width() const {
    return m_width;
}

i32 TridentImage::height() const {
    return m_height;
}

paint::ImageFormat TridentImage::format() const {
    return m_format;
}

const String& TridentImage::debugName() const {
    return m_name;
}

} // namespace mc::client::ui::kagero::backends::trident
