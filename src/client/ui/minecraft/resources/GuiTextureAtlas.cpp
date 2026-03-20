#include "GuiTextureAtlas.hpp"

namespace mc::client::ui::minecraft {

void GuiTextureAtlas::addSprite(GuiSprite sprite) {
    m_sprites[sprite.id] = std::move(sprite);
}

const GuiSprite* GuiTextureAtlas::findSprite(const String& id) const {
    const auto it = m_sprites.find(id);
    return it != m_sprites.end() ? &it->second : nullptr;
}

} // namespace mc::client::ui::minecraft
