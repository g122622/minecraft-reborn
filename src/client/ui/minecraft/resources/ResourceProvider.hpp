#pragma once

#include "GuiTextureAtlas.hpp"
#include "MinecraftTypeface.hpp"
#include "../../kagero/backend/IRenderBackend.hpp"
#include <optional>

namespace mc::client::ui::minecraft {

class ResourceProvider {
public:
    explicit ResourceProvider(kagero::backend::IRenderBackend& backend);

    void loadGuiTextureAtlas(const String& path);
    void loadMinecraftTypeface(const String& path);

    [[nodiscard]] const GuiTextureAtlas& atlas() const;
    [[nodiscard]] const MinecraftTypeface* typeface() const;

private:
    kagero::backend::IRenderBackend& m_backend;
    GuiTextureAtlas m_atlas;
    std::optional<MinecraftTypeface> m_typeface;
};

} // namespace mc::client::ui::minecraft
