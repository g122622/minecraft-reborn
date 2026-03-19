#pragma once

#include "Widget.hpp"
#include <functional>
#include <string>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 文本输入框组件
 *
 * 支持文本输入、选择、复制粘贴等操作的输入框组件。
 *
 * 参考MC 1.16.5 TextFieldWidget.java实现
 *
 * 使用示例：
 * @code
 * auto textField = std::make_unique<TextFieldWidget>(
 *     "txt_name", 10, 10, 200, 20
 * );
 * textField->setPlaceholder("Enter your name...");
 * textField->setMaxLength(32);
 * textField->setTextChangedCallback([](const String& text) {
 *     // 处理文本变化
 * });
 * @endcode
 */
class TextFieldWidget : public Widget {
public:
    /**
     * @brief 文本变化回调类型
     */
    using TextChangedCallback = std::function<void(const String&)>;

    /**
     * @brief 文本验证器类型
     */
    using TextValidator = std::function<bool(const String&)>;

    /**
     * @brief 默认构造函数
     */
    TextFieldWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    TextFieldWidget(String id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void tick(f32 dt) override {
        (void)dt;
        // 更新光标闪烁
        ++m_cursorBlinkCounter;
    }

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)ctx;
        (void)mouseX;
        (void)mouseY;
        (void)partialTick;

        if (!isVisible()) return;

        // 更新悬停状态
        setHovered(isMouseOver(mouseX, mouseY));

        // TODO: 实际渲染逻辑
        // renderTextField(ctx, mouseX, mouseY, partialTick);
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false; // 只响应左键

        // 设置焦点
        setFocused(true);

        // 计算点击位置的光标位置
        // TODO: 根据字体和点击位置计算光标位置
        // setCursorPositionFromMouse(mouseX);

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override {
        (void)scanCode;
        (void)mods;

        if (!canWrite()) return false;

        if (action != 1 && action != 2) return false; // 只处理按下和重复

        // 处理快捷键
        // Ctrl+A: 全选
        // Ctrl+C: 复制
        // Ctrl+V: 粘贴
        // Ctrl+X: 剪切
        // Backspace: 删除前一个字符
        // Delete: 删除后一个字符
        // Left/Right: 移动光标
        // Home/End: 移动到开头/结尾

        switch (key) {
            case 259: // GLFW_KEY_BACKSPACE
                if (m_active) {
                    deleteFromCursor(-1);
                }
                return true;

            case 261: // GLFW_KEY_DELETE
                if (m_active) {
                    deleteFromCursor(1);
                }
                return true;

            case 262: // GLFW_KEY_RIGHT
                moveCursorBy(1);
                return true;

            case 263: // GLFW_KEY_LEFT
                moveCursorBy(-1);
                return true;

            case 268: // GLFW_KEY_HOME
                setCursorPosition(0);
                return true;

            case 269: // GLFW_KEY_END
                setCursorPosition(static_cast<i32>(m_text.size()));
                return true;

            default:
                return false;
        }
    }

    bool onChar(u32 codePoint) override {
        if (!canWrite()) return false;

        // 检查是否是允许的字符
        if (!isAllowedCharacter(codePoint)) return false;

        // 插入字符
        writeText(codePointToString(codePoint));
        return true;
    }

    // ==================== 文本操作 ====================

    /**
     * @brief 设置文本
     */
    void setText(const String& text) {
        if (m_validator && !m_validator(text)) return;

        String newText = text;
        if (static_cast<i32>(newText.size()) > m_maxLength) {
            newText = newText.substr(0, m_maxLength);
        }

        if (m_text != newText) {
            m_text = newText;
            setCursorPositionEnd();
            setSelectionPosition(m_cursorPosition);
            onTextChanged();
        }
    }

    /**
     * @brief 获取文本
     */
    [[nodiscard]] const String& text() const { return m_text; }

    /**
     * @brief 写入文本（在光标位置插入或替换选中内容）
     */
    void writeText(const String& text) {
        if (text.empty() && !hasSelection()) return;
        if (!m_active) return;

        // 计算选区范围
        i32 selStart = std::min(m_cursorPosition, m_selectionEnd);
        i32 selEnd = std::max(m_cursorPosition, m_selectionEnd);

        // 计算可写入的空间
        i32 availableSpace = m_maxLength - static_cast<i32>(m_text.size()) - (selStart - selEnd);
        String toWrite = filterAllowedCharacters(text);
        if (static_cast<i32>(toWrite.size()) > availableSpace) {
            toWrite = toWrite.substr(0, availableSpace);
        }

        // 替换文本
        String newText = m_text.substr(0, selStart) + toWrite + m_text.substr(selEnd);
        if (m_validator && !m_validator(newText)) return;

        m_text = newText;
        setCursorPosition(selStart + static_cast<i32>(toWrite.size()));
        setSelectionPosition(m_cursorPosition);
        onTextChanged();
    }

