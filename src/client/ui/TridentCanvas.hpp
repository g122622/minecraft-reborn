#pragma once

#include "kagero/paint/ICanvas.hpp"
#include "kagero/Types.hpp"
#include "Glyph.hpp"
#include <vector>

namespace mc::client {
class Font;
class FontRenderer;
}

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client::ui {

/**
 * @brief ICanvas 的 Trident 实现
 *
 * 简单适配器，将 ICanvas 调用转换为 GuiRenderer 调用。
 * 不持有任何 Vulkan 资源，只累积顶点数据并提交给 GuiRenderer。
 *
 * 注意：此类不是线程安全的，应在主线程使用。
 */
class TridentCanvas final : public kagero::paint::ICanvas {
public:
    /**
     * @brief 构造函数
     * @param renderer GuiRenderer 引用，用于实际渲染
     * @param font Font 引用，用于文本渲染
     */
    TridentCanvas(renderer::trident::gui::GuiRenderer& renderer, Font& font);
    ~TridentCanvas() override = default;

    // 禁止拷贝
    TridentCanvas(const TridentCanvas&) = delete;
    TridentCanvas& operator=(const TridentCanvas&) = delete;

    // ==================== 帧管理 ====================

    /**
     * @brief 开始新帧
     *
     * 清空内部状态，准备接收新的绘制命令。
     * 应在 GuiRenderer::beginFrame() 之后调用。
     */
    void beginFrame();

    /**
     * @brief 结束帧
     *
     * 完成当前帧的绘制。所有累积的顶点数据已通过 GuiRenderer 提交。
     * 应在 GuiRenderer::render() 之前调用。
     */
    void endFrame();

    // ==================== ICanvas 实现 ====================

    void drawRect(const kagero::Rect& rect, const kagero::paint::IPaint& paint) override;
    void drawRRect(const kagero::paint::RRect& roundRect, const kagero::paint::IPaint& paint) override;
    void drawCircle(f32 cx, f32 cy, f32 radius, const kagero::paint::IPaint& paint) override;
    void drawOval(const kagero::Rect& bounds, const kagero::paint::IPaint& paint) override;
    void drawPath(const kagero::paint::IPath& path, const kagero::paint::IPaint& paint) override;
    void drawLine(f32 x0, f32 y0, f32 x1, f32 y1, const kagero::paint::IPaint& paint) override;

    void drawImage(const kagero::paint::IImage& image, f32 x, f32 y) override;
    void drawImageRect(const kagero::paint::IImage& image, const kagero::Rect& src, const kagero::Rect& dst) override;
    void drawImageNine(const kagero::paint::IImage& image, const kagero::Rect& center, const kagero::Rect& dst, const kagero::paint::IPaint* paint) override;

    void drawText(const String& text, f32 x, f32 y, const kagero::paint::IPaint& paint) override;
    void drawTextBlob(const kagero::paint::ITextBlob& blob, f32 x, f32 y, const kagero::paint::IPaint& paint) override;

    void clipRect(const kagero::Rect& rect) override;
    void clipRRect(const kagero::paint::RRect& roundRect) override;
    void clipPath(const kagero::paint::IPath& path) override;
    void clipOutRect(const kagero::Rect& rect) override;
    [[nodiscard]] bool clipIsEmpty() const override;
    [[nodiscard]] kagero::Rect getClipBounds() const override;

    void translate(f32 dx, f32 dy) override;
    void scale(f32 sx, f32 sy) override;
    void rotate(f32 degrees) override;
    void concat(const kagero::paint::Matrix& matrix) override;
    void setMatrix(const kagero::paint::Matrix& matrix) override;
    [[nodiscard]] kagero::paint::Matrix getTotalMatrix() const override;

    i32 save() override;
    void restore() override;
    void restoreToCount(i32 saveCount) override;

    i32 saveLayer(const kagero::Rect* bounds, const kagero::paint::IPaint* paint) override;
    i32 saveLayerAlpha(const kagero::Rect* bounds, u8 alpha) override;

    [[nodiscard]] i32 width() const override;
    [[nodiscard]] i32 height() const override;

    // ==================== 扩展接口 ====================

    /**
     * @brief 调整画布尺寸
     */
    void resize(i32 width, i32 height);

    /**
     * @brief 获取关联的 Font
     */
    [[nodiscard]] Font& font() { return m_font; }
    [[nodiscard]] const Font& font() const { return m_font; }

    /**
     * @brief 获取关联的 GuiRenderer
     */
    [[nodiscard]] renderer::trident::gui::GuiRenderer& renderer() { return m_renderer; }
    [[nodiscard]] const renderer::trident::gui::GuiRenderer& renderer() const { return m_renderer; }

private:
    /**
     * @brief 应用当前变换到点
     */
    void transformPoint(f32& x, f32& y) const;

    /**
     * @brief 从 IPaint 提取颜色
     */
    [[nodiscard]] u32 extractColor(const kagero::paint::IPaint& paint) const;

    renderer::trident::gui::GuiRenderer& m_renderer;
    Font& m_font;

    i32 m_width = 0;
    i32 m_height = 0;
    kagero::Rect m_clipBounds;
    kagero::paint::Matrix m_matrix;
    std::vector<kagero::Rect> m_clipStack;
    std::vector<kagero::paint::Matrix> m_matrixStack;
    std::vector<f32> m_alphaStack;  ///< 用于 saveLayerAlpha
};

} // namespace mc::client::ui
