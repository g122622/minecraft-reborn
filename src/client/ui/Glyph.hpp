#pragma once

#include "../../common/core/Types.hpp"
#include <array>

namespace mr::client {

/**
 * @brief 字形数据结构
 *
 * 存储单个字符的渲染信息，包括纹理坐标和度量数据。
 * 参考Minecraft的IGlyphInfo接口设计。
 */
struct Glyph {
    u32 codepoint = 0;          // Unicode码点
    f32 u0 = 0.0f, v0 = 0.0f;   // 纹理左上角UV
    f32 u1 = 0.0f, v1 = 0.0f;   // 纹理右下角UV
    f32 advance = 0.0f;          // 字符宽度（光标前进距离）
    f32 bearingX = 0.0f;         // 字形左侧到光标的水平偏移
    f32 bearingY = 0.0f;         // 字形顶部到基线的垂直偏移
    f32 width = 0.0f;            // 字形像素宽度
    f32 height = 0.0f;           // 字形像素高度

    Glyph() = default;

    /**
     * @brief 构造字形
     * @param cp Unicode码点
     * @param u0_ 纹理左上角U
     * @param v0_ 纹理左上角V
     * @param u1_ 纹理右下角U
     * @param v1_ 纹理右下角V
     * @param adv 前进宽度
     * @param bx 水平偏移
     * @param by 垂直偏移
     * @param w 字形宽度
     * @param h 字形高度
     */
    Glyph(u32 cp, f32 u0_, f32 v0_, f32 u1_, f32 v1_,
          f32 adv, f32 bx, f32 by, f32 w, f32 h)
        : codepoint(cp)
        , u0(u0_), v0(v0_), u1(u1_), v1(v1_)
        , advance(adv)
        , bearingX(bx), bearingY(by)
        , width(w), height(h) {}

    /**
     * @brief 获取粗体偏移量
     * MC中粗体通过额外绘制一次偏移后的字形实现
     */
    [[nodiscard]] static constexpr f32 getBoldOffset() { return 1.0f; }

    /**
     * @brief 获取阴影偏移量
     * MC中阴影通过偏移绘制实现
     */
    [[nodiscard]] static constexpr f32 getShadowOffset() { return 1.0f; }
};

/**
 * @brief 空字形（用于空格等不可见字符）
 */
struct EmptyGlyph : public Glyph {
    EmptyGlyph() : Glyph(' ', 0, 0, 0, 0, 4.0f, 0, 0, 0, 0) {}
};

/**
 * @brief 字形顶点 (用于UI渲染)
 *
 * 2D屏幕空间顶点，包含位置、UV和颜色
 */
struct GuiVertex {
    f32 x, y;       // 屏幕坐标 (像素)
    f32 u, v;       // 纹理坐标
    u32 color;      // RGBA颜色

    GuiVertex() = default;
    GuiVertex(f32 px, f32 py, f32 pu, f32 pv, u32 col)
        : x(px), y(py), u(pu), v(pv), color(col) {}
};

/**
 * @brief GUI矩形 (用于绘制填充矩形)
 */
struct GuiRect {
    f32 x, y;       // 左上角
    f32 width, height;
    u32 color;      // RGBA颜色

    GuiRect() = default;
    GuiRect(f32 px, f32 py, f32 w, f32 h, u32 col = 0xFFFFFFFF)
        : x(px), y(py), width(w), height(h), color(col) {}
};

/**
 * @brief 文本样式
 */
struct TextStyle {
    u32 color = 0xFFFFFFFF;         // 文本颜色
    bool shadow = true;             // 是否绘制阴影
    bool bold = false;              // 粗体
    bool italic = false;            // 斜体
    bool strikethrough = false;     // 删除线
    bool underline = false;         // 下划线

    TextStyle() = default;
    explicit TextStyle(u32 col, bool sh = true)
        : color(col), shadow(sh) {}
};

/**
 * @brief 颜色常量 (参考MC的颜色系统)
 */
namespace Colors {
    constexpr u32 WHITE       = 0xFFFFFFFF;
    constexpr u32 BLACK       = 0xFF000000;
    constexpr u32 RED         = 0xFFFF0000;
    constexpr u32 GREEN       = 0xFF00FF00;
    constexpr u32 BLUE        = 0xFF0000FF;
    constexpr u32 YELLOW      = 0xFFFFFF00;
    constexpr u32 CYAN        = 0xFF00FFFF;
    constexpr u32 MAGENTA     = 0xFFFF00FF;
    constexpr u32 GRAY        = 0xFF808080;
    constexpr u32 DARK_GRAY   = 0xFF404040;
    constexpr u32 LIGHT_GRAY  = 0xFFC0C0C0;
    constexpr u32 ORANGE      = 0xFFFFA500;

    // MC聊天颜色
    constexpr u32 MC_BLACK        = 0xFF000000;
    constexpr u32 MC_DARK_BLUE    = 0xFF0000AA;
    constexpr u32 MC_DARK_GREEN   = 0xFF00AA00;
    constexpr u32 MC_DARK_AQUA    = 0xFF00AAAA;
    constexpr u32 MC_DARK_RED     = 0xFFAA0000;
    constexpr u32 MC_DARK_PURPLE  = 0xFFAA00AA;
    constexpr u32 MC_GOLD         = 0xFFFFAA00;
    constexpr u32 MC_GRAY         = 0xFFAAAAAA;
    constexpr u32 MC_DARK_GRAY    = 0xFF555555;
    constexpr u32 MC_BLUE         = 0xFF5555FF;
    constexpr u32 MC_GREEN        = 0xFF55FF55;
    constexpr u32 MC_AQUA         = 0xFF55FFFF;
    constexpr u32 MC_RED          = 0xFFFF5555;
    constexpr u32 MC_LIGHT_PURPLE = 0xFFFF55FF;
    constexpr u32 MC_YELLOW       = 0xFFFFFF55;
    constexpr u32 MC_WHITE        = 0xFFFFFFFF;

    /**
     * @brief 从ARGB分量创建颜色
     */
    [[nodiscard]] constexpr u32 fromARGB(u8 a, u8 r, u8 g, u8 b) {
        return (static_cast<u32>(a) << 24) |
               (static_cast<u32>(r) << 16) |
               (static_cast<u32>(g) << 8)  |
               (static_cast<u32>(b));
    }

    /**
     * @brief 从RGB分量创建颜色 (Alpha默认255)
     */
    [[nodiscard]] constexpr u32 fromRGB(u8 r, u8 g, u8 b) {
        return fromARGB(255, r, g, b);
    }

    /**
     * @brief 获取颜色的Alpha分量
     */
    [[nodiscard]] constexpr u8 getAlpha(u32 color) {
        return static_cast<u8>((color >> 24) & 0xFF);
    }

    /**
     * @brief 获取颜色的红色分量
     */
    [[nodiscard]] constexpr u8 getRed(u32 color) {
        return static_cast<u8>((color >> 16) & 0xFF);
    }

    /**
     * @brief 获取颜色的绿色分量
     */
    [[nodiscard]] constexpr u8 getGreen(u32 color) {
        return static_cast<u8>((color >> 8) & 0xFF);
    }

    /**
     * @brief 获取颜色的蓝色分量
     */
    [[nodiscard]] constexpr u8 getBlue(u32 color) {
        return static_cast<u8>(color & 0xFF);
    }
} // namespace Colors

} // namespace mr::client
