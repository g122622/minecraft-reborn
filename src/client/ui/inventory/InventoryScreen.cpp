#include "InventoryScreen.hpp"
#include "../GuiRenderer.hpp"
#include "../../../common/entity/Player.hpp"
#include "../../../common/entity/inventory/Container.hpp"
#include "../../../common/entity/inventory/PlayerInventory.hpp"
#include "../../../common/item/ItemStack.hpp"
#include "../../../common/item/Item.hpp"

namespace mr {
namespace client {

// ============================================================================
// 构造函数
// ============================================================================

InventoryScreen::InventoryScreen()
{
}

// ============================================================================
// 初始化
// ============================================================================

bool InventoryScreen::initialize() {
    if (m_initialized) {
        return true;
    }

    m_initialized = true;
    return true;
}

// ============================================================================
// 渲染
// ============================================================================

void InventoryScreen::render(GuiRenderer& gui, const Player& player,
                              f32 mouseX, f32 mouseY,
                              f32 screenWidth, f32 screenHeight) {
    if (!m_open || !m_initialized) {
        return;
    }

    // 计算界面位置（居中）
    f32 guiX = (screenWidth - INVENTORY_WIDTH) / 2.0f;
    f32 guiY = (screenHeight - INVENTORY_HEIGHT) / 2.0f;

    // 绘制背景
    gui.fillRect(guiX, guiY, INVENTORY_WIDTH, INVENTORY_HEIGHT,
                 0xF0C0C0C0);  // 半透明灰色背景
    gui.drawRect(guiX, guiY, INVENTORY_WIDTH, INVENTORY_HEIGHT,
                 0xFF373737);   // 边框

    // 绘制标题
    std::string title = "Inventory";
    f32 titleWidth = gui.getTextWidth(title);
    gui.drawText(title, guiX + (INVENTORY_WIDTH - titleWidth) / 2.0f, guiY + 6.0f,
                 0xFF404040, false);

    const PlayerInventory& inventory = player.inventory();

    // 绘制护甲槽位（左上角，从下到上：靴子、护腿、胸甲、头盔）
    for (i32 i = 0; i < 4; ++i) {
        f32 slotX = guiX + ARMOR_X;
        f32 slotY = guiY + ARMOR_Y + (3 - i) * SLOT_SPACING;  // 从下到上
        i32 slotIndex = InventorySlots::ARMOR_FEET + i;  // 36, 37, 38, 39

        const ItemStack& stack = inventory.getItem(slotIndex);
        renderSlot(gui, slotIndex, slotX, slotY, stack, mouseX, mouseY);
    }

    // 绘制主背包槽位（3行9列）
    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = InventorySlots::MAIN_START + row * 9 + col;
            f32 slotX = guiX + MAIN_X + col * SLOT_SPACING;
            f32 slotY = guiY + MAIN_Y + row * SLOT_SPACING;

            const ItemStack& stack = inventory.getItem(slotIndex);
            renderSlot(gui, slotIndex, slotX, slotY, stack, mouseX, mouseY);
        }
    }

    // 绘制快捷栏槽位（底部）
    for (i32 i = 0; i < 9; ++i) {
        f32 slotX = guiX + HOTBAR_X + i * SLOT_SPACING;
        f32 slotY = guiY + HOTBAR_Y;

        const ItemStack& stack = inventory.getItem(i);
        renderSlot(gui, i, slotX, slotY, stack, mouseX, mouseY);

        // 高亮选中的槽位
        if (i == inventory.getSelectedSlot()) {
            gui.drawRect(slotX - 1, slotY - 1, SLOT_SIZE + 2, SLOT_SIZE + 2,
                         0xFFFFFFFF);
        }
    }

    // 绘制副手槽位（左侧）
    {
        f32 slotX = guiX + OFFHAND_X;
        f32 slotY = guiY + OFFHAND_Y;
        const ItemStack& stack = inventory.getOffhandItem();
        renderSlot(gui, InventorySlots::OFFHAND, slotX, slotY, stack, mouseX, mouseY);
    }

