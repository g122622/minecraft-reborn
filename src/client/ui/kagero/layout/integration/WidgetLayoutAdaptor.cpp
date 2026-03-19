#include "WidgetLayoutAdaptor.hpp"
#include <algorithm>

namespace mc::client::ui::kagero::layout {

// ============================================================================
// WidgetLayoutAdaptor 实现
// ============================================================================

WidgetLayoutAdaptor::WidgetLayoutAdaptor(widget::Widget* widget)
    : m_widget(widget) {
    if (m_widget) {
        // 从Widget获取初始边距和内边距
        m_constraints.margin = m_widget->margin();
        m_constraints.padding = m_widget->padding();
    }
}

const String& WidgetLayoutAdaptor::id() const {
    static const String empty;
    return m_widget ? m_widget->id() : empty;
}

Size WidgetLayoutAdaptor::currentSize() const {
    if (!m_widget) return Size();
    return Size(m_widget->width(), m_widget->height());
}

Rect WidgetLayoutAdaptor::currentBounds() const {
    if (!m_widget) return Rect();
    return m_widget->bounds();
}

Margin WidgetLayoutAdaptor::margin() const {
    return m_constraints.margin;
}

Padding WidgetLayoutAdaptor::padding() const {
    return m_constraints.padding;
}

Size WidgetLayoutAdaptor::measure(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec) {
    // 检查缓存是否有效
    if (m_cacheValid && m_lastWidthSpec == widthSpec && m_lastHeightSpec == heightSpec) {
        return m_lastMeasuredSize;
    }

    Size result;

    // 使用自定义测量函数
    if (m_measureFunc) {
        result = m_measureFunc(this, widthSpec, heightSpec);
    } else {
        result = measureDefault(widthSpec, heightSpec);
    }

    // 应用约束
    result.width = m_constraints.clampWidth(result.width);
    result.height = m_constraints.clampHeight(result.height);

    // 缓存结果
    m_lastMeasuredSize = result;
    m_lastWidthSpec = widthSpec;
    m_lastHeightSpec = heightSpec;
    m_cacheValid = true;

    return result;
}

Size WidgetLayoutAdaptor::measureDefault(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec) {
    if (!m_widget) return Size();

    Size result;

    // 优先使用约束中的preferredSize
    i32 preferredWidth = m_constraints.preferredWidth;
    i32 preferredHeight = m_constraints.preferredHeight;

    // 如果没有设置preferredSize，尝试从Widget获取
    if (preferredWidth < 0 && m_widget->width() > 0) {
        preferredWidth = m_widget->width();
    }
    if (preferredHeight < 0 && m_widget->height() > 0) {
        preferredHeight = m_widget->height();
    }

    // 解析宽度
    if (preferredWidth >= 0) {
        result.width = widthSpec.resolve(preferredWidth);
    } else {
        // 如果是容器，测量子元素
        if (isContainer()) {
            result = measureContainer(widthSpec, heightSpec);
            return result;
        }
        // 否则使用约束中的minSize或0
        result.width = widthSpec.resolve(m_constraints.minWidth);
    }

    // 解析高度
    if (preferredHeight >= 0) {
        result.height = heightSpec.resolve(preferredHeight);
    } else {
        if (isContainer()) {
            result = measureContainer(widthSpec, heightSpec);
            return result;
        }
        result.height = heightSpec.resolve(m_constraints.minHeight);
    }

    // 应用宽高比约束
    if (m_constraints.aspectRatio > 0.0f) {
        f32 currentRatio = static_cast<f32>(result.width) / static_cast<f32>(result.height);
        if (currentRatio > m_constraints.aspectRatio) {
            // 太宽，调整高度
            result.height = static_cast<i32>(result.width / m_constraints.aspectRatio);
        } else if (currentRatio < m_constraints.aspectRatio) {
            // 太高，调整宽度
            result.width = static_cast<i32>(result.height * m_constraints.aspectRatio);
        }
    }

    return result;
}

Size WidgetLayoutAdaptor::measureContainer(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec) {
    // 容器的默认测量：遍历子元素并计算总尺寸
    // 具体的布局算法（如Flex、Grid）会覆盖此行为

    auto children = getChildren();
    if (children.empty()) {
        return Size(m_constraints.minWidth, m_constraints.minHeight);
    }

    i32 maxWidth = 0;
    i32 maxHeight = 0;

    // 简单堆叠测量（子类可覆盖实现更复杂的逻辑）
    for (auto* child : children) {
        if (!child->constraints().isLayoutEnabled()) continue;

        Size childSize = child->measure(
            MeasureSpec::MakeUnspecified(),
            MeasureSpec::MakeUnspecified()
        );

        // 加上子元素的边距
        childSize.width += child->constraints().margin.horizontal();
        childSize.height += child->constraints().margin.vertical();

        maxWidth = std::max(maxWidth, childSize.width);
        maxHeight = std::max(maxHeight, childSize.height);
    }

    // 加上内边距
    maxWidth += m_constraints.padding.horizontal();
    maxHeight += m_constraints.padding.vertical();

    // 应用约束
    maxWidth = widthSpec.resolve(maxWidth);
    maxHeight = heightSpec.resolve(maxHeight);

    return Size(maxWidth, maxHeight);
}

void WidgetLayoutAdaptor::applyLayout(const LayoutResult& result) {
    if (!m_widget) return;

    // 应用边界
    m_widget->setBounds(result.bounds);

    // 清除脏标记
    m_layoutDirty = false;
    m_renderDirty = result.needsRepaint;
}

void WidgetLayoutAdaptor::applyLayout(i32 x, i32 y, i32 width, i32 height) {
    applyLayout(LayoutResult(x, y, width, height));
}

void WidgetLayoutAdaptor::requestLayout() {
    if (m_layoutDirty) return;  // 已经是脏的，不需要重复传播

    m_layoutDirty = true;
    m_cacheValid = false;

    // 向上传播
    propagateLayoutRequest();
}

void WidgetLayoutAdaptor::propagateLayoutRequest() {
    if (!m_widget) return;

    auto* parent = m_widget->parent();
    if (parent) {
        // 查找父容器的适配器并请求布局
        // 注意：这里需要一种机制来关联Widget和其适配器
        // 当前简化实现：直接标记父Widget
        // 实际实现中可能需要一个全局映射表或Widget自身持有适配器引用
    }
}

std::vector<WidgetLayoutAdaptor*> WidgetLayoutAdaptor::getChildren() {
    std::vector<WidgetLayoutAdaptor*> result;

    if (!m_widget) return result;

    // 尝试将Widget转换为容器
    auto* container = dynamic_cast<widget::IWidgetContainer*>(m_widget);
    if (!container) return result;

    const auto& widgets = container->widgets();
    result.reserve(widgets.size());

    for (const auto& child : widgets) {
        // 这里需要获取或创建子元素的适配器
        // 当前简化实现：返回空列表
        // 实际实现中需要适配器管理器或Widget持有适配器
    }

    return result;
}

size_t WidgetLayoutAdaptor::childCount() const {
    if (!m_widget) return 0;

    auto* container = dynamic_cast<const widget::IWidgetContainer*>(m_widget);
    if (!container) return 0;

    return container->widgetCount();
}

bool WidgetLayoutAdaptor::isContainer() const {
    if (!m_widget) return false;
    return dynamic_cast<const widget::IWidgetContainer*>(m_widget) != nullptr;
}

// ============================================================================
// ContainerLayoutAdaptor 实现
// ============================================================================

ContainerLayoutAdaptor::ContainerLayoutAdaptor(
    widget::Widget* widget,
    widget::IWidgetContainer* container
) : WidgetLayoutAdaptor(widget)
  , m_container(container) {
}

std::vector<WidgetLayoutAdaptor> ContainerLayoutAdaptor::getChildAdaptors() {
    std::vector<WidgetLayoutAdaptor> result;

    if (!m_container) return result;

    const auto& widgets = m_container->widgets();
    result.reserve(widgets.size());

    for (const auto& child : widgets) {
        result.emplace_back(child.get());
    }

    return result;
}

} // namespace mc::client::ui::kagero::layout
