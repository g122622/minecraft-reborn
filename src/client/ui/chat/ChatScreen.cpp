#include "ChatScreen.hpp"
#include "client/ui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"
#include <chrono>

// GLFW key codes
#define GLFW_KEY_ENTER          257
#define GLFW_KEY_BACKSPACE      259
#define GLFW_KEY_DELETE         261
#define GLFW_KEY_UP             265
#define GLFW_KEY_DOWN           264
#define GLFW_KEY_LEFT           263
#define GLFW_KEY_RIGHT          262
#define GLFW_KEY_HOME           268
#define GLFW_KEY_END            269
#define GLFW_KEY_ESCAPE         256
#define GLFW_KEY_TAB            258
#define GLFW_KEY_V              86
#define GLFW_KEY_C              67
#define GLFW_KEY_A              65
#define GLFW_KEY_X              88

#define GLFW_PRESS              1
#define GLFW_RELEASE            0
#define GLFW_MOD_CONTROL        2
#define GLFW_MOD_SHIFT          1

namespace mc::client {

void ChatScreen::initialize(Font* font) {
    m_font = font;
    m_lastBlinkTime = std::chrono::steady_clock::now();
}

void ChatScreen::open(bool command) {
    m_state = command ? ChatState::OpenCommand : ChatState::Open;
    m_cursorVisible = true;
    m_lastBlinkTime = std::chrono::steady_clock::now();

    if (command && m_input.empty()) {
        m_input = "/";
        m_cursorPos = 1;
    }
}

void ChatScreen::close() {
    m_state = ChatState::Closed;
    m_input.clear();
    m_cursorPos = 0;
    m_hasSelection = false;
    m_history.resetInputNavigation();
}

void ChatScreen::toggle() {
    if (isOpen()) {
        close();
    } else {
        open(false);
    }
}

bool ChatScreen::onCharInput(u32 codepoint) {
    if (!isOpen()) return false;

    // 忽略控制字符
    if (codepoint < 32) return false;

    // 检查是否在命令模式且光标在开头
    if (m_state == ChatState::OpenCommand && m_cursorPos == 0) {
        // 在命令模式不允许删除开头的 /
    }

    // 删除选中的文本
    if (m_hasSelection) {
        deleteSelection();
    }

    // 插入字符
    String utf8Char;
    if (codepoint < 0x80) {
        utf8Char = static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        utf8Char += static_cast<char>(0xC0 | (codepoint >> 6));
        utf8Char += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        utf8Char += static_cast<char>(0xE0 | (codepoint >> 12));
        utf8Char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        utf8Char += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        utf8Char += static_cast<char>(0xF0 | (codepoint >> 18));
        utf8Char += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        utf8Char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        utf8Char += static_cast<char>(0x80 | (codepoint & 0x3F));
    }

    insertText(utf8Char);

    // 重置光标闪烁
    m_cursorVisible = true;
    m_lastBlinkTime = std::chrono::steady_clock::now();

    return true;
}

bool ChatScreen::onKeyInput(i32 key, i32 action, i32 mods) {
    if (!isOpen()) {
        // 检查是否应该打开聊天框
        if (action == GLFW_PRESS && key == 'T') {
            open(false);
            return true;
        }
        if (action == GLFW_PRESS && key == '/') {
            open(true);
            return true;
        }
        return false;
    }

    if (action != GLFW_PRESS && action != 2) { // 2 = repeat
        return false;
    }

    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
        case GLFW_KEY_ESCAPE:
            close();
            return true;

        case GLFW_KEY_ENTER:
            sendInput();
            close();
            return true;

        case GLFW_KEY_BACKSPACE:
            if (m_hasSelection) {
                deleteSelection();
            } else {
                deleteBeforeCursor();
            }
            break;

        case GLFW_KEY_DELETE:
            if (m_hasSelection) {
                deleteSelection();
            } else {
                deleteAfterCursor();
            }
            break;

        case GLFW_KEY_LEFT:
            if (ctrl) {
                // 移动到上一个单词
                size_t pos = m_cursorPos;
                while (pos > 0 && m_input[pos - 1] == ' ') pos--;
                while (pos > 0 && m_input[pos - 1] != ' ') pos--;
                setCursorPosition(pos);
            } else {
                moveCursor(-1, shift);
            }
            break;

        case GLFW_KEY_RIGHT:
            if (ctrl) {
                // 移动到下一个单词
                size_t pos = m_cursorPos;
                while (pos < m_input.size() && m_input[pos] == ' ') pos++;
                while (pos < m_input.size() && m_input[pos] != ' ') pos++;
                setCursorPosition(pos);
            } else {
                moveCursor(1, shift);
            }
            break;

        case GLFW_KEY_UP:
            // 导航输入历史
            setInput(m_history.getPreviousInput());
            m_cursorPos = m_input.size();
            break;

        case GLFW_KEY_DOWN:
            setInput(m_history.getNextInput());
            m_cursorPos = m_input.size();
            break;

        case GLFW_KEY_HOME:
            moveCursorToEdge(true, shift);
            break;

        case GLFW_KEY_END:
            moveCursorToEdge(false, shift);
            break;

        case GLFW_KEY_A:
            if (ctrl) {
                // 全选
                m_selectionStart = 0;
                m_selectionEnd = m_input.size();
                m_hasSelection = true;
            }
            break;

        case GLFW_KEY_C:
            if (ctrl && m_hasSelection) {
                // TODO: 复制到剪贴板
            }
            break;

        case GLFW_KEY_V:
            if (ctrl) {
                // TODO: 从剪贴板粘贴
            }
            break;

        case GLFW_KEY_X:
            if (ctrl && m_hasSelection) {
                // TODO: 剪切到剪贴板
                deleteSelection();
            }
            break;

        case GLFW_KEY_TAB:
            // TODO: 命令自动补全
            if (m_state == ChatState::OpenCommand) {
                // 暂时忽略
            }
            break;

        default:
            return false;
    }