    /**
     * @brief 获取选中的文本
     */
    [[nodiscard]] String getSelectedText() const {
        i32 start = std::min(m_cursorPosition, m_selectionEnd);
        i32 end = std::max(m_cursorPosition, m_selectionEnd);
        return m_text.substr(start, end - start);
    }

    /**
     * @brief 删除选中的文本
     */
    void deleteSelectedText() {
        if (!hasSelection()) return;

        i32 start = std::min(m_cursorPosition, m_selectionEnd);
        i32 end = std::max(m_cursorPosition, m_selectionEnd);

        String newText = m_text.substr(0, start) + m_text.substr(end);
        if (m_validator && !m_validator(newText)) return;

        m_text = newText;
        setCursorPosition(start);
        setSelectionPosition(m_cursorPosition);
        onTextChanged();
    }

    // ==================== 光标操作 ====================

    /**
     * @brief 设置光标位置
     */
    void setCursorPosition(i32 position) {
        m_cursorPosition = clampPosition(position);
        if (!m_shiftHeld) {
            m_selectionEnd = m_cursorPosition;
        }
        updateScrollOffset();
    }

    /**
     * @brief 设置光标到开头
     */
    void setCursorPositionStart() {
        setCursorPosition(0);
    }

    /**
     * @brief 设置光标到结尾
     */
    void setCursorPositionEnd() {
        setCursorPosition(static_cast<i32>(m_text.size()));
    }

    /**
     * @brief 移动光标
     */
    void moveCursorBy(i32 delta) {
        setCursorPosition(m_cursorPosition + delta);
    }

    /**
     * @brief 获取光标位置
     */
    [[nodiscard]] i32 cursorPosition() const { return m_cursorPosition; }

    /**
     * @brief 设置选区结束位置
     */
    void setSelectionPosition(i32 position) {
        m_selectionEnd = clampPosition(position);
        updateScrollOffset();
    }

    /**
     * @brief 选择全部文本
     */
    void selectAll() {
        setCursorPositionEnd();
        m_selectionEnd = 0;
    }

    /**
     * @brief 清除选择
     */
    void clearSelection() {
        m_selectionEnd = m_cursorPosition;
    }

    /**
     * @brief 是否有选择
     */
    [[nodiscard]] bool hasSelection() const {
        return m_cursorPosition != m_selectionEnd;
    }

    // ==================== 属性设置 ====================

    /**
     * @brief 设置最大长度
     */
    void setMaxLength(i32 maxLength) {
        m_maxLength = maxLength;
        if (static_cast<i32>(m_text.size()) > maxLength) {
            m_text = m_text.substr(0, maxLength);
            onTextChanged();
        }
    }

    /**
     * @brief 获取最大长度
     */
    [[nodiscard]] i32 maxLength() const { return m_maxLength; }

    /**
     * @brief 设置占位符文本
     */
    void setPlaceholder(const String& placeholder) {
        m_placeholder = placeholder;
    }

    /**
     * @brief 获取占位符文本
     */
    [[nodiscard]] const String& placeholder() const { return m_placeholder; }

    /**
     * @brief 设置文本变化回调
     */
    void setTextChangedCallback(TextChangedCallback callback) {
        m_onTextChanged = std::move(callback);
    }

    /**
     * @brief 设置文本验证器
     */
    void setValidator(TextValidator validator) {
        m_validator = std::move(validator);
    }

    /**
     * @brief 设置启用状态
     */
    void setEnabled(bool enabled) {
        m_enabled = enabled;
    }

    /**
     * @brief 是否启用
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * @brief 设置是否可以失去焦点
     */
    void setCanLoseFocus(bool canLoseFocus) {
        m_canLoseFocus = canLoseFocus;
    }

    /**
     * @brief 是否可以失去焦点
     */
    [[nodiscard]] bool canLoseFocus() const { return m_canLoseFocus; }

    /**
     * @brief 设置是否绘制背景
     */
    void setDrawBackground(bool draw) {
        m_drawBackground = draw;
    }

