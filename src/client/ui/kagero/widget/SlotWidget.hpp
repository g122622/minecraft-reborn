#pragma once

#include "Widget.hpp"
#include "PaintContext.hpp"
#include "../../../../common/item/ItemStack.hpp"
#include <functional>
#include <string>

namespace mc::client::ui::kagero::widget {

// 前向声明
class ItemRenderer2D;

/**
 * @brief 物品槽组件
 *
 * 显示物品槽的组件，支持物品显示、交互和背景。
 *
 * 参考MC 1.16.5 Slot类实现
 *
 * 使用示例：
 * @code
 * auto slot = std::make_unique<SlotWidget>("slot_0", 10, 10);
 * slot->setItem(itemStack);
 * slot->setOnItemClick([](SlotWidget& s, i32 button) {
 *     // 处理点击
 * });
 * @endcode
 */
class SlotWidget : public Widget {
public:
    /**
     * @brief 槽位点击回调类型
     */
    using OnSlotClickCallback = std::function<void(SlotWidget&, i32)>;

    /**
     * @brief 槽位释放回调类型
     */
    using OnSlotReleaseCallback = std::function<void(SlotWidget&, i32)>;

    /**
     * @brief 默认构造函数
     */
    SlotWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     */
    SlotWidget(String id, i32 x, i32 y)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, 16, 16)); // 默认槽位大小16x16
    }

    /**
     * @brief 构造函数（带尺寸）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param size 尺寸（宽高相等）
     */
    SlotWidget(String id, i32 x, i32 y, i32 size)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, size, size));
    }

    // ==================== 生命周期 ====================

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)ctx;
        (void)partialTick;

        if (!isVisible()) return;

        // 更新悬停状态
        setHovered(isMouseOver(mouseX, mouseY));

        // TODO: 实际渲染逻辑
        // 1. 渲染背景（如果有）
        // 2. 渲染物品
        // 3. 渲染数量
        // 4. 渲染高亮（如果悬停）
    }

    void paint(PaintContext& ctx) override {
        if (!isVisible()) return;
        if (m_showBackground) {
            ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 40, 40, 40));
            ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 100, 100, 100));
        }
        if (isHovered()) {
            ctx.drawBorder(bounds(), 1.0f, m_highlightColor);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        if (m_onClick) {
            m_onClick(*this, button);
        }

        return true;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button) override {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        if (m_onRelease) {
            m_onRelease(*this, button);
        }

        return true;
    }

    // ==================== 物品操作 ====================

    /**
     * @brief 设置物品
     */
    void setItem(const mc::ItemStack& item) {
        m_item = item;
    }

    /**
     * @brief 获取物品
     */
    [[nodiscard]] const mc::ItemStack& item() const { return m_item; }

    /**
     * @brief 获取可变物品引用
     */
    [[nodiscard]] mc::ItemStack& item() { return m_item; }

    /**
     * @brief 检查槽位是否为空
     */
    [[nodiscard]] bool isEmpty() const { return m_item.isEmpty(); }

    /**
     * @brief 清空槽位
     */
    void clearItem() {
        m_item = mc::ItemStack();
    }

    // ==================== 属性设置 ====================

    /**
     * @brief 设置槽位索引
     */
    void setSlotIndex(i32 index) { m_slotIndex = index; }

    /**
     * @brief 获取槽位索引
     */
    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }

    /**
     * @brief 设置背景纹理路径
     */
    void setBackgroundTexture(const String& path) {
        m_backgroundTexture = path;
    }

    /**
     * @brief 获取背景纹理路径
     */
    [[nodiscard]] const String& backgroundTexture() const { return m_backgroundTexture; }

    /**
     * @brief 设置是否显示背景
     */
    void setShowBackground(bool show) { m_showBackground = show; }

    /**
     * @brief 是否显示背景
     */
    [[nodiscard]] bool showBackground() const { return m_showBackground; }

    /**
     * @brief 设置是否可交互
     */
    void setInteractive(bool interactive) { m_interactive = interactive; }

    /**
     * @brief 是否可交互
     */
    [[nodiscard]] bool isInteractive() const { return m_interactive; }

    /**
     * @brief 设置高亮颜色
     */
    void setHighlightColor(u32 color) { m_highlightColor = color; }

    /**
     * @brief 获取高亮颜色
     */
    [[nodiscard]] u32 highlightColor() const { return m_highlightColor; }

    /**
     * @brief 设置是否显示数量
     */
    void setShowCount(bool show) { m_showCount = show; }

    /**
     * @brief 是否显示数量
     */
    [[nodiscard]] bool showCount() const { return m_showCount; }

    // ==================== 回调设置 ====================

    /**
     * @brief 设置点击回调
     */
    void setOnClick(OnSlotClickCallback callback) {
        m_onClick = std::move(callback);
    }

    /**
     * @brief 设置释放回调
     */
    void setOnRelease(OnSlotReleaseCallback callback) {
        m_onRelease = std::move(callback);
    }

protected:
    mc::ItemStack m_item;               ///< 槽位中的物品
    i32 m_slotIndex = -1;               ///< 槽位索引

    // 显示属性
    String m_backgroundTexture;         ///< 背景纹理路径
    bool m_showBackground = true;       ///< 是否显示背景
    bool m_interactive = true;          ///< 是否可交互
    bool m_showCount = true;            ///< 是否显示数量
    u32 m_highlightColor = Colors::fromARGB(128, 255, 255, 255); ///< 高亮颜色

    // 回调
    OnSlotClickCallback m_onClick;      ///< 点击回调
    OnSlotReleaseCallback m_onRelease;  ///< 释放回调
};

} // namespace mc::client::ui::kagero::widget
