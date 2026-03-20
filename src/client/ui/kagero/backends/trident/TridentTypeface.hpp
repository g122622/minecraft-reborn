#pragma once

#include "../../paint/ITypeface.hpp"

namespace mc::client::ui::kagero::backends::trident {

class TridentTypeface final : public paint::ITypeface {
public:
    explicit TridentTypeface(String family, bool bold = false, bool italic = false);

    [[nodiscard]] const String& familyName() const override;
    [[nodiscard]] bool isBold() const override;
    [[nodiscard]] bool isItalic() const override;

private:
    String m_family;
    bool m_bold = false;
    bool m_italic = false;
};

} // namespace mc::client::ui::kagero::backends::trident
