#pragma once

#include "Widget.hpp"
#include <vector>
#include <memory>

namespace mc::client::ui::kagero::widget {

/**
 * @brief Widget容器接口
 *
 * 定义可以包含子组件的容器行为。
 * 实现 IWidgetContainer 的类可以管理一组子组件。
 *
 * 使用示例：
 * @code
 * class Panel : public Widget, public IWidgetContainer {
 * public:
 *     void addWidget(Widget::Ptr widget) override {
 *         widget->setParent(this);
 *         widget->init();
 *         m_children.push_back(std::move(widget));
 *     }
 *
 *     void paint(PaintContext& ctx) override {
 *         paintChildren(ctx);
 *     }
 * };
 * @endcode
 */
class IWidgetContainer {
public:
    virtual ~IWidgetContainer() = default;

    /**
     * @brief 添加子组件
     * @param widget 子组件
     */
    virtual void addWidget(Widget::Ptr widget) = 0;

    /**
     * @brief 移除子组件
     * @param widget 子组件指针
     */
    virtual void removeWidget(Widget* widget) = 0;

    /**
     * @brief 通过ID移除子组件
     * @param id 组件ID
     * @return 如果找到并移除返回true
     */
    virtual bool removeWidgetById(const String& id) = 0;

    /**
     * @brief 清空所有子组件
     */
    virtual void clearWidgets() = 0;

    /**
     * @brief 获取所有子组件
     */
    [[nodiscard]] virtual const std::vector<Widget::Ptr>& widgets() const = 0;

    /**
     * @brief 获取子组件数量
     */
    [[nodiscard]] virtual size_t widgetCount() const = 0;

    /**
     * @brief 通过ID查找子组件
     * @param id 组件ID
     * @return 组件指针，如果未找到返回nullptr
     */
    [[nodiscard]] virtual Widget* findWidgetById(const String& id) = 0;

    /**
     * @brief 通过ID查找子组件（const版本）
     */
    [[nodiscard]] virtual const Widget* findWidgetById(const String& id) const = 0;

    /**
     * @brief 获取指定位置的组件
     * @param x X坐标
     * @param y Y坐标
     * @return 组件指针，如果未找到返回nullptr
     *
     * 返回最上层（最后添加）的可见且激活的组件
     */
    [[nodiscard]] virtual Widget* getWidgetAt(i32 x, i32 y) = 0;

    /**
     * @brief 获取指定位置的组件（const版本）
     */
    [[nodiscard]] virtual const Widget* getWidgetAt(i32 x, i32 y) const = 0;

    /**
     * @brief 将子组件提升到顶层
     * @param widget 子组件指针
     */
    virtual void bringToFront(Widget* widget) = 0;

    /**
     * @brief 将子组件降低到底层
     * @param widget 子组件指针
     */
    virtual void sendToBack(Widget* widget) = 0;

    /**
     * @brief 遍历所有子组件
     * @param callback 回调函数
     */
    virtual void forEachWidget(const std::function<void(Widget&)>& callback) = 0;

    /**
     * @brief 遍历所有子组件（const版本）
     */
    virtual void forEachWidget(const std::function<void(const Widget&)>& callback) const = 0;
};

/**
 * @brief Widget容器混入类
 *
 * 提供IWidgetContainer的默认实现，可以混入到其他类中。
 * 使用CRTP模式。
 *
 * @tparam Derived 派生类类型
 *
 * 使用示例：
 * @code
 * class Panel : public Widget, public WidgetContainerMixin<Panel> {
 * public:
 *     using WidgetContainerMixin<Panel>::addWidget;
 *     using WidgetContainerMixin<Panel>::widgets;
 *
 *     void paint(PaintContext& ctx) override {
 *         paintChildren(ctx);
 *     }
 * };
 * @endcode
 */
template<typename Derived>
class WidgetContainerMixin : public IWidgetContainer {
public:
    WidgetContainerMixin() = default;
    ~WidgetContainerMixin() override = default;

    void addWidget(Widget::Ptr widget) override {
        if (widget) {
            widget->setParent(this);
            widget->init();
            m_children.push_back(std::move(widget));
        }
    }

    /**
     * @brief 添加子组件（别名，与文档一致）
     * @param widget 子组件
     */
    void addChild(Widget::Ptr widget) {
        addWidget(std::move(widget));
    }

    void removeWidget(Widget* widget) override {
        if (widget == nullptr) return;

        auto it = std::find_if(m_children.begin(), m_children.end(),
            [widget](const Widget::Ptr& ptr) {
                return ptr.get() == widget;
            });

        if (it != m_children.end()) {
            (*it)->setParent(nullptr);
            m_children.erase(it);
        }
    }

    /**
     * @brief 移除子组件（别名，与文档一致）
     * @param id 组件ID
     * @return 是否成功移除
     */
    bool removeChild(const String& id) {
        return removeWidgetById(id);
    }

