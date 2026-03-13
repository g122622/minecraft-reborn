#include "CrosshairRenderer.hpp"
#include "../GuiRenderer.hpp"

namespace mc::client {

CrosshairRenderer::CrosshairRenderer() = default;

Result<void> CrosshairRenderer::initialize(GuiRenderer* guiRenderer)
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    if (guiRenderer == nullptr) {
        return Error(ErrorCode::NullPointer, "GuiRenderer is null");
    }

    m_guiRenderer = guiRenderer;
    m_initialized = true;
    return Result<void>::ok();
}

void CrosshairRenderer::render()
{
    if (!m_visible || !m_initialized || m_guiRenderer == nullptr) {
        return;
    }

    // 获取屏幕尺寸
    f32 screenW = m_guiRenderer->screenWidth();
    f32 screenH = m_guiRenderer->screenHeight();

    // 计算屏幕中心
    f32 centerX = screenW / 2.0f;
    f32 centerY = screenH / 2.0f;

    // 计算准星尺寸
    f32 halfSize = m_size / 2.0f;
    f32 halfThick = m_thickness / 2.0f;

    // 绘制水平线
    // 从中心向左右各延伸 halfSize
    m_guiRenderer->fillRect(
        centerX - halfSize,      // x: 左边界
        centerY - halfThick,     // y: 上边界
        m_size,                  // width: 线长
        m_thickness,             // height: 线宽
        m_color
    );

    // 绘制垂直线
    // 从中心向上下各延伸 halfSize
    m_guiRenderer->fillRect(
        centerX - halfThick,     // x: 左边界
        centerY - halfSize,      // y: 上边界
        m_thickness,             // width: 线宽
        m_size,                  // height: 线长
        m_color
    );
}

} // namespace mc::client