    // 重置光标闪烁
    m_cursorVisible = true;
    m_lastBlinkTime = std::chrono::steady_clock::now();

    return true;
}

bool ChatScreen::onMouseClick(f64 /*x*/, f64 /*y*/, i32 /*button*/) {
    // TODO: 实现鼠标点击选择
    return false;
}

void ChatScreen::render(GuiRenderer& gui, f32 screenWidth, f32 screenHeight) {
    // 渲染消息列表
    renderMessages(gui, screenWidth, screenHeight);

    // 如果打开，渲染输入框
    if (isOpen()) {
        renderInputBox(gui, screenWidth, screenHeight);
    }
}

void ChatScreen::renderMessages(GuiRenderer& gui, f32 screenWidth, f32 screenHeight) {
    if (!m_font) return;

    (void)screenWidth;  // 暂时未使用，未来可用于消息换行
    f32 lineHeight = static_cast<f32>(m_font->getFontHeight());
    f32 padding = INPUT_BOX_PADDING;

    auto now = std::chrono::steady_clock::now();
    auto visibleMessages = m_history.getVisibleMessages(isOpen());

    f32 y = screenHeight - INPUT_BOX_HEIGHT - padding * 2;
    if (isOpen()) {
        y -= padding;
    }

    for (const auto& msg : visibleMessages) {
        if (y < lineHeight) break;

        // 计算消息年龄和透明度
        float alpha = 1.0f;
        if (!msg.permanent && !isOpen()) {
            auto age = std::chrono::duration<float>(now - msg.timestamp).count();
            if (age > MESSAGE_FADE_START) {
                float fadeProgress = (age - MESSAGE_FADE_START) / (ChatHistory::MESSAGE_FADE_TIME - MESSAGE_FADE_START);
                alpha = 1.0f - std::min(fadeProgress, 1.0f);
            }
        }

        if (alpha > 0.01f) {
            // 提取颜色分量并应用透明度
            u32 baseColor = msg.color;
            u32 a = static_cast<u32>(((baseColor >> 24) & 0xFF) * alpha);
            u32 r = (baseColor >> 16) & 0xFF;
            u32 g = (baseColor >> 8) & 0xFF;
            u32 b = baseColor & 0xFF;
            u32 color = (a << 24) | (r << 16) | (g << 8) | b;

            // 渲染消息
            gui.drawText(msg.text, padding, y, color, true);
        }

        y -= lineHeight;
    }
}

