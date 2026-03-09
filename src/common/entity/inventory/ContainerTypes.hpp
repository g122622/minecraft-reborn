#pragma once

#include "../../core/Types.hpp"

namespace mr {

/**
 * @brief 容器ID类型
 *
 * 用于标识不同类型的容器窗口。
 */
using ContainerId = u8;

/**
 * @brief 容器类型枚举
 */
enum class ContainerType : u8 {
    Player = 0,         // 玩家背包
    Chest = 1,          // 箱子
    CraftingTable = 2,  // 工作台
    Furnace = 3,        // 熔炉
    Dispenser = 4,      // 发射器/投掷器
    Enchantment = 5,    // 附魔台
    Anvil = 6,          // 铁砧
    BrewingStand = 7,   // 酿造台
    Villager = 8,       // 村民交易
    Beacon = 9,         // 信标
    Hopper = 10,        // 漏斗
    ShulkerBox = 11     // 潜影盒
};

/**
 * @brief 容器点击类型
 */
enum class ClickType : u8 {
    Pick = 0,           // 左键拾取
    PickAll = 1,        // 右键拾取全部
    Throw = 3,          // 丢弃物品
    ThrowAll = 4,       // 丢弃全部
    Pickup = 5,         // 拖拽拾取
    QuickMove = 6,      // Shift+点击快速移动
    Clone = 7,          // 创造模式复制
    Spread = 8,         // 均匀分布
    Swap = 9            // 数字键交换
};

/**
 * @brief 点击操作类型（网络包中使用）
 */
enum class ClickAction : u8 {
    Pick = 0,           // 左键拾取
    PickAll = 1,        // 右键拾取全部
    Throw = 3,          // 丢弃物品
    ThrowAll = 4,       // 丢弃全部
    Pickup = 5,         // 拖拽拾取
    QuickMove = 6,      // Shift+点击快速移动
    Clone = 7,          // 创造模式复制
    Spread = 8,         // 均匀分布
    Swap = 9            // 数字键交换
};

} // namespace mr
