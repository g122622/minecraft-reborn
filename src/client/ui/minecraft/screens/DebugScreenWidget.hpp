#pragma once

#include "Screen.hpp"
#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/Types.hpp"
#include <vector>
#include <functional>

namespace mc::client::ui::minecraft {

/**
 * @brief 调试屏幕Widget
 *
 * 使用 kagero UI 引擎渲染 Minecraft F3 调试屏幕。
 * 显示左侧和右侧两个面板，包含游戏状态、系统信息等调试数据。
 *
 * 参考 MC 1.16.5 DebugOverlayGui
 */
class DebugScreenWidget : public Screen {
public:
    /**
     * @brief 文本宽度测量回调类型
     * @param text 要测量宽度的文本
     * @return 文本宽度（像素）
     */
    using TextWidthCallback = std::function<f32(const String& text)>;

    DebugScreenWidget();
    ~DebugScreenWidget() override = default;

    /**
     * @brief 设置调试数据
     * @param leftLines 左侧面板文本行
     * @param rightLines 右侧面板文本行
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     */
    void setData(const std::vector<String>& leftLines,
                 const std::vector<String>& rightLines,
                 f32 screenWidth, f32 screenHeight);

    /**
     * @brief 设置文本宽度测量回调
     * @param callback 测量文本宽度的回调函数
     */
    void setTextWidthCallback(TextWidthCallback callback);

    /**
     * @brief 清空数据
     */
    void clearData();

    /**
     * @brief 绘制调试屏幕
     * @param ctx 绘图上下文
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 设置字体高度（行高）
     * @param lineHeight 行高（像素）
     */
    void setLineHeight(i32 lineHeight) { m_lineHeight = lineHeight; }

private:
    /**
     * @brief 计算文本最大宽度
     */
    void measureTexts();

    // 左侧文本行
    std::vector<String> m_leftLines;
    // 右侧文本行
    std::vector<String> m_rightLines;
    // 最大宽度（用于背景）
    f32 m_leftMaxWidth = 0.0f;
    f32 m_rightMaxWidth = 0.0f;
    // 屏幕尺寸
    f32 m_screenWidth = 0.0f;
    f32 m_screenHeight = 0.0f;
    // 字体高度（行高）
    i32 m_lineHeight = 11;  // 默认字体高度+2
    // 文本宽度测量回调
    TextWidthCallback m_textWidthCallback;

    // 颜色常量（与原版 DebugScreen 保持一致）
    static constexpr u32 BG_COLOR = 0xA0303030;       // 半透明深灰背景
    static constexpr u32 TEXT_COLOR = 0xFFFFFFFF;     // 白色文本
    static constexpr u32 SHADOW_COLOR = 0xFF3F3F3F;   // 深灰阴影
};

} // namespace mc::client::ui::minecraft
