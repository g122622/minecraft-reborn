#include "TridentTypeface.hpp"

namespace mc::client::ui::kagero::backends::trident {

TridentTypeface::TridentTypeface(String family, bool bold, bool italic)
    : m_family(std::move(family)), m_bold(bold), m_italic(italic) {}

const String& TridentTypeface::familyName() const {
    return m_family;
}

bool TridentTypeface::isBold() const {
    return m_bold;
}

bool TridentTypeface::isItalic() const {
    return m_italic;
}

} // namespace mc::client::ui::kagero::backends::trident
