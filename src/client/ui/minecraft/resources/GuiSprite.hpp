#pragma once

#include "../../kagero/Types.hpp"

namespace mc::client::ui::minecraft {

struct GuiSprite {
    String id;
    kagero::Rect uvRect;
    i32 atlasWidth = 256;
    i32 atlasHeight = 256;
};

} // namespace mc::client::ui::minecraft
