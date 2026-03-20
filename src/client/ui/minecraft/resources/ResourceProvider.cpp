#include "ResourceProvider.hpp"
#include "client/ui/Font.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"

namespace mc::client::ui::minecraft {

ResourceProvider::ResourceProvider(Font& font, renderer::trident::gui::GuiRenderer& renderer)
    : m_font(font)
    , m_renderer(renderer) {
}

void ResourceProvider::loadGuiTextureAtlas(const String& path) {
    // TODO: 从资源路径加载纹理图集
    // 需要通过实际的纹理加载器加载
    // 然后注册精灵到 atlas
    (void)path;
}

const GuiTextureAtlas& ResourceProvider::atlas() const {
    return m_atlas;
}

} // namespace mc::client::ui::minecraft
