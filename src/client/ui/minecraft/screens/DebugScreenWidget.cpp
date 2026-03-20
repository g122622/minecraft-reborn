#include "DebugScreenWidget.hpp"

namespace mc::client::ui::minecraft {

DebugScreenWidget::DebugScreenWidget()
    : Screen("debug_screen")
    , m_lineHeight(11) {
}

void DebugScreenWidget::setData(const std::vector<String>& leftLines,
                                 const std::vector<String>& rightLines,
                                 f32 screenWidth, f32 screenHeight) {
    m_leftLines = leftLines;
    m_rightLines = rightLines;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    measureTexts();
}

void DebugScreenWidget::setTextWidthCallback(TextWidthCallback callback) {
    m_textWidthCallback = std::move(callback);
}

void DebugScreenWidget::clearData() {
    m_leftLines.clear();
    m_rightLines.clear();
    m_leftMaxWidth = 0.0f;
    m_rightMaxWidth = 0.0f;
}

void DebugScreenWidget::measureTexts() {
    m_leftMaxWidth = 0.0f;
    m_rightMaxWidth = 0.0f;

    // 测量左侧文本最大宽度
    for (const auto& line : m_leftLines) {
        if (!line.empty() && m_textWidthCallback) {
            f32 width = m_textWidthCallback(line);
            m_leftMaxWidth = std::max(m_leftMaxWidth, width);
        }
    }

    // 测量右侧文本最大宽度
    for (const auto& line : m_rightLines) {
        if (!line.empty() && m_textWidthCallback) {
            f32 width = m_textWidthCallback(line);
            m_rightMaxWidth = std::max(m_rightMaxWidth, width);
        }
    }
}

void DebugScreenWidget::paint(kagero::widget::PaintContext& ctx) {
    if (m_leftLines.empty() && m_rightLines.empty()) return;

    // ==================== 绘制左侧面板 ====================
    if (!m_leftLines.empty()) {
        // 计算背景尺寸
        i32 leftBgWidth = static_cast<i32>(m_leftMaxWidth + 10.0f);
        i32 leftBgHeight = static_cast<i32>(m_leftLines.size() * m_lineHeight + 4);

        // 绘制背景
        ctx.drawFilledRect(kagero::Rect(1, 1, leftBgWidth, leftBgHeight), BG_COLOR);

        // 绘制文本（带阴影）
        i32 y = 2;
        for (const auto& line : m_leftLines) {
            if (!line.empty()) {
                // 先绘制阴影（偏移1像素）
                ctx.drawText(line, 3, y + 1, SHADOW_COLOR);
                // 再绘制前景文本
                ctx.drawText(line, 2, y, TEXT_COLOR);
            }
            y += m_lineHeight;
        }
    }

    // ==================== 绘制右侧面板 ====================
    if (!m_rightLines.empty()) {
        // 计算背景尺寸
        i32 rightBgWidth = static_cast<i32>(m_rightMaxWidth + 10.0f);
        i32 rightBgHeight = static_cast<i32>(m_rightLines.size() * m_lineHeight + 4);

        // 右对齐背景位置
        i32 rightBgX = static_cast<i32>(m_screenWidth - m_rightMaxWidth - 12.0f);

        // 绘制背景
        ctx.drawFilledRect(kagero::Rect(rightBgX, 1, rightBgWidth, rightBgHeight), BG_COLOR);

        // 绘制文本（右对齐，带阴影）
        i32 y = 2;
        for (const auto& line : m_rightLines) {
            if (!line.empty()) {
                // 计算右对齐位置
                f32 textWidth = 0.0f;
                if (m_textWidthCallback) {
                    textWidth = m_textWidthCallback(line);
                }
                i32 x = static_cast<i32>(m_screenWidth - textWidth - 4.0f);

                // 先绘制阴影（偏移1像素）
                ctx.drawText(line, x + 1, y + 1, SHADOW_COLOR);
                // 再绘制前景文本
                ctx.drawText(line, x, y, TEXT_COLOR);
            }
            y += m_lineHeight;
        }
    }
}

} // namespace mc::client::ui::minecraft
