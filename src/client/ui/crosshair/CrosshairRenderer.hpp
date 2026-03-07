#pragma once

#include "../../core/Result.hpp"

namespace mr::client {

// 前向声明
class GuiRenderer;

/**
 * @brief 准星渲染器
 *
 * 在屏幕中心渲染十字准星，用于第一人称视角瞄准。
 * 使用GuiRenderer::fillRect()绘制两条交叉线。
 */
class CrosshairRenderer {
public:
    CrosshairRenderer();
    ~CrosshairRenderer() = default;

    // 禁止拷贝
    CrosshairRenderer(const CrosshairRenderer&) = delete;
    CrosshairRenderer& operator=(const CrosshairRenderer&) = delete;

    /**
     * @brief 初始化准星渲染器
     * @param guiRenderer GUI渲染器（必须非空）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(GuiRenderer* guiRenderer);

    /**
     * @brief 渲染准星
     *
     * 在GuiRenderer::beginFrame()之后、render()之前调用。
     * 准星将被添加到GUI渲染队列中。
     */
    void render();

    // ==================== 配置 ====================

    /**
     * @brief 设置准星可见性
     */
    void setVisible(bool visible) { m_visible = visible; }

    /**
     * @brief 设置准星颜色（ARGB格式）
     * @param color ARGB颜色，如0xFFFFFFFF为白色，0xFF00FF00为绿色
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 设置准星大小（十字线长度）
     * @param size 每条线的长度（像素），默认10
     */
    void setSize(f32 size) { m_size = size; }

    /**
     * @brief 设置准星线宽
     * @param thickness 线条宽度（像素），默认1
     */
    void setThickness(f32 thickness) { m_thickness = thickness; }

    /**
     * @brief 检查是否可见
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    GuiRenderer* m_guiRenderer = nullptr;
    bool m_visible = true;
    bool m_initialized = false;

    // 准星样式参数
    u32 m_color = 0xFFFFFFFF;     ///< 白色，完全不透明
    f32 m_size = 10.0f;           ///< 十字线长度
    f32 m_thickness = 1.0f;       ///< 线宽
};

} // namespace mr::client
