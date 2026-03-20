#pragma once

#include "../../../paint/Geometry.hpp"

namespace mc::client::ui::kagero::backends::trident::pipelines {

/**
 * @brief UI渲染管线占位实现
 */
class UIPipeline {
public:
    void beginFrame(i32 width, i32 height);
    void endFrame();

private:
    i32 m_width = 0;
    i32 m_height = 0;
};

} // namespace mc::client::ui::kagero::backends::trident::pipelines
