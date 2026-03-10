#include "ChatHistory.hpp"

namespace mr::client {

void ChatHistory::addMessage(const String& message, u32 color, bool permanent) {
    m_messages.emplace_front(message, color, permanent);

    // 限制消息数量
    while (m_messages.size() > MAX_MESSAGES) {
        m_messages.pop_back();
    }
}

void ChatHistory::addSystemMessage(const String& message) {
    // 系统消息使用黄色
    addMessage(message, 0xFFAAAAAA, false);
}

void ChatHistory::clear() {
    m_messages.clear();
}

std::vector<ChatMessage> ChatHistory::getVisibleMessages(bool includeFading) const {
    std::vector<ChatMessage> result;
    auto now = std::chrono::steady_clock::now();

    size_t count = 0;
    for (const auto& msg : m_messages) {
        if (count >= MAX_VISIBLE) break;

        if (msg.permanent) {
            result.push_back(msg);
            count++;
        } else if (includeFading) {
            // 计算消息年龄
            auto age = std::chrono::duration<float>(now - msg.timestamp).count();
            if (age < MESSAGE_FADE_TIME + 1.0f) {  // 额外1秒淡出时间
                result.push_back(msg);
                count++;
            }
        }
    }

    return result;
}

void ChatHistory::addToInputHistory(const String& input) {
    if (input.empty()) return;

    // 避免重复
    if (!m_inputHistory.empty() && m_inputHistory.back() == input) return;

    m_inputHistory.push_back(input);

    // 限制历史大小
    while (m_inputHistory.size() > MAX_INPUT_HISTORY) {
        m_inputHistory.erase(m_inputHistory.begin());
    }

    // 重置导航索引
    m_historyIndex = m_inputHistory.size();
}

String ChatHistory::getPreviousInput() {
    if (m_inputHistory.empty()) return "";

    if (m_historyIndex == m_inputHistory.size()) {
        // 保存当前输入
        m_savedInput = "";
    }

    if (m_historyIndex > 0) {
        m_historyIndex--;
        return m_inputHistory[m_historyIndex];
    }

    return m_inputHistory[0];
}

String ChatHistory::getNextInput() {
    if (m_inputHistory.empty()) return "";

    if (m_historyIndex < m_inputHistory.size() - 1) {
        m_historyIndex++;
        return m_inputHistory[m_historyIndex];
    }

    // 到达底部，返回保存的输入
    m_historyIndex = m_inputHistory.size();
    return m_savedInput;
}

void ChatHistory::resetInputNavigation() {
    m_historyIndex = m_inputHistory.size();
    m_savedInput.clear();
}

void ChatHistory::clearInputHistory() {
    m_inputHistory.clear();
    m_historyIndex = 0;
    m_savedInput.clear();
}

} // namespace mr::client
