#pragma once

#include "Widget.hpp"
#include "IWidgetContainer.hpp"
#include <memory>
#include <vector>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 可滚动容器组件
 *
 * 提供滚动功能的容器，可以包含其他组件。
 *
 * 参考MC 1.16.5 AbstractList.java实现
 *
 * 使用示例：
 * @code
 * auto scrollable = std::make_unique<ScrollableWidget>("scroll", 10, 10, 200, 300);
 * scrollable->setContentHeight(1000);
 * scrollable->addWidget(std::make_unique<TextWidget>("item1", 0, 0, 180, 30, "Item 1"));
 * @endcode
 */
class ScrollableWidget : public Widget, public WidgetContainerMixin<ScrollableWidget> {
public:
    using WidgetContainerMixin<ScrollableWidget>::addWidget;
    using WidgetContainerMixin<ScrollableWidget>::widgets;
    using WidgetContainerMixin<ScrollableWidget>::findWidgetById;
    using WidgetContainerMixin<ScrollableWidget>::getWidgetAt;

    /**
     * @brief 默认构造函数
     */
    ScrollableWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    ScrollableWidget(String id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void init() override {
        for (auto& child : m_children) {
            child->init();
        }
    }

    void tick(f32 dt) override {
        if (!isVisible() || !isActive()) return;
        tickChildren(dt);
    }

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)ctx;
        (void)partialTick;

        if (!isVisible()) return;

        // 更新悬停状态
        setHovered(isMouseOver(mouseX, mouseY));

        // 计算可见区域
        i32 visibleX = m_bounds.x + m_padding.left;
        i32 visibleY = m_bounds.y + m_padding.top;
        i32 visibleWidth = m_bounds.width - m_padding.horizontal();
        i32 visibleHeight = m_bounds.height - m_padding.vertical();

        // TODO: 实际渲染逻辑
        // 1. 设置裁剪区域
        // 2. 根据滚动偏移调整子组件位置
        // 3. 渲染子组件
        // 4. 渲染滚动条

        // 调整子组件渲染位置
        i32 adjustedMouseY = mouseY + m_scrollY;

        for (auto& child : m_children) {
            if (!child->isVisible()) continue;

            // 检查是否在可见区域内
            Rect childBounds = child->bounds();
            childBounds.y -= m_scrollY;

            if (childBounds.bottom() >= visibleY && childBounds.y < visibleY + visibleHeight) {
                // TODO: 设置裁剪区域并渲染
                // child->render(ctx, mouseX, adjustedMouseY, partialTick);
            }
        }

