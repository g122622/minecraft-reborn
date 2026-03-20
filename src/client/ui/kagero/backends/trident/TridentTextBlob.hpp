#pragma once

#include "../../paint/ITextBlob.hpp"
#include "TridentTypeface.hpp"
#include <memory>

namespace mc::client::ui::kagero::backends::trident {

class TridentTextBlob final : public paint::ITextBlob {
public:
    TridentTextBlob(String text, const paint::ITypeface& typeface, f32 size);

    [[nodiscard]] const String& text() const override;
    [[nodiscard]] const paint::ITypeface& typeface() const override;
    [[nodiscard]] f32 textSize() const override;

private:
    String m_text;
    std::shared_ptr<TridentTypeface> m_typeface;
    f32 m_size = 12.0f;
};

} // namespace mc::client::ui::kagero::backends::trident
