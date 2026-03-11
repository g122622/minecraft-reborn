#pragma once

#include "../../../core/Types.hpp"

namespace mr::entity::ai::pathfinding {

/**
 * @brief 路径节点类型
 *
 * 定义不同地形节点的可通行性和代价。
 *
 * 参考 MC 1.16.5 PathNodeType
 */
enum class PathNodeType : u8 {
    /// 完全阻塞，无法通行
    Blocked = 0,

    /// 空气，可以跌落通过
    Open = 1,

    /// 可行走的地面
    Walkable = 2,

    /// 可行走的门
    WalkableDoor = 3,

    /// 活板门
    Trapdoor = 4,

    /// 栅栏
    Fence = 5,

    /// 岩浆
    Lava = 6,

    /// 水
    Water = 7,

    /// 火焰危险区域
    DangerFire = 8,

    /// 仙人掌危险区域
    DangerCactus = 9,

    /// 甜浆果丛危险区域
    DangerBerry = 10,

    /// 栅栏门
    FenceGate = 11,

    /// 铁轨
    Rail = 12,

    /// 活板门（可下落）
    TrapdoorDown = 13,

    /// 攀爬（梯子、藤蔓等）
    Climbable = 14,

    /// 跌落危险
    DangerFall = 15,

    /// 其他
    Other = 255
};

/**
 * @brief 获取节点类型的代价惩罚
 * @param type 节点类型
 * @return 代价惩罚值（负值表示更危险）
 */
[[nodiscard]] inline f32 getPathCostPenalty(PathNodeType type) {
    switch (type) {
        case PathNodeType::Blocked:
            return 0.0f; // 完全阻塞，不应该通行
        case PathNodeType::Open:
            return 1.0f;
        case PathNodeType::Walkable:
            return 0.0f; // 无惩罚
        case PathNodeType::WalkableDoor:
            return 0.0f;
        case PathNodeType::Trapdoor:
            return 0.0f;
        case PathNodeType::Fence:
            return 0.0f; // 需要跳跃
        case PathNodeType::Lava:
            return -1.0f; // 极度危险
        case PathNodeType::Water:
            return 0.0f;
        case PathNodeType::DangerFire:
            return -1.0f;
        case PathNodeType::DangerCactus:
            return -1.0f;
        case PathNodeType::DangerBerry:
            return -1.0f;
        case PathNodeType::FenceGate:
            return 0.0f;
        case PathNodeType::Rail:
            return 0.0f;
        case PathNodeType::TrapdoorDown:
            return 0.0f;
        case PathNodeType::Climbable:
            return 0.0f;
        case PathNodeType::DangerFall:
            return -1.0f;
        case PathNodeType::Other:
        default:
            return 0.0f;
    }
}

/**
 * @brief 检查节点是否可通行
 * @param type 节点类型
 * @return 是否可通行
 */
[[nodiscard]] inline bool isWalkable(PathNodeType type) {
    return type == PathNodeType::Walkable ||
           type == PathNodeType::WalkableDoor ||
           type == PathNodeType::Trapdoor ||
           type == PathNodeType::FenceGate ||
           type == PathNodeType::Rail ||
           type == PathNodeType::Water ||
           type == PathNodeType::Climbable;
}

} // namespace mr::entity::ai::pathfinding
