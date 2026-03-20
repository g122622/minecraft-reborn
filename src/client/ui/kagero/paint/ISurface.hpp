#pragma once

#include "ICanvas.hpp"

namespace mc::client::ui::kagero::paint {

/**
 * @brief 绘制表面接口
 */
class ISurface {
public:
    virtual ~ISurface() = default;

    [[nodiscard]] virtual ICanvas& canvas() = 0;
    [[nodiscard]] virtual const ICanvas& canvas() const = 0;

    virtual void flush() = 0;
    virtual void resize(i32 width, i32 height) = 0;

    [[nodiscard]] virtual i32 width() const = 0;
    [[nodiscard]] virtual i32 height() const = 0;
};

} // namespace mc::client::ui::kagero::paint
