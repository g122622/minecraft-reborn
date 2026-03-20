#pragma once

#include "GuiSprite.hpp"
#include "../../kagero/paint/IImage.hpp"
#include <unordered_map>
#include <memory>

namespace mc::client::ui::minecraft {

class GuiTextureAtlas {
public:
    void setTexture(std::unique_ptr<kagero::paint::IImage> texture);
    [[nodiscard]] const kagero::paint::IImage* texture() const;

    void addSprite(GuiSprite sprite);
    [[nodiscard]] const GuiSprite* findSprite(const String& id) const;

private:
    std::unique_ptr<kagero::paint::IImage> m_texture;
    std::unordered_map<String, GuiSprite> m_sprites;
};

} // namespace mc::client::ui::minecraft
