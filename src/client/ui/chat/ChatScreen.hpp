#pragma once

#include "ChatHistory.hpp"
#include "client/input/InputManager.hpp"
#include <functional>
#include <memory>

namespace mc::client {
class Font;
}

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client {
// 引入GuiRenderer到mc::client命名空间（向后兼容）
using GuiRenderer = renderer::trident::gui::GuiRenderer;
}

namespace mc::client {

/**
 * @brief 聊天屏幕状态
 */
enum class ChatState {
    Closed,         ///< 聊天框关闭（仅显示最近消息）
    Open,           ///< 聊天框打开（可以输入）
    OpenCommand     ///< 聊天框打开，输入命令（以 / 开头）
};

/**
 * @brief 聊天屏幕
 *
 * 处理聊天框的显示和交互，包括：
 * - 消息显示
 * - 文本输入
 * - 命令历史导航
 * - 光标和选区
 *
 * 参考 MC 的 ChatScreen 实现
 */
class ChatScreen {
public:
    /**
     * @brief 命令发送回调
     */
    using CommandCallback = std::function<void(const String&)>;

    ChatScreen() = default;
    ~ChatScreen() = default;

    // 禁止拷贝
    ChatScreen(const ChatScreen&) = delete;
    ChatScreen& operator=(const ChatScreen&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化聊天屏幕
     * @param font 字体
     */
    void initialize(Font* font);

    /**
     * @brief 设置命令回调
     * @param callback 发送命令时的回调
     */
    void setCommandCallback(CommandCallback callback) { m_commandCallback = std::move(callback); }

    // ========== 状态 ==========

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] ChatState state() const { return m_state; }

    /**
     * @brief 检查是否打开
     */
    [[nodiscard]] bool isOpen() const { return m_state != ChatState::Closed; }

    /**
     * @brief 打开聊天框
     * @param command 是否以命令模式打开（自动填入 /）
     */
    void open(bool command = false);

    /**
     * @brief 关闭聊天框
     */
    void close();

    /**
     * @brief 切换聊天框状态
     */
    void toggle();

    // ========== 输入处理 ==========

    /**
     * @brief 处理字符输入
     * @param codepoint Unicode码点
     * @return 是否处理了输入
     */
    bool onCharInput(u32 codepoint);

    /**
     * @brief 处理按键
     * @param key GLFW键码
     * @param action GLFW动作
     * @param mods 修饰键
     * @return 是否处理了按键
     */
    bool onKeyInput(i32 key, i32 action, i32 mods);

    /**
     * @brief 处理鼠标点击
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @param button 鼠标按键
     * @return 是否处理了点击
     */
    bool onMouseClick(f64 x, f64 y, i32 button);

    // ========== 渲染 ==========

    /**
     * @brief 渲染聊天框
     * @param gui GUI渲染器
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     */
    void render(GuiRenderer& gui, f32 screenWidth, f32 screenHeight);

    // ========== 消息 ==========

    /**
     * @brief 添加聊天消息
     */
    void addMessage(const String& message, u32 color = 0xFFFFFFFF);

    /**
     * @brief 添加系统消息
     */
    void addSystemMessage(const String& message);

    /**
     * @brief 获取聊天历史
     */
    [[nodiscard]] ChatHistory& history() { return m_history; }
    [[nodiscard]] const ChatHistory& history() const { return m_history; }

    // ========== 输入 ==========

    /**
     * @brief 获取当前输入
     */
    [[nodiscard]] const String& input() const { return m_input; }

    /**
     * @brief 设置输入文本
     */
    void setInput(const String& text);

    /**
     * @brief 清除输入
     */
    void clearInput();

    /**
     * @brief 获取光标位置
     */
    [[nodiscard]] size_t cursorPosition() const { return m_cursorPos; }

    /**
     * @brief 设置光标位置
     */
    void setCursorPosition(size_t pos);

private:
    // ========== 内部方法 ==========

    /**
     * @brief 插入文本到光标位置
     */
    void insertText(const String& text);

    /**
     * @brief 删除选中的文本
     */
    void deleteSelection();

    /**
     * @brief 删除光标前的字符
     */
    void deleteBeforeCursor();

    /**
     * @brief 删除光标后的字符
     */
    void deleteAfterCursor();

    /**
     * @brief 移动光标
     * @param offset 偏移量（负数向前，正数向后）
     * @param selecting 是否选择
     */
    void moveCursor(i32 offset, bool selecting = false);

    /**
     * @brief 移动光标到行首/行尾
     * @param start 是否移动到行首
     * @param selecting 是否选择
     */
    void moveCursorToEdge(bool start, bool selecting = false);

    /**
     * @brief 获取光标位置对应的像素偏移
     */
    [[nodiscard]] f32 getCursorPixelPosition() const;

    /**
     * @brief 发送当前输入
     */
    void sendInput();

    /**
     * @brief 更新光标闪烁
     */
    void updateCursorBlink();

    // ========== 渲染辅助 ==========

    /**
     * @brief 渲染消息列表
     */
    void renderMessages(GuiRenderer& gui, f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染输入框
     */
    void renderInputBox(GuiRenderer& gui, f32 screenWidth, f32 screenHeight);

    /**
     * @brief 渲染光标
     */
    void renderCursor(GuiRenderer& gui, f32 x, f32 y);

    // ========== 成员变量 ==========

    ChatHistory m_history;
    String m_input;
    size_t m_cursorPos = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    bool m_hasSelection = false;
    ChatState m_state = ChatState::Closed;

    Font* m_font = nullptr;

    // 光标闪烁
    std::chrono::steady_clock::time_point m_lastBlinkTime;
    bool m_cursorVisible = true;
    static constexpr float CURSOR_BLINK_RATE = 0.5f;

    // 输入框尺寸
    static constexpr f32 INPUT_BOX_HEIGHT = 20.0f;
    static constexpr f32 INPUT_BOX_PADDING = 4.0f;
    static constexpr f32 CHAT_WIDTH_RATIO = 0.4f;  ///< 屏幕宽度的40%
    static constexpr f32 MESSAGE_FADE_START = 3.0f; ///< 消息开始淡出的时间

    // 命令回调
    CommandCallback m_commandCallback;

    // 鼠标拖拽
    bool m_isDragging = false;
    f64 m_dragStartX = 0;
    f64 m_dragStartY = 0;
};

} // namespace mc::client
