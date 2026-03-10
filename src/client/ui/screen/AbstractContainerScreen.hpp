#pragma once

#include "screen/IScreen.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "core/Types.hpp"
#include <memory>

namespace mr::client {

/**
 * @brief 容器屏幕基类
 *
 * 管理容器菜单的客户端屏幕，处理槽位渲染和交互。
 * 与服务端的AbstractContainerMenu配对使用。
 *
 * 槽位渲染：
 * - 槽位背景
 * - 槽位物品
 * - 槽位高亮
 * - 鼠标悬停提示
 *
 * 使用示例：
 * @code
 * class CraftingScreen : public AbstractContainerScreen<CraftingMenu> {
 * public:
 *     void render(i32 mouseX, i32 mouseY, f32 partialTick) override;
 *     bool onClick(i32 mouseX, i32 mouseY, i32 button) override;
 * };
 * @endcode
 *
 * @tparam Menu 菜单类型
 */
template<typename Menu>
class AbstractContainerScreen : public IScreen {
public:
    /**
     * @brief 构造函数
     * @param menu 菜单实例
     */
    explicit AbstractContainerScreen(std::unique_ptr<Menu> menu)
        : m_menu(std::move(menu))
        , m_leftPos(0)
        , m_topPos(0)
        , m_imageWidth(176)
        , m_imageHeight(166)
        , m_initialized(false) {
    }

    /**
     * @brief 析构函数
     */
    ~AbstractContainerScreen() override = default;

    // 禁止拷贝
    AbstractContainerScreen(const AbstractContainerScreen&) = delete;
    AbstractContainerScreen& operator=(const AbstractContainerScreen&) = delete;

    // 允许移动
    AbstractContainerScreen(AbstractContainerScreen&&) noexcept = default;
    AbstractContainerScreen& operator=(AbstractContainerScreen&&) noexcept = default;

    /**
     * @brief 初始化屏幕
     */
    void init() override {
        if (!m_initialized) {
            m_initialized = true;
            onInit();
        }
    }

    /**
     * @brief 渲染屏幕
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param partialTick 部分tick时间
     */
    void render(i32 mouseX, i32 mouseY, f32 partialTick) override {
        // 渲染背景
        if (shouldRenderBackground()) {
            renderBackground();
        }

        // 渲染容器GUI
        renderContainerBackground();
        renderSlots();
        renderContainerForeground(mouseX, mouseY);

        // 渲染鼠标持有的物品
        renderCarriedItem(mouseX, mouseY);

        // 渲染悬停提示
        renderTooltip(mouseX, mouseY);
    }

    /**
     * @brief 处理鼠标点击
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        if (m_menu == nullptr) {
            return false;
        }

        // 查找点击的槽位
        mr::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot != nullptr) {
            return onSlotClick(*slot, slot->getIndex(), button);
        }

        // 点击空白区域
        return onClickOutside(mouseX, mouseY, button);
    }

    /**
     * @brief 处理键盘按键
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override {
        // ESC关闭屏幕
        if (key == 256 && action == 1) { // GLFW_KEY_ESCAPE
            onClose();
            return true;
        }

        // E键关闭容器屏幕
        if (key == 69 && action == 1) { // GLFW_KEY_E
            onClose();
            return true;
        }

        return false;
    }

    /**
     * @brief 屏幕关闭
     */
    void onClose() override {
        // 子类可重写以保存状态
    }

    /**
     * @brief 容器屏幕不暂停游戏
     */
    [[nodiscard]] bool isPauseScreen() const override {
        return false;
    }

    /**
     * @brief 获取菜单
     */
    [[nodiscard]] Menu* getMenu() { return m_menu.get(); }
    [[nodiscard]] const Menu* getMenu() const { return m_menu.get(); }

    /**
     * @brief 获取鼠标持有的物品
     */
    [[nodiscard]] mr::ItemStack& getCarriedItem() {
        return m_menu ? m_menu->getCarriedItem() : m_emptyStack;
    }
    [[nodiscard]] const mr::ItemStack& getCarriedItem() const {
        return m_menu ? m_menu->getCarriedItem() : m_emptyStack;
    }

    /**
     * @brief 获取GUI左边界
     */
    [[nodiscard]] i32 getLeftPos() const { return m_leftPos; }

    /**
     * @brief 获取GUI上边界
     */
    [[nodiscard]] i32 getTopPos() const { return m_topPos; }

    /**
     * @brief 获取GUI宽度
     */
    [[nodiscard]] i32 getImageWidth() const { return m_imageWidth; }

