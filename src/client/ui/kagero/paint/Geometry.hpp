#pragma once

#include "../Types.hpp"
#include <array>

namespace mc::client::ui::kagero::paint {

/**
 * @brief 2D仿射矩阵（3x3）
 */
struct Matrix {
    std::array<f32, 9> m = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    [[nodiscard]] static Matrix identity() {
        return Matrix{};
    }
};

/**
 * @brief 圆角矩形
 */
struct RRect {
    Rect rect;
    f32 radiusX = 0.0f;
    f32 radiusY = 0.0f;
};

} // namespace mc::client::ui::kagero::paint