    // 绘制鼠标持有的物品
    if (!m_cursorItem.isEmpty()) {
        gui.fillRect(mouseX - 8, mouseY - 8, 16, 16, 0xFFAAAAAA);
        // 绘制数量
        if (m_cursorItem.getCount() > 1) {
            std::string countText = std::to_string(m_cursorItem.getCount());
            f32 textWidth = gui.getTextWidth(countText);
            gui.drawText(countText, mouseX - 8 + 16 - textWidth - 1,
                         mouseY - 8 + 16 - 8, 0xFFFFFFFF, true);
        }
    }

    // 检测鼠标悬停的槽位并显示提示
    i32 hoverSlot = getSlotAtPosition(mouseX - guiX, mouseY - guiY);
    if (hoverSlot >= 0) {
        const ItemStack& stack = inventory.getItem(hoverSlot);
        if (!stack.isEmpty()) {
            renderItemTooltip(gui, stack, mouseX, mouseY);
        }
    }
}

// ============================================================================
// 交互处理
// ============================================================================

bool InventoryScreen::onClick(Container& container, f32 mouseX, f32 mouseY, i32 button) {
    if (!m_open) {
        return false;
    }

    // 计算界面位置
    // 注意：这里需要从render获取实际位置，或保存为成员变量
    // 简化处理：假设居中

    // 获取点击的槽位
    i32 slotIndex = getSlotAtPosition(mouseX, mouseY);
    if (slotIndex < 0) {
        return false;
    }

    // 执行点击操作
    ClickType clickType = (button == 0) ? ClickType::Pick : ClickType::PickAll;
    m_cursorItem = container.clicked(slotIndex, button, clickType, m_cursorItem);

    return true;
}

bool InventoryScreen::onKeyPress(i32 key) {
    if (!m_open) {
        return false;
    }

    // 数字键1-9切换快捷栏选中
    if (key >= 49 && key <= 57) {  // '1'-'9' 对应 ASCII 49-57
        // 切换到对应槽位 (0-8)
        // 由外部处理
        return true;
    }

    // E键关闭背包
    if (key == 69) {  // 'E'
        m_open = false;
        return true;
    }

    return false;
}

bool InventoryScreen::onScroll(f32 delta) {
    if (!m_open) {
        return false;
    }

    // 滚轮在背包界面内滚动可以切换快捷栏
    // 简化：由外部处理
    (void)delta;
    return false;
}

// ============================================================================
// 私有方法
// ============================================================================

void InventoryScreen::renderPlayerModel(GuiRenderer& gui, const Player& player,
                                         f32 x, f32 y, f32 scale) {
    // TODO: 实现玩家模型渲染
    // 目前绘制占位符
    (void)player;
    (void)scale;
    gui.fillRect(x - 16, y - 32, 32, 32, 0xFF808080);
    gui.drawRect(x - 16, y - 32, 32, 32, 0xFF404040);
}

void InventoryScreen::renderSlot(GuiRenderer& gui, i32 slotIndex, f32 x, f32 y,
                                  const ItemStack& stack, f32 mouseX, f32 mouseY) {
    (void)slotIndex;
    (void)mouseX;
    (void)mouseY;

    // 绘制槽位背景
    gui.fillRect(x, y, SLOT_SIZE, SLOT_SIZE, 0xFF8B8B8B);
    gui.drawRect(x, y, SLOT_SIZE, SLOT_SIZE, 0xFF373737);

    // 如果有物品，绘制物品
    if (!stack.isEmpty()) {
        // TODO: 绘制物品图标
        // 目前绘制占位符
        gui.fillRect(x + 1, y + 1, SLOT_SIZE - 2, SLOT_SIZE - 2, 0xFF55FF55);

        // 绘制数量
        if (stack.getCount() > 1) {
            std::string countText = std::to_string(stack.getCount());
            f32 textWidth = gui.getTextWidth(countText);
            gui.drawText(countText,
                        x + SLOT_SIZE - textWidth - 1,
                        y + SLOT_SIZE - 8,
                        0xFFFFFFFF, true);
        }
    }
}