        // 渲染滚动条
        if (m_showScrollbar && m_contentHeight > visibleHeight) {
            renderScrollbar(ctx, mouseX, mouseY);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        if (!isActive() || !isVisible()) return false;

        // 检查是否点击滚动条
        if (m_showScrollbar && isOnScrollbar(mouseX, mouseY)) {
            m_draggingScrollbar = true;
            m_lastMouseY = mouseY;
            return true;
        }

        // 调整Y坐标并传递给子组件
        i32 adjustedY = mouseY + m_scrollY;
        return handleClickInChildren(mouseX, adjustedY, button);
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button) override {
        (void)mouseX;
        (void)mouseY;

        if (button != 0) return false;

        if (m_draggingScrollbar) {
            m_draggingScrollbar = false;
            return true;
        }

        i32 adjustedY = mouseY + m_scrollY;
        return handleReleaseInChildren(mouseX, adjustedY, button);
    }

    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) override {
        (void)deltaX;

        if (m_draggingScrollbar) {
            // 滚动条拖动
            i32 visibleHeight = m_bounds.height - m_padding.vertical();
            f64 scrollRatio = static_cast<f64>(deltaY) / visibleHeight;
            m_scrollY += static_cast<i32>(scrollRatio * m_contentHeight);
            clampScroll();
            return true;
        }

        i32 adjustedY = mouseY + m_scrollY;
        return WidgetContainerMixin<ScrollableWidget>::handleDragInChildren(mouseX, adjustedY, deltaX, deltaY);
    }

    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        // 滚动内容
        m_scrollY -= static_cast<i32>(delta * m_scrollSpeed);
        clampScroll();

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override {
        if (!isActive() || !isVisible() || !isFocused()) return false;

        // 处理上下键滚动
        if (action == 1 || action == 2) {
            switch (key) {
                case 264: // GLFW_KEY_DOWN
                    scrollBy(20);
                    return true;

                case 265: // GLFW_KEY_UP
                    scrollBy(-20);
                    return true;

                case 266: // GLFW_KEY_PAGE_DOWN
                    scrollBy(m_bounds.height - m_padding.vertical());
                    return true;

                case 267: // GLFW_KEY_PAGE_UP
                    scrollBy(-(m_bounds.height - m_padding.vertical()));
                    return true;

                default:
                    break;
            }
        }

        // 传递给子组件
        for (auto& child : m_children) {
            if (child->isVisible() && child->isActive() && child->isFocused()) {
                if (child->onKey(key, scanCode, action, mods)) {
                    return true;
                }
            }
        }

        return false;
    }

    // ==================== 滚动操作 ====================

    /**
     * @brief 设置内容高度
     */
    void setContentHeight(i32 height) {
        m_contentHeight = height;
        clampScroll();
    }

    /**
     * @brief 获取内容高度
     */
    [[nodiscard]] i32 contentHeight() const { return m_contentHeight; }

    /**
     * @brief 设置滚动位置
     */
    void setScrollY(i32 scrollY) {
        m_scrollY = scrollY;
        clampScroll();
    }

    /**
     * @brief 获取滚动位置
     */
    [[nodiscard]] i32 scrollY() const { return m_scrollY; }

    /**
     * @brief 滚动指定距离
     */
    void scrollBy(i32 delta) {
        m_scrollY += delta;
        clampScroll();
    }

    /**
     * @brief 滚动到顶部
     */
    void scrollToTop() {
        m_scrollY = 0;
    }

    /**
     * @brief 滚动到底部
     */
    void scrollToBottom() {
        i32 visibleHeight = m_bounds.height - m_padding.vertical();
        m_scrollY = std::max(0, m_contentHeight - visibleHeight);
    }

    /**
     * @brief 滚动到指定位置
     */
    void scrollTo(i32 y) {
        m_scrollY = y;
        clampScroll();
    }

    /**
     * @brief 滚动到使指定位置可见
     */
    void scrollIntoView(i32 y, i32 height = 0) {
        i32 visibleHeight = m_bounds.height - m_padding.vertical();
        i32 viewTop = m_scrollY;
        i32 viewBottom = m_scrollY + visibleHeight;

        if (y < viewTop) {
            m_scrollY = y;
        } else if (y + height > viewBottom) {
            m_scrollY = y + height - visibleHeight;
        }
        clampScroll();
    }

    // ==================== 显示属性 ====================

    /**
     * @brief 设置是否显示滚动条
     */
    void setShowScrollbar(bool show) {
        m_showScrollbar = show;
    }

    /**
     * @brief 是否显示滚动条
     */
    [[nodiscard]] bool showScrollbar() const { return m_showScrollbar; }

    /**
     * @brief 设置滚动速度
     */
    void setScrollSpeed(f64 speed) {
        m_scrollSpeed = speed;
    }

    /**
     * @brief 获取滚动速度
     */
    [[nodiscard]] f64 scrollSpeed() const { return m_scrollSpeed; }

    /**
     * @brief 设置滚动条宽度
     */
    void setScrollbarWidth(i32 width) {
        m_scrollbarWidth = width;
    }

    /**
     * @brief 获取滚动条宽度
     */
    [[nodiscard]] i32 scrollbarWidth() const { return m_scrollbarWidth; }

    /**
     * @brief 获取可见区域高度
     */
    [[nodiscard]] i32 visibleHeight() const {
        return m_bounds.height - m_padding.vertical();
    }

    /**
     * @brief 获取可见区域宽度
     */
    [[nodiscard]] i32 visibleWidth() const {
        return m_bounds.width - m_padding.horizontal() - (m_showScrollbar ? m_scrollbarWidth : 0);
    }

    /**
     * @brief 计算滚动比例（0.0-1.0）
     */
    [[nodiscard]] f64 scrollRatio() const {
        i32 maxScroll = m_contentHeight - visibleHeight();
        if (maxScroll <= 0) return 0.0;
        return static_cast<f64>(m_scrollY) / maxScroll;
    }

protected:
    /**
     * @brief 限制滚动范围
     */
    void clampScroll() {
        i32 maxScroll = std::max(0, m_contentHeight - visibleHeight());
        m_scrollY = std::max(0, std::min(m_scrollY, maxScroll));
    }

    /**
     * @brief 检查是否在滚动条上
     */
    [[nodiscard]] bool isOnScrollbar(i32 mouseX, i32 mouseY) const {
        if (!m_showScrollbar) return false;

        i32 scrollbarX = m_bounds.right() - m_scrollbarWidth;
        return mouseX >= scrollbarX && mouseX < m_bounds.right() &&
               mouseY >= m_bounds.y && mouseY < m_bounds.bottom();
    }

    /**
     * @brief 渲染滚动条
     */
    void renderScrollbar(RenderContext& ctx, i32 mouseX, i32 mouseY) {
        (void)ctx;
        (void)mouseX;
        (void)mouseY;

        // TODO: 实际渲染滚动条
        // 1. 计算滚动条位置和高度
        // 2. 绘制滚动条背景和滑块
    }

    // 处理子组件拖动
    bool handleDragInChildren(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onDrag(mouseX, mouseY, deltaX, deltaY);
        }
        return false;
    }

    // 内容
    i32 m_contentHeight = 0;            ///< 内容高度
    i32 m_scrollY = 0;                  ///< 垂直滚动位置

    // 滚动条
    bool m_showScrollbar = true;        ///< 是否显示滚动条
    i32 m_scrollbarWidth = 6;           ///< 滚动条宽度
    f64 m_scrollSpeed = 20.0;           ///< 滚动速度

    // 状态
    bool m_draggingScrollbar = false;   ///< 是否正在拖动滚动条
    i32 m_lastMouseY = 0;               ///< 上次鼠标Y位置
};

} // namespace mc::client::ui::kagero::widget
