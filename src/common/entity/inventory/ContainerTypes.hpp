#pragma once

#include "../../core/Types.hpp"

namespace mc {

/**
 * @brief 容器ID类型
 *
 * 用于标识打开的容器窗口，同步客户端和服务端状态。
 * 内部使用 i32，网络传输使用 u8。
 */
using ContainerId = i32;

/**
 * @brief 无效容器ID
 */
constexpr ContainerId INVALID_CONTAINER_ID = -1;

/**
 * @brief 容器ID的网络传输类型
 *
 * 网络包中使用 u8 传输容器ID。
 */
using ContainerIdU8 = u8;

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
 * @brief 点击操作类型（网络包中使用）
 *
 * 对应 MC 协议中的点击模式
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

/**
 * @brief 容器动作类型
 *
 * 定义玩家与容器交互的操作类型
 */
enum class ContainerAction : u8 {
    Click,           ///< 点击槽位
    ShiftClick,      ///< Shift+点击（快速移动）
    HotbarSwap,      ///< 数字键交换
    CreativePick,    ///< 创造模式选取
    DoubleClick,     ///< 双击（合并相同物品）
    Drag,            ///< 拖动分发
    Throw,           ///< 丢弃物品
};

/**
 * @brief 点击类型
 *
 * 定义容器菜单中的点击操作类型
 */
enum class ClickType : u8 {
    Pick,            ///< 拾取（左键）
    PickAll,         ///< 全部拾取（双击）
    PickSome,        ///< 部分拾取（右键）
    Place,           ///< 放置（左键）
    PlaceSome,       ///< 部分放置（右键）
    PlaceAll,        ///< 全部放置
    Throw,           ///< 丢弃
    ThrowAll,        ///< 全部丢弃
    QuickMove,       ///< 快速移动（Shift+点击）
    QuickCraft,      ///< 快速合成（拖动）
    Clone,           ///< 克隆（创造模式中键）
    Pickup,          ///< 拖拽拾取
    Swap             ///< 数字键交换
};

} // namespace mc
