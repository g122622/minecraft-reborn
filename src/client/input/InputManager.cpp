#include "InputManager.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mr::client {

void InputManager::initialize(GLFWwindow* window)
{
    m_window = window;

    // 设置回调
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // 初始化鼠标位置
    glfwGetCursorPos(window, &m_mouseX, &m_mouseY);
    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;

    m_initialized = true;
    spdlog::info("InputManager initialized");
}

void InputManager::update()
{
    // 清除"刚按下/刚释放"状态
    m_keysJustPressed.clear();
    m_keysJustReleased.clear();
    m_mouseButtonsJustPressed.clear();
    m_mouseButtonsJustReleased.clear();

    // 重置滚轮增量
    m_scrollDeltaX = 0.0;
    m_scrollDeltaY = 0.0;

    // 计算鼠标增量
    m_mouseDeltaX = m_mouseX - m_lastMouseX;
    m_mouseDeltaY = m_mouseY - m_lastMouseY;
    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;

    // 触发按键绑定
    for (const auto& key : m_keysJustPressed) {
        auto it = m_keyBindings.find(key);
        if (it != m_keyBindings.end()) {
            auto callbackIt = m_actionCallbacks.find(it->second);
            if (callbackIt != m_actionCallbacks.end() && callbackIt->second) {
                callbackIt->second();
            }
        }
    }
}

bool InputManager::isKeyPressed(i32 key) const
{
    return m_keysPressed.count(key) > 0;
}

bool InputManager::isKeyJustPressed(i32 key) const
{
    return m_keysJustPressed.count(key) > 0;
}

bool InputManager::isKeyJustReleased(i32 key) const
{
    return m_keysJustReleased.count(key) > 0;
}

bool InputManager::isMouseButtonPressed(i32 button) const
{
    return m_mouseButtonsPressed.count(button) > 0;
}

bool InputManager::isMouseButtonJustPressed(i32 button) const
{
    return m_mouseButtonsJustPressed.count(button) > 0;
}

bool InputManager::isMouseButtonJustReleased(i32 button) const
{
    return m_mouseButtonsJustReleased.count(button) > 0;
}

void InputManager::setMouseLocked(bool locked)
{
    m_mouseLocked = locked;
    if (m_window) {
        glfwSetInputMode(
            m_window,
            GLFW_CURSOR,
            locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
        );
        // 切换模式后重置鼠标状态，避免跳跃
        glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
    }
}

void InputManager::bindKeyAction(i32 key, const String& action)
{
    m_keyBindings[key] = action;
}

void InputManager::bindActionCallback(const String& action, ActionCallback callback)
{
    m_actionCallbacks[action] = std::move(callback);
}

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input && key >= 0) {
        input->handleKey(key, action);
    }
}

void InputManager::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
        input->handleMouseMove(xpos, ypos);
    }
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input && button >= 0) {
        input->handleMouseButton(button, action);
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* input = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
        input->handleScroll(xoffset, yoffset);
    }
}

void InputManager::handleKey(i32 key, i32 action)
{
    if (action == GLFW_PRESS) {
        m_keysPressed.insert(key);
        m_keysJustPressed.insert(key);
        m_keysJustReleased.erase(key);

        // 触发按键绑定
        auto it = m_keyBindings.find(key);
        if (it != m_keyBindings.end()) {
            auto callbackIt = m_actionCallbacks.find(it->second);
            if (callbackIt != m_actionCallbacks.end() && callbackIt->second) {
                callbackIt->second();
            }
        }
    } else if (action == GLFW_RELEASE) {
        m_keysPressed.erase(key);
        m_keysJustPressed.erase(key);
        m_keysJustReleased.insert(key);
    }
}

void InputManager::handleMouseButton(i32 button, i32 action)
{
    if (action == GLFW_PRESS) {
        m_mouseButtonsPressed.insert(button);
        m_mouseButtonsJustPressed.insert(button);
        m_mouseButtonsJustReleased.erase(button);
    } else if (action == GLFW_RELEASE) {
        m_mouseButtonsPressed.erase(button);
        m_mouseButtonsJustPressed.erase(button);
        m_mouseButtonsJustReleased.insert(button);
    }
}

void InputManager::handleMouseMove(f64 x, f64 y)
{
    m_mouseX = x;
    m_mouseY = y;
}

void InputManager::handleScroll(f64 x, f64 y)
{
    m_scrollDeltaX = x;
    m_scrollDeltaY = y;
}

} // namespace mr::client
