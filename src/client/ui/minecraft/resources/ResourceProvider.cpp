#include "ResourceProvider.hpp"

namespace mc::client::ui::minecraft {

ResourceProvider::ResourceProvider(kagero::backend::IRenderBackend& backend)
    : m_backend(backend) {}

void ResourceProvider::loadGuiTextureAtlas(const String& path) {
    m_atlas.setTexture(m_backend.loadImage(path));
}

void ResourceProvider::loadMinecraftTypeface(const String& path) {
    m_typeface.emplace(m_backend.loadTypeface(path));
}

const GuiTextureAtlas& ResourceProvider::atlas() const {
    return m_atlas;
}

const MinecraftTypeface* ResourceProvider::typeface() const {
    if (!m_typeface.has_value()) {
        return nullptr;
    }
    return &m_typeface.value();
}

} // namespace mc::client::ui::minecraft
