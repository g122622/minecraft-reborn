#pragma once

#include "../paint/ICanvas.hpp"
#include "../paint/IImage.hpp"
#include "../paint/IPaint.hpp"
#include "../paint/Color.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 绘图上下文
 *
 * 提供便捷的绘图方法，封装 ICanvas 操作。
 * 内部缓存 paint 对象以避免每帧创建临时对象。
 */
class PaintContext {
public:
    explicit PaintContext(paint::ICanvas& canvas);

    [[nodiscard]] paint::ICanvas& canvas();
    [[nodiscard]] const paint::ICanvas& canvas() const;

    // ==================== 绘图方法 ====================

    /**
     * @brief 绘制居中文本
     * @param text 文本内容
     * @param bounds 边界矩形
     * @param color 颜色（ARGB）
     */
    void drawTextCentered(const String& text, const Rect& bounds, u32 color);

    /**
     * @brief 绘制边框
     * @param bounds 边界矩形
     * @param width 边框宽度
     * @param color 颜色（ARGB）
     */
    void drawBorder(const Rect& bounds, f32 width, u32 color);

    /**
     * @brief 绘制填充矩形
     * @param bounds 边界矩形
     * @param color 颜色（ARGB）
     */
    void drawFilledRect(const Rect& bounds, u32 color);

    /**
     * @brief 绘制九宫格图像
     * @param image 图像
     * @param center 中心区域（可拉伸部分）
     * @param dst 目标区域
     * @param tint 色调（ARGB）
     */
    void drawNinePatch(const paint::IImage& image, const Rect& center, const Rect& dst, u32 tint = 0xFFFFFFFF);

    // ==================== 变换方法 ====================

    /**
     * @brief 保存当前状态
     * @return 保存点计数
     */
    i32 save();

    /**
     * @brief 恢复到上一个保存的状态
     */
    void restore();

    /**
     * @brief 平移
     */
    void translate(f32 dx, f32 dy);

    // ==================== Paint 访问器 ====================

    /**
     * @brief 获取填充画笔（用于绘制填充形状）
     * @note 返回的画笔已被缓存，可直接修改
     */
    [[nodiscard]] paint::IPaint& fillPaint() { return *m_fillPaint; }
    [[nodiscard]] const paint::IPaint& fillPaint() const { return *m_fillPaint; }

    /**
     * @brief 获取描边画笔（用于绘制边框）
     * @note 返回的画笔已被缓存，可直接修改
     */
    [[nodiscard]] paint::IPaint& strokePaint() { return *m_strokePaint; }
    [[nodiscard]] const paint::IPaint& strokePaint() const { return *m_strokePaint; }

private:
    paint::ICanvas& m_canvas;

    // 缓存的 paint 对象，避免每帧创建临时对象
    std::unique_ptr<paint::IPaint> m_fillPaint;
    std::unique_ptr<paint::IPaint> m_strokePaint;
};

} // namespace mc::client::ui::kagero::widget
