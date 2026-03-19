#pragma once

#include "Event.hpp"
#include "../Types.hpp"

namespace mc::client::ui::kagero::event {

/**
 * @brief 焦点获得事件
 */
class FocusGainedEvent : public Event {
public:
    FocusGainedEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::FocusGained; }
    [[nodiscard]] const char* getName() const override { return "FocusGained"; }
    [[nodiscard]] bool bubbles() const override { return false; }
};

/**
 * @brief 焦点失去事件
 */
class FocusLostEvent : public Event {
public:
    FocusLostEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::FocusLost; }
    [[nodiscard]] const char* getName() const override { return "FocusLost"; }
    [[nodiscard]] bool bubbles() const override { return false; }
};

/**
 * @brief 组件初始化事件
 */
class WidgetInitEvent : public Event {
public:
    WidgetInitEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetInit; }
    [[nodiscard]] const char* getName() const override { return "WidgetInit"; }
    [[nodiscard]] bool bubbles() const override { return false; }
};

/**
 * @brief 组件尺寸改变事件
 */
class WidgetResizeEvent : public Event {
public:
    WidgetResizeEvent(i32 oldWidth, i32 oldHeight, i32 newWidth, i32 newHeight)
        : m_oldWidth(oldWidth), m_oldHeight(oldHeight)
        , m_newWidth(newWidth), m_newHeight(newHeight) {}

    [[nodiscard]] EventType getType() const override { return EventType::WidgetResize; }
    [[nodiscard]] const char* getName() const override { return "WidgetResize"; }

    [[nodiscard]] i32 oldWidth() const { return m_oldWidth; }
    [[nodiscard]] i32 oldHeight() const { return m_oldHeight; }
    [[nodiscard]] i32 newWidth() const { return m_newWidth; }
    [[nodiscard]] i32 newHeight() const { return m_newHeight; }

private:
    i32 m_oldWidth;
    i32 m_oldHeight;
    i32 m_newWidth;
    i32 m_newHeight;
};

/**
 * @brief 组件显示事件
 */
class WidgetShowEvent : public Event {
public:
    WidgetShowEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetShow; }
    [[nodiscard]] const char* getName() const override { return "WidgetShow"; }
};

/**
 * @brief 组件隐藏事件
 */
class WidgetHideEvent : public Event {
public:
    WidgetHideEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetHide; }
    [[nodiscard]] const char* getName() const override { return "WidgetHide"; }
};

/**
 * @brief 组件启用事件
 */
class WidgetEnableEvent : public Event {
public:
    WidgetEnableEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetEnable; }
    [[nodiscard]] const char* getName() const override { return "WidgetEnable"; }
};

/**
 * @brief 组件禁用事件
 */
class WidgetDisableEvent : public Event {
public:
    WidgetDisableEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetDisable; }
    [[nodiscard]] const char* getName() const override { return "WidgetDisable"; }
};

/**
 * @brief 屏幕打开事件
 */
class ScreenOpenEvent : public Event {
public:
    explicit ScreenOpenEvent(const String& screenId)
        : m_screenId(screenId) {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ScreenOpen"; }

    [[nodiscard]] const String& screenId() const { return m_screenId; }

private:
    String m_screenId;
};

/**
 * @brief 屏幕关闭事件
 */
class ScreenCloseEvent : public Event {
public:
    explicit ScreenCloseEvent(const String& screenId)
        : m_screenId(screenId) {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ScreenClose"; }

    [[nodiscard]] const String& screenId() const { return m_screenId; }

private:
    String m_screenId;
};

/**
 * @brief 屏幕切换事件
 */
class ScreenChangeEvent : public Event {
public:
    ScreenChangeEvent(const String& fromScreen, const String& toScreen)
        : m_fromScreen(fromScreen), m_toScreen(toScreen) {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ScreenChange"; }

    [[nodiscard]] const String& fromScreen() const { return m_fromScreen; }
    [[nodiscard]] const String& toScreen() const { return m_toScreen; }

private:
    String m_fromScreen;
    String m_toScreen;
};

} // namespace mc::client::ui::kagero::event