void InventoryScreen::getSlotPosition(i32 slotIndex, f32& x, f32& y) const {
    // 护甲槽（36-39，从下到上：靴子、护腿、胸甲、头盔）
    if (slotIndex >= InventorySlots::ARMOR_START && slotIndex <= InventorySlots::ARMOR_END) {
        i32 armorIndex = slotIndex - InventorySlots::ARMOR_START;
        x = ARMOR_X;
        y = ARMOR_Y + (3 - armorIndex) * SLOT_SPACING;  // 从下到上
        return;
    }

    // 副手槽（40）
    if (slotIndex == InventorySlots::OFFHAND) {
        x = OFFHAND_X;
        y = OFFHAND_Y;
        return;
    }

    // 快捷栏（0-8）
    if (slotIndex >= InventorySlots::HOTBAR_START && slotIndex <= InventorySlots::HOTBAR_END) {
        x = HOTBAR_X + slotIndex * SLOT_SPACING;
        y = HOTBAR_Y;
        return;
    }

    // 主背包（9-35）
    if (slotIndex >= InventorySlots::MAIN_START && slotIndex <= InventorySlots::MAIN_END) {
        i32 mainIndex = slotIndex - InventorySlots::MAIN_START;
        i32 row = mainIndex / 9;
        i32 col = mainIndex % 9;
        x = MAIN_X + col * SLOT_SPACING;
        y = MAIN_Y + row * SLOT_SPACING;
        return;
    }

    // 无效槽位
    x = 0;
    y = 0;
}

i32 InventoryScreen::getSlotAtPosition(f32 mouseX, f32 mouseY) const {
    // 检查护甲槽
    for (i32 i = 0; i < 4; ++i) {
        f32 slotX = ARMOR_X;
        f32 slotY = ARMOR_Y + (3 - i) * SLOT_SPACING;

        if (mouseX >= slotX && mouseX < slotX + SLOT_SIZE &&
            mouseY >= slotY && mouseY < slotY + SLOT_SIZE) {
            return InventorySlots::ARMOR_FEET + i;
        }
    }

    // 检查副手槽
    {
        f32 slotX = OFFHAND_X;
        f32 slotY = OFFHAND_Y;

        if (mouseX >= slotX && mouseX < slotX + SLOT_SIZE &&
            mouseY >= slotY && mouseY < slotY + SLOT_SIZE) {
            return InventorySlots::OFFHAND;
        }
    }

    // 检查主背包
    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            f32 slotX = MAIN_X + col * SLOT_SPACING;
            f32 slotY = MAIN_Y + row * SLOT_SPACING;

            if (mouseX >= slotX && mouseX < slotX + SLOT_SIZE &&
                mouseY >= slotY && mouseY < slotY + SLOT_SIZE) {
                return InventorySlots::MAIN_START + row * 9 + col;
            }
        }
    }

    // 检查快捷栏
    for (i32 i = 0; i < 9; ++i) {
        f32 slotX = HOTBAR_X + i * SLOT_SPACING;
        f32 slotY = HOTBAR_Y;

        if (mouseX >= slotX && mouseX < slotX + SLOT_SIZE &&
            mouseY >= slotY && mouseY < slotY + SLOT_SIZE) {
            return i;
        }
    }

    return -1;
}

void InventoryScreen::renderItemTooltip(GuiRenderer& gui, const ItemStack& stack,
                                         f32 mouseX, f32 mouseY) {
    if (stack.isEmpty()) {
        return;
    }

    // 获取物品名称
    std::string itemName = stack.getDisplayName();

    // 计算提示框尺寸
    constexpr f32 PADDING = 4.0f;
    constexpr f32 OFFSET_Y = 12.0f;
    f32 textWidth = gui.getTextWidth(itemName);
    f32 tooltipWidth = textWidth + PADDING * 2;
    f32 tooltipHeight = gui.getFontHeight() + PADDING * 2;

    // 提示框位置（鼠标右上方）
    f32 tooltipX = mouseX + OFFSET_Y;
    f32 tooltipY = mouseY - tooltipHeight - OFFSET_Y;

    // 绘制背景
    gui.fillRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight,
                 0xF0100010);
    gui.drawRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight,
                 0xFF5000FF);

    // 绘制物品名称
    gui.drawText(itemName,
                 tooltipX + PADDING,
                 tooltipY + PADDING,
                 0xFFFFFFFF, false);
}

} // namespace client
} // namespace mr