void ChatScreen::renderInputBox(GuiRenderer& gui, f32 screenWidth, f32 screenHeight) {
    if (!m_font) return;

    f32 chatWidth = screenWidth * CHAT_WIDTH_RATIO;
    f32 boxHeight = INPUT_BOX_HEIGHT;
    f32 padding = INPUT_BOX_PADDING;
    f32 boxY = screenHeight - boxHeight - padding * 2;

    // 绲制输入框背景
    u32 bgColor = 0x80000000; // 半透明黑色
    gui.fillRect(0, boxY, chatWidth, boxHeight, bgColor);

    // 绘制边框
    u32 borderColor = 0xFF555555;
    gui.drawRect(0, boxY, chatWidth, boxHeight, borderColor);

    // 绘制提示文本
    String displayText = m_input;
    if (displayText.empty() && m_state == ChatState::Open) {
        displayText = "Click here to type...";
        gui.drawText(displayText, padding, boxY + padding + 2, 0xFF808080);
    } else {
        // 渲染输入文本
        gui.drawText(displayText, padding, boxY + padding + 2, 0xFFFFFFFF, true);
    }

    // 绘制光标
    updateCursorBlink();
    if (m_cursorVisible) {
        f32 cursorX = padding + getCursorPixelPosition();
        renderCursor(gui, cursorX, boxY + padding + 2);
    }

    // 绘制选区高亮
    if (m_hasSelection) {
        // TODO: 实现选区高亮
    }
}

void ChatScreen::renderCursor(GuiRenderer& gui, f32 x, f32 y) {
    f32 cursorHeight = static_cast<f32>(m_font->getFontHeight()) - 2;
    u32 cursorColor = 0xFFFFFFFF;
    gui.fillRect(x, y, 2, cursorHeight, cursorColor);
}

void ChatScreen::updateCursorBlink() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - m_lastBlinkTime).count();

    if (elapsed >= CURSOR_BLINK_RATE) {
        m_cursorVisible = !m_cursorVisible;
        m_lastBlinkTime = now;
    }
}

void ChatScreen::insertText(const String& text) {
    m_input.insert(m_cursorPos, text);
    m_cursorPos += text.size();
    m_hasSelection = false;
}

void ChatScreen::deleteSelection() {
    if (!m_hasSelection) return;

    size_t start = std::min(m_selectionStart, m_selectionEnd);
    size_t end = std::max(m_selectionStart, m_selectionEnd);

    m_input.erase(start, end - start);
    m_cursorPos = start;
    m_hasSelection = false;
}

void ChatScreen::deleteBeforeCursor() {
    if (m_cursorPos == 0) return;

    // 在命令模式不允许删除开头的 /
    if (m_state == ChatState::OpenCommand && m_cursorPos == 1) return;

    m_input.erase(m_cursorPos - 1, 1);
    m_cursorPos--;
}

void ChatScreen::deleteAfterCursor() {
    if (m_cursorPos >= m_input.size()) return;
    m_input.erase(m_cursorPos, 1);
}

void ChatScreen::moveCursor(i32 offset, bool selecting) {
    size_t newPos = static_cast<size_t>(static_cast<i32>(m_cursorPos) + offset);
    newPos = std::min(newPos, m_input.size());

    if (selecting) {
        if (!m_hasSelection) {
            m_selectionStart = m_cursorPos;
            m_hasSelection = true;
        }
        m_selectionEnd = newPos;
    } else {
        m_hasSelection = false;
    }

    m_cursorPos = newPos;
}

void ChatScreen::moveCursorToEdge(bool start, bool selecting) {
    size_t newPos = start ? 0 : m_input.size();

    if (selecting) {
        if (!m_hasSelection) {
            m_selectionStart = m_cursorPos;
            m_hasSelection = true;
        }
        m_selectionEnd = newPos;
    } else {
        m_hasSelection = false;
    }

    m_cursorPos = newPos;
}

f32 ChatScreen::getCursorPixelPosition() const {
    if (!m_font) return 0.0f;

    std::string textBeforeCursor = m_input.substr(0, m_cursorPos);
    return m_font->getStringWidthUTF8(textBeforeCursor);
}

void ChatScreen::sendInput() {
    if (m_input.empty()) return;

    // 添加到历史
    m_history.addToInputHistory(m_input);

    // 调用回调
    if (m_commandCallback) {
        m_commandCallback(m_input);
    }

    // 清除输入
    clearInput();
}

void ChatScreen::setInput(const String& text) {
    m_input = text;
    m_cursorPos = m_input.size();
    m_hasSelection = false;
}

void ChatScreen::clearInput() {
    m_input.clear();
    m_cursorPos = 0;
    m_hasSelection = false;
}

void ChatScreen::setCursorPosition(size_t pos) {
    m_cursorPos = std::min(pos, m_input.size());
    m_hasSelection = false;
}

void ChatScreen::addMessage(const String& message, u32 color) {
    m_history.addMessage(message, color);
}

void ChatScreen::addSystemMessage(const String& message) {
    m_history.addSystemMessage(message);
}

} // namespace mc::client