    /**
     * @brief 是否绘制背景
     */
    [[nodiscard]] bool drawBackground() const { return m_drawBackground; }

    /**
     * @brief 设置文本颜色
     */
    void setTextColor(u32 color) {
        m_textColor = color;
    }

    /**
     * @brief 设置禁用文本颜色
     */
    void setDisabledTextColor(u32 color) {
        m_disabledTextColor = color;
    }

    /**
     * @brief 设置是否可写
     */
    bool canWrite() const {
        return isVisible() && isFocused() && m_enabled;
    }

protected:
    /**
     * @brief 失去焦点时调用
     */
    void onFocusLost() override {
        if (m_canLoseFocus) {
            // 清除选择
            if (!hasSelection()) {
                m_selectionEnd = m_cursorPosition;
            }
        }
    }

    /**
     * @brief 文本变化时调用
     */
    virtual void onTextChanged() {
        if (m_onTextChanged) {
            m_onTextChanged(m_text);
        }
    }

    /**
     * @brief 从光标位置删除字符
     */
    void deleteFromCursor(i32 delta) {
        if (hasSelection()) {
            deleteSelectedText();
            return;
        }

        if (m_text.empty()) return;

        i32 start;
        i32 end;

        if (delta < 0) {
            // 向前删除
            start = std::max(0, m_cursorPosition + delta);
            end = m_cursorPosition;
        } else {
            // 向后删除
            start = m_cursorPosition;
            end = std::min(static_cast<i32>(m_text.size()), m_cursorPosition + delta);
        }

        if (start != end) {
            String newText = m_text.substr(0, start) + m_text.substr(end);
            if (m_validator && !m_validator(newText)) return;

            m_text = newText;
            setCursorPosition(start);
            onTextChanged();
        }
    }

    /**
     * @brief 更新滚动偏移
     */
    void updateScrollOffset() {
        // TODO: 根据字体和可见宽度计算滚动偏移
    }

    /**
     * @brief 限制光标位置在有效范围内
     */
    [[nodiscard]] i32 clampPosition(i32 pos) const {
        return std::max(0, std::min(pos, static_cast<i32>(m_text.size())));
    }

    /**
     * @brief 检查字符是否允许
     */
    static bool isAllowedCharacter(u32 codePoint) {
        // 允许大部分Unicode字符，排除控制字符
        if (codePoint < 32) return false;
        if (codePoint == 127) return false; // DEL
        return true;
    }

    /**
     * @brief 过滤允许的字符
     */
    static String filterAllowedCharacters(const String& text) {
        String result;
        result.reserve(text.size());

        // 简单实现：保留所有非控制字符
        for (char c : text) {
            if (static_cast<unsigned char>(c) >= 32 || c == '\n') {
                result += c;
            }
        }

        return result;
    }

    /**
     * @brief Unicode码点转字符串
     */
    static String codePointToString(u32 codePoint) {
        String result;
        if (codePoint < 0x80) {
            result += static_cast<char>(codePoint);
        } else if (codePoint < 0x800) {
            result += static_cast<char>(0xC0 | (codePoint >> 6));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else if (codePoint < 0x10000) {
            result += static_cast<char>(0xE0 | (codePoint >> 12));
            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (codePoint >> 18));
            result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
        return result;
    }

    // 文本状态
    String m_text;                      ///< 当前文本
    String m_placeholder;               ///< 占位符文本
    i32 m_maxLength = 32;               ///< 最大长度
    i32 m_cursorPosition = 0;           ///< 光标位置
    i32 m_selectionEnd = 0;             ///< 选区结束位置
    i32 m_scrollOffset = 0;             ///< 滚动偏移
    i32 m_cursorBlinkCounter = 0;       ///< 光标闪烁计数器

    // 状态标志
    bool m_enabled = true;              ///< 是否启用
    bool m_canLoseFocus = true;         ///< 是否可以失去焦点
    bool m_drawBackground = true;       ///< 是否绘制背景
    bool m_shiftHeld = false;           ///< Shift键是否按下

    // 颜色
    u32 m_textColor = 0xE0E0E0;         ///< 文本颜色
    u32 m_disabledTextColor = 0x707070; ///< 禁用文本颜色
    u32 m_selectionColor = 0xFF0000FF;  ///< 选区颜色

    // 回调
    TextChangedCallback m_onTextChanged; ///< 文本变化回调
    TextValidator m_validator;          ///< 文本验证器
};

} // namespace mc::client::ui::kagero::widget