    /**
     * @brief 获取GUI高度
     */
    [[nodiscard]] i32 getImageHeight() const { return m_imageHeight; }

protected:
    /**
     * @brief 子类初始化回调
     */
    virtual void onInit() {}

    /**
     * @brief 设置GUI尺寸
     * @param width 宽度
     * @param height 高度
     */
    void setImageSize(i32 width, i32 height) {
        m_imageWidth = width;
        m_imageHeight = height;
        updatePosition();
    }

    /**
     * @brief 更新GUI位置（居中）
     */
    void updatePosition() {
        // TODO: 获取屏幕尺寸并居中
        // 暂时使用固定值
        m_leftPos = 0;
        m_topPos = 0;
    }

    /**
     * @brief 渲染背景暗化
     */
    virtual void renderBackground() {
        // TODO: 渲染半透明黑色背景
    }

    /**
     * @brief 渲染容器背景
     */
    virtual void renderContainerBackground() {
        // TODO: 渲染容器GUI纹理
    }

    /**
     * @brief 渲染所有槽位
     */
    virtual void renderSlots() {
        // TODO: 遍历槽位并渲染
    }

    /**
     * @brief 渲染容器前景（标题等）
     */
    virtual void renderContainerForeground(i32 mouseX, i32 mouseY) {
        (void)mouseX;
        (void)mouseY;
        // 子类可重写以渲染标题
    }

    /**
     * @brief 渲染鼠标持有的物品
     */
    virtual void renderCarriedItem(i32 mouseX, i32 mouseY) {
        const auto& carried = getCarriedItem();
        if (!carried.isEmpty()) {
            // TODO: 渲染鼠标跟随的物品
            (void)mouseX;
            (void)mouseY;
        }
    }

    /**
     * @brief 渲染悬停提示
     */
    virtual void renderTooltip(i32 mouseX, i32 mouseY) {
        mr::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot != nullptr && !slot->getItem().isEmpty()) {
            // TODO: 渲染物品提示
        }
    }

    /**
     * @brief 获取指定位置的槽位
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @return 槽位指针，如果无效返回nullptr
     */
    [[nodiscard]] mr::Slot* getSlotAt(i32 mouseX, i32 mouseY) {
        if (m_menu == nullptr) {
            return nullptr;
        }

        // 遍历所有槽位查找
        for (i32 i = 0; i < m_menu->getSlotCount(); ++i) {
            mr::Slot* slot = m_menu->getSlot(i);
            if (slot != nullptr && isMouseOverSlot(*slot, mouseX, mouseY)) {
                return slot;
            }
        }
        return nullptr;
    }

    /**
     * @brief 检查鼠标是否在槽位上
     */
    [[nodiscard]] virtual bool isMouseOverSlot(const mr::Slot& slot, i32 mouseX, i32 mouseY) const {
        i32 slotX = m_leftPos + slot.getX();
        i32 slotY = m_topPos + slot.getY();
        return mouseX >= slotX && mouseX < slotX + 16 &&
               mouseY >= slotY && mouseY < slotY + 16;
    }

    /**
     * @brief 槽位点击处理
     */
    virtual bool onSlotClick(mr::Slot& slot, i32 slotIndex, i32 button) {
        if (m_menu == nullptr) {
            return false;
        }

        // 转换为点击类型
        mr::ClickType clickType = (button == 0) ? mr::ClickType::Pick : mr::ClickType::PlaceSome;

        // 调用菜单处理
        // TODO: 需要玩家引用
        // m_menu->clicked(slotIndex, button, clickType, player);
        (void)slot;
        (void)slotIndex;
        (void)button;
        return true;
    }

    /**
     * @brief 点击空白区域处理
     */
    virtual bool onClickOutside(i32 mouseX, i32 mouseY, i32 button) {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        return false;
    }

    // 成员变量
    std::unique_ptr<Menu> m_menu;
    i32 m_leftPos;          ///< GUI左边界（居中后的位置）
    i32 m_topPos;           ///< GUI上边界
    i32 m_imageWidth;       ///< GUI纹理宽度
    i32 m_imageHeight;      ///< GUI纹理高度
    bool m_initialized;     ///< 是否已初始化

    // 空物品堆（用于空菜单时返回）
    static mr::ItemStack m_emptyStack;
};

// 静态成员定义
template<typename Menu>
mr::ItemStack AbstractContainerScreen<Menu>::m_emptyStack;

} // namespace mr::client