    bool removeWidgetById(const String& id) override {
        auto it = std::find_if(m_children.begin(), m_children.end(),
            [&id](const Widget::Ptr& ptr) {
                return ptr->id() == id;
            });

        if (it != m_children.end()) {
            (*it)->setParent(nullptr);
            m_children.erase(it);
            return true;
        }
        return false;
    }

    void clearWidgets() override {
        for (auto& child : m_children) {
            child->setParent(nullptr);
        }
        m_children.clear();
    }

    /**
     * @brief 清空所有子组件（别名，与文档一致）
     */
    void clearChildren() {
        clearWidgets();
    }

    [[nodiscard]] const std::vector<Widget::Ptr>& widgets() const override {
        return m_children;
    }

    /**
     * @brief 获取子组件数量（别名，与文档一致）
     */
    [[nodiscard]] size_t childCount() const {
        return m_children.size();
    }

    [[nodiscard]] size_t widgetCount() const override {
        return m_children.size();
    }

    [[nodiscard]] Widget* findWidgetById(const String& id) override {
        auto it = std::find_if(m_children.begin(), m_children.end(),
            [&id](const Widget::Ptr& ptr) {
                return ptr->id() == id;
            });

        return (it != m_children.end()) ? it->get() : nullptr;
    }

    /**
     * @brief 通过ID查找子组件（别名，与文档一致）
     */
    [[nodiscard]] Widget* findChild(const String& id) {
        return findWidgetById(id);
    }

    [[nodiscard]] const Widget* findWidgetById(const String& id) const override {
        auto it = std::find_if(m_children.begin(), m_children.end(),
            [&id](const Widget::Ptr& ptr) {
                return ptr->id() == id;
            });

        return (it != m_children.end()) ? it->get() : nullptr;
    }

    [[nodiscard]] const Widget* findChild(const String& id) const {
        return findWidgetById(id);
    }

    [[nodiscard]] Widget* getWidgetAt(i32 x, i32 y) override {
        // 从后往前遍历（后添加的在上面）
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            Widget* widget = it->get();
            if (widget->isVisible() && widget->isActive() && widget->contains(x, y)) {
                return widget;
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Widget* getWidgetAt(i32 x, i32 y) const override {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            const Widget* widget = it->get();
            if (widget->isVisible() && widget->isActive() && widget->contains(x, y)) {
                return widget;
            }
        }
        return nullptr;
    }

    void bringToFront(Widget* widget) override {
        if (widget == nullptr) return;

        auto it = std::find_if(m_children.begin(), m_children.end(),
            [widget](const Widget::Ptr& ptr) {
                return ptr.get() == widget;
            });

        // 由于 getWidgetAt 从后往前遍历，"最前面"应该是列表末尾
        if (it != m_children.end() && it != m_children.end() - 1) {
            auto ptr = std::move(*it);
            m_children.erase(it);
            m_children.push_back(std::move(ptr));
        }
    }

    void sendToBack(Widget* widget) override {
        if (widget == nullptr) return;

        auto it = std::find_if(m_children.begin(), m_children.end(),
            [widget](const Widget::Ptr& ptr) {
                return ptr.get() == widget;
            });

        // 由于 getWidgetAt 从后往前遍历，"最后面"应该是列表开头
        if (it != m_children.end() && it != m_children.begin()) {
            auto ptr = std::move(*it);
            m_children.erase(it);
            m_children.insert(m_children.begin(), std::move(ptr));
        }
    }

    void forEachWidget(const std::function<void(Widget&)>& callback) override {
        for (auto& child : m_children) {
            callback(*child);
        }
    }

    void forEachWidget(const std::function<void(const Widget&)>& callback) const override {
        for (const auto& child : m_children) {
            callback(*child);
        }
    }

protected:
    /**
     * @brief 绘制所有子组件
     * @param ctx 绘图上下文
     */
    void paintChildren(PaintContext& ctx) {
        for (auto& child : m_children) {
            if (child->isVisible()) {
                child->paint(ctx);
            }
        }
    }

    /**
     * @brief 更新所有子组件
     * @param dt 增量时间
     */
    void tickChildren(f32 dt) {
        for (auto& child : m_children) {
            if (child->isVisible() && child->isActive()) {
                child->tick(dt);
            }
        }
    }

    /**
     * @brief 处理子组件的点击事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @return 如果有组件处理了事件返回true
     */
    bool handleClickInChildren(i32 mouseX, i32 mouseY, i32 button) {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onClick(mouseX, mouseY, button);
        }
        return false;
    }

    /**
     * @brief 处理子组件的释放事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @return 如果有组件处理了事件返回true
     */
    bool handleReleaseInChildren(i32 mouseX, i32 mouseY, i32 button) {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onRelease(mouseX, mouseY, button);
        }
        return false;
    }

    std::vector<Widget::Ptr> m_children;
};

} // namespace mc::client::ui::kagero::widget
