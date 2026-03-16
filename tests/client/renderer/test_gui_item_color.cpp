#include <gtest/gtest.h>

#include "client/renderer/trident/gui/GuiRenderer.hpp"

namespace mc::client {

TEST(GuiItemColorTest, ItemTextureColorUsesItemSamplerBranch) {
    constexpr u32 color = GuiRenderer::ITEM_TEXTURE_COLOR;
    constexpr u32 alpha = (color >> 24) & 0xFF;

    // alpha=255 会被GUI片段着色器当作字体采样模式
    EXPECT_LT(alpha, 255u);

    // alpha必须接近1，避免物品图标被意外透明
    EXPECT_GE(alpha, 254u);
}

TEST(GuiItemColorTest, ItemTextureColorIsNotTransparent) {
    constexpr u32 color = GuiRenderer::ITEM_TEXTURE_COLOR;
    constexpr u32 alpha = (color >> 24) & 0xFF;
    const f32 outputAlpha = static_cast<f32>(alpha) / 255.0f;

    EXPECT_GT(outputAlpha, 0.99f);
}

} // namespace mc::client
