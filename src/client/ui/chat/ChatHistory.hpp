#pragma once

#include "common/core/Types.hpp"
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <chrono>

namespace mr::client {

/**
 * @brief 聊天消息
 */
struct ChatMessage {
    String text;                              ///< 消息文本
    u32 color = 0xFFFFFFFF;                   ///< ARGB颜色
    std::chrono::steady_clock::time_point timestamp;  ///< 时间戳
    bool permanent = false;                   ///< 是否永久显示（不淡出）

    ChatMessage() = default;
    ChatMessage(const String& t, u32 c = 0xFFFFFFFF, bool perm = false)
        : text(t), color(c), timestamp(std::chrono::steady_clock::now()), permanent(perm) {}
};

/**
 * @brief 聊天历史管理器
 *
 * 管理聊天消息历史，支持：
 * - 消息添加和过期
 * - 消息历史导航
 * - 命令历史
 */
class ChatHistory {
public:
    static constexpr size_t MAX_MESSAGES = 100;      ///< 最大消息数
    static constexpr size_t MAX_VISIBLE = 10;        ///< 最大可见消息数
    static constexpr size_t MAX_INPUT_HISTORY = 50;  ///< 最大输入历史
    static constexpr float MESSAGE_FADE_TIME = 5.0f; ///< 消息淡出时间（秒）

    ChatHistory() = default;

    // ========== 消息管理 ==========

    /**
     * @brief 添加聊天消息
     * @param message 消息文本
     * @param color 消息颜色（ARGB）
     * @param permanent 是否永久显示
     */
    void addMessage(const String& message, u32 color = 0xFFFFFFFF, bool permanent = false);

    /**
     * @brief 添加系统消息
     */
    void addSystemMessage(const String& message);

    /**
     * @brief 清除所有消息
     */
    void clear();

    /**
     * @brief 获取可见消息
     * @param includeFading 是否包含正在淡出的消息
     */
    [[nodiscard]] std::vector<ChatMessage> getVisibleMessages(bool includeFading = true) const;

    /**
     * @brief 获取所有消息
     */
    [[nodiscard]] const std::deque<ChatMessage>& allMessages() const { return m_messages; }

    // ========== 输入历史 ==========

    /**
     * @brief 添加到输入历史
     * @param input 输入文本
     */
    void addToInputHistory(const String& input);

    /**
     * @brief 获取上一个输入
     */
    [[nodiscard]] String getPreviousInput();

    /**
     * @brief 获取下一个输入
     */
    [[nodiscard]] String getNextInput();

    /**
     * @brief 重置输入历史导航
     */
    void resetInputNavigation();

    /**
     * @brief 清除输入历史
     */
    void clearInputHistory();

private:
    std::deque<ChatMessage> m_messages;
    std::vector<String> m_inputHistory;
    size_t m_historyIndex = 0;
    String m_savedInput;  ///< 导航时保存的当前输入
};

} // namespace mr::client
