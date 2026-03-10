#include "client/ui/screen/ScreenManager.hpp"

namespace mr::client {

ScreenManager& ScreenManager::instance() {
    static ScreenManager instance;
    return instance;
}

void ScreenManager::openScreen(std::unique_ptr<IScreen> screen) {
    if (screen) {
        screen->init();
        m_screens.push_back(std::move(screen));

        if (m_onScreenChange) {
            m_onScreenChange(getCurrentScreen());
        }
    }
}

void ScreenManager::closeScreen() {
    if (!m_screens.empty()) {
        m_screens.back()->onClose();
        m_screens.pop_back();

        if (m_onScreenChange) {
            m_onScreenChange(getCurrentScreen());
        }
    }
}

void ScreenManager::closeAll() {
    while (!m_screens.empty()) {
        m_screens.back()->onClose();
        m_screens.pop_back();
    }

    if (m_onScreenChange) {
        m_onScreenChange(nullptr);
    }
}

void ScreenManager::tick(f32 dt) {
    if (!m_screens.empty()) {
        m_screens.back()->tick(dt);
    }
}

void ScreenManager::render(i32 mouseX, i32 mouseY, f32 partialTick) {
    // 从底部向上渲染，这样上层屏幕会覆盖下层
    for (auto& screen : m_screens) {
        screen->render(mouseX, mouseY, partialTick);
    }
}

bool ScreenManager::onClick(i32 mouseX, i32 mouseY, i32 button) {
    if (!m_screens.empty()) {
        IScreen* screen = m_screens.back().get();
        if (screen->onClick(mouseX, mouseY, button)) {
            m_isDragging = true;
            m_dragButton = button;
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            return true;
        }
    }
    return false;
}

bool ScreenManager::onRelease(i32 mouseX, i32 mouseY, i32 button) {
    if (m_isDragging && button == m_dragButton) {
        m_isDragging = false;
    }

    if (!m_screens.empty()) {
        return m_screens.back()->onRelease(mouseX, mouseY, button);
    }
    return false;
}

bool ScreenManager::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) {
    if (m_isDragging && !m_screens.empty()) {
        IScreen* screen = m_screens.back().get();
        bool handled = screen->onDrag(mouseX, mouseY, deltaX, deltaY);
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        return handled;
    }
    return false;
}

bool ScreenManager::onScroll(i32 mouseX, i32 mouseY, f64 delta) {
    if (!m_screens.empty()) {
        return m_screens.back()->onScroll(mouseX, mouseY, delta);
    }
    return false;
}

bool ScreenManager::onKey(i32 key, i32 scanCode, i32 action, i32 mods) {
    if (!m_screens.empty()) {
        return m_screens.back()->onKey(key, scanCode, action, mods);
    }
    return false;
}

bool ScreenManager::onChar(u32 codePoint) {
    if (!m_screens.empty()) {
        return m_screens.back()->onChar(codePoint);
    }
    return false;
}

void ScreenManager::onResize(i32 width, i32 height) {
    for (auto& screen : m_screens) {
        screen->onResize(width, height);
    }
}

bool ScreenManager::shouldPauseGame() const {
    if (m_screens.empty()) {
        return false;
    }

    // 如果有任何暂停屏幕，游戏暂停
    for (const auto& screen : m_screens) {
        if (screen->isPauseScreen()) {
            return true;
        }
    }
    return false;
}

} // namespace mr::client
