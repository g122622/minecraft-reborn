#include "TridentTextBlob.hpp"

namespace mc::client::ui::kagero::backends::trident {

TridentTextBlob::TridentTextBlob(String text, const paint::ITypeface& typeface, f32 size)
    : m_text(std::move(text)),
      m_typeface(std::make_shared<TridentTypeface>(typeface.familyName(), typeface.isBold(), typeface.isItalic())),
      m_size(size) {}

const String& TridentTextBlob::text() const {
    return m_text;
}

const paint::ITypeface& TridentTextBlob::typeface() const {
    return *m_typeface;
}

f32 TridentTextBlob::textSize() const {
    return m_size;
}

} // namespace mc::client::ui::kagero::backends::trident
