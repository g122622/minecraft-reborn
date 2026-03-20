#pragma once

#include "../../kagero/paint/ITypeface.hpp"
#include <memory>

namespace mc::client::ui::minecraft {

class MinecraftTypeface {
public:
    explicit MinecraftTypeface(std::unique_ptr<kagero::paint::ITypeface> typeface);

    [[nodiscard]] const kagero::paint::ITypeface* get() const;

private:
    std::unique_ptr<kagero::paint::ITypeface> m_typeface;
};

} // namespace mc::client::ui::minecraft
