#include "ChatWidget.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/kagero/Types.hpp"
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft::widgets {

ChatWidget::ChatWidget()
    : ContainerWidget()
    , m_open(false)
    , m_commandMode(false)
{
    setId("chat");
    setVisible(true);
    setActive(false);  // 默认不激活，打开时才激活
}

void ChatWidget::open(bool command) {
    m_open = true;
    m_commandMode = command;
    if (command && m_input.empty()) {
        m_input = "/";
        m_cursorPos = 1;
    }
    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
    setActive(true);  // 打开时激活，接收事件
}

void ChatWidget::close() {
    m_open = false;
    m_commandMode = false;
    clearInput();
    setActive(false);  // 关闭时取消激活
}

void ChatWidget::toggle() {
    if (m_open) {
        close();
    } else {
        open(false);
    }
}

void ChatWidget::paint(kagero::widget::PaintContext& ctx) {
    // 渲染消息列表
    renderMessages(ctx);

    // 如果打开，渲染输入框
    if (m_open) {
        renderInputBox(ctx);
    }
}

void ChatWidget::tick(f32 dt) {
    if (m_open) {
        updateCursorBlink(dt);
    }
}

bool ChatWidget::onKey(i32 key, i32 scanCode, i32 action, i32 mods) {
    if (!m_open || action != GLFW_PRESS) {
        return false;
    }

    switch (key) {
        case GLFW_KEY_ESCAPE:
            close();
            return true;

        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            sendInput();
            return true;

        case GLFW_KEY_BACKSPACE:
            if (m_hasSelection) {
                deleteSelection();
            } else {
                deleteBeforeCursor();
            }
            return true;

        case GLFW_KEY_DELETE:
            if (m_hasSelection) {
                deleteSelection();
            } else {
                deleteAfterCursor();
            }
            return true;

        case GLFW_KEY_LEFT:
            if (mods & GLFW_MOD_CONTROL) {
                moveCursorToEdge(true, mods & GLFW_MOD_SHIFT);
            } else {
                moveCursor(-1, mods & GLFW_MOD_SHIFT);
            }
            return true;

        case GLFW_KEY_RIGHT:
            if (mods & GLFW_MOD_CONTROL) {
                moveCursorToEdge(false, mods & GLFW_MOD_SHIFT);
            } else {
                moveCursor(1, mods & GLFW_MOD_SHIFT);
            }
            return true;

        case GLFW_KEY_HOME:
            moveCursorToEdge(true, mods & GLFW_MOD_SHIFT);
            return true;

        case GLFW_KEY_END:
            moveCursorToEdge(false, mods & GLFW_MOD_SHIFT);
            return true;

        case GLFW_KEY_UP:
            // 浏览命令历史
            if (!m_commandHistory.empty()) {
                if (m_historyIndex > 0) {
                    --m_historyIndex;
                    setInput(m_commandHistory[m_historyIndex]);
                }
            }
            return true;

        case GLFW_KEY_DOWN:
            // 浏览命令历史
            if (!m_commandHistory.empty()) {
                if (m_historyIndex < m_commandHistory.size() - 1) {
                    ++m_historyIndex;
                    setInput(m_commandHistory[m_historyIndex]);
                } else if (m_historyIndex == m_commandHistory.size() - 1) {
                    ++m_historyIndex;
                    clearInput();
                }
            }
            return true;

        case GLFW_KEY_A:
            if (mods & GLFW_MOD_CONTROL) {
                // 全选
                m_selectionStart = 0;
                m_selectionEnd = m_input.size();
                m_hasSelection = true;
                return true;
            }
            break;

        case GLFW_KEY_C:
            if (mods & GLFW_MOD_CONTROL && m_hasSelection) {
                // 复制（TODO: 实现剪贴板）
                return true;
            }
            break;

        case GLFW_KEY_V:
            if (mods & GLFW_MOD_CONTROL) {
                // 粘贴（TODO: 实现剪贴板）
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

bool ChatWidget::onChar(u32 codePoint) {
    if (!m_open) {
        return false;
    }

    // 忽略控制字符
    if (codePoint < 32) {
        return false;
    }

    // 删除选中内容
    if (m_hasSelection) {
        deleteSelection();
    }

    // 插入字符
    char utf8[5] = {0};
    if (codePoint < 0x80) {
        utf8[0] = static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        utf8[0] = static_cast<char>(0xC0 | (codePoint >> 6));
        utf8[1] = static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        utf8[0] = static_cast<char>(0xE0 | (codePoint >> 12));
        utf8[1] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        utf8[0] = static_cast<char>(0xF0 | (codePoint >> 18));
        utf8[1] = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        utf8[3] = static_cast<char>(0x80 | (codePoint & 0x3F));
    }

    insertText(utf8);
    return true;
}

bool ChatWidget::onClick(i32 mouseX, i32 mouseY, i32 button) {
    if (!m_open) {
        return false;
    }
    // TODO: 处理点击选择
    (void)mouseX;
    (void)mouseY;
    (void)button;
    return true;
}

void ChatWidget::addMessage(const String& message, u32 color) {
    m_history.addMessage(message, color);
}

void ChatWidget::addSystemMessage(const String& message) {
    m_history.addMessage(message, 0xFFFFFF00);  // 黄色系统消息
}

void ChatWidget::setInput(const String& text) {
    m_input = text;
    m_cursorPos = m_input.size();
    m_hasSelection = false;
}

void ChatWidget::clearInput() {
    m_input.clear();
    m_cursorPos = 0;
    m_selectionStart = 0;
    m_selectionEnd = 0;
    m_hasSelection = false;
}

void ChatWidget::insertText(const String& text) {
    m_input.insert(m_cursorPos, text);
    m_cursorPos += text.size();
    m_hasSelection = false;
    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
}

void ChatWidget::deleteSelection() {
    if (!m_hasSelection) {
        return;
    }

    size_t start = std::min(m_selectionStart, m_selectionEnd);
    size_t end = std::max(m_selectionStart, m_selectionEnd);

    m_input.erase(start, end - start);
    m_cursorPos = start;
    m_hasSelection = false;
}

void ChatWidget::deleteBeforeCursor() {
    if (m_cursorPos > 0) {
        m_input.erase(m_cursorPos - 1, 1);
        --m_cursorPos;
    }
}

void ChatWidget::deleteAfterCursor() {
    if (m_cursorPos < m_input.size()) {
        m_input.erase(m_cursorPos, 1);
    }
}

void ChatWidget::moveCursor(i32 offset, bool selecting) {
    if (selecting && !m_hasSelection) {
        m_selectionStart = m_cursorPos;
        m_hasSelection = true;
    }

    if (offset < 0) {
        m_cursorPos = m_cursorPos > static_cast<size_t>(-offset)
            ? m_cursorPos + offset
            : 0;
    } else {
        m_cursorPos = m_cursorPos + offset < m_input.size()
            ? m_cursorPos + offset
            : m_input.size();
    }

    if (selecting) {
        m_selectionEnd = m_cursorPos;
    } else {
        m_hasSelection = false;
    }

    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
}

void ChatWidget::moveCursorToEdge(bool start, bool selecting) {
    if (selecting && !m_hasSelection) {
        m_selectionStart = m_cursorPos;
        m_hasSelection = true;
    }

    m_cursorPos = start ? 0 : m_input.size();

    if (selecting) {
        m_selectionEnd = m_cursorPos;
    } else {
        m_hasSelection = false;
    }

    m_cursorVisible = true;
    m_cursorBlinkTimer = 0.0f;
}

f32 ChatWidget::getCursorPixelPosition() const {
    // TODO: 使用字体测量
    if (m_gui == nullptr || m_font == nullptr) {
        return static_cast<f32>(m_cursorPos * 6);  // 估计宽度
    }
    return m_gui->getTextWidth(m_input.substr(0, m_cursorPos));
}

void ChatWidget::sendInput() {
    if (m_input.empty()) {
        close();
        return;
    }

    // 添加到命令历史
    m_commandHistory.push_back(m_input);
    m_historyIndex = m_commandHistory.size();

    // 调用回调
    if (m_commandCallback) {
        m_commandCallback(m_input);
    }

    close();
}

void ChatWidget::updateCursorBlink(f32 dt) {
    m_cursorBlinkTimer += dt;
    if (m_cursorBlinkTimer >= CURSOR_BLINK_RATE) {
        m_cursorBlinkTimer = 0.0f;
        m_cursorVisible = !m_cursorVisible;
    }
}

void ChatWidget::renderMessages(kagero::widget::PaintContext& ctx) {
    if (m_gui == nullptr) {
        return;
    }

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());
    const f32 chatWidth = screenWidth * CHAT_WIDTH_RATIO;
    constexpr f32 lineHeight = 9.0f;  // 字体行高
    constexpr f32 padding = 4.0f;

    // 从底部向上渲染消息
    const auto& messages = m_history.allMessages();
    f32 y = screenHeight - INPUT_BOX_HEIGHT - 20.0f;  // 留出输入框空间

    auto now = std::chrono::steady_clock::now();

    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (y < 0.0f) {
            break;  // 超出屏幕顶部
        }

        // 计算消息透明度（旧消息淡出）
        f32 alpha = 1.0f;
        if (!m_open && !it->permanent) {
            // 关闭状态时，旧消息淡出
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->timestamp).count();
            f32 age = static_cast<f32>(elapsed);
            if (age > MESSAGE_FADE_START) {
                alpha = std::max(0.0f, 1.0f - (age - MESSAGE_FADE_START) / 3.0f);
            }
        }

        if (alpha > 0.01f) {
            u32 color = it->color;
            // 设置alpha
            u8 a = static_cast<u8>(((color >> 24) & 0xFF) * alpha);
            color = (color & 0x00FFFFFF) | (static_cast<u32>(a) << 24);

            // 渲染消息背景
            f32 textWidth = m_gui->getTextWidth(it->text);
            ctx.drawFilledRect(
                kagero::Rect(
                    static_cast<i32>(padding),
                    static_cast<i32>(y - lineHeight),
                    static_cast<i32>(textWidth + padding * 2),
                    static_cast<i32>(lineHeight + 2)
                ),
                static_cast<u32>(0x80000000 * alpha)
            );

            // 渲染消息文本
            m_gui->drawText(it->text, padding + 2.0f, y - lineHeight, color, true);
        }

        y -= lineHeight + 2.0f;
    }
}

void ChatWidget::renderInputBox(kagero::widget::PaintContext& ctx) {
    if (m_gui == nullptr) {
        return;
    }

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());
    const f32 chatWidth = screenWidth * CHAT_WIDTH_RATIO;
    const f32 inputY = screenHeight - INPUT_BOX_HEIGHT - 10.0f;

    // 渲染输入框背景
    ctx.drawFilledRect(
        kagero::Rect(
            static_cast<i32>(INPUT_BOX_PADDING),
            static_cast<i32>(inputY),
            static_cast<i32>(chatWidth),
            static_cast<i32>(INPUT_BOX_HEIGHT)
        ),
        0x80000000
    );

    // 渲染输入文本
    f32 textX = INPUT_BOX_PADDING + 4.0f;
    f32 textY = inputY + (INPUT_BOX_HEIGHT - 9.0f) / 2.0f;
    m_gui->drawText(m_input, textX, textY, 0xFFFFFFFF, false);

    // 渲染光标
    if (m_cursorVisible) {
        constexpr f32 lineHeight = 9.0f;
        f32 cursorX = textX + getCursorPixelPosition();
        ctx.drawFilledRect(
            kagero::Rect(
                static_cast<i32>(cursorX),
                static_cast<i32>(textY - 1.0f),
                1,
                static_cast<i32>(lineHeight + 2.0f)
            ),
            0xFFFFFFFF
        );
    }
}

} // namespace mc::client::ui::minecraft::widgets
