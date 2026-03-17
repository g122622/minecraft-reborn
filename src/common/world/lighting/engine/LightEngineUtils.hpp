#pragma once

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include "../block/Block.hpp"
#include "../chunk/BlockPos.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockState;
class CollisionShape;

/**
 * @brief 光照引擎工具类
 *
 * 提供光照引擎共享的工具方法。
 */
class LightEngineUtils {
public:
    /**
     * @brief 世界位置编码
     *
     * 编码格式: X(28位) | Z(28位) | Y(12位)
     * 支持 X/Z: -134,217,728 到 +134,217,727
     * 支持 Y: -2048 到 +2047
     */
    [[nodiscard]] static constexpr i64 packPos(i32 x, i32 y, i32 z) {
        u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFLL);
        u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFLL);
        u64 uy = static_cast<u64>(y) & 0xFFF;
        return (ux << 38) | (uz << 12) | uy;
    }

    /**
     * @brief 从BlockPos编码位置
     */
    [[nodiscard]] static constexpr i64 packPos(const BlockPos& pos) {
        return packPos(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 世界位置解码
     */
    static void unpackPos(i64 packed, i32& x, i32& y, i32& z) {
        x = static_cast<i32>((packed >> 38) & 0xFFFFFFF);
        y = static_cast<i32>(packed & 0xFFF);
        z = static_cast<i32>((packed >> 12) & 0xFFFFFFF);

        // 符号扩展
        x = (x << 4) >> 4;
        z = (z << 4) >> 4;
    }

    /**
     * @brief 位置偏移
     */
    [[nodiscard]] static constexpr i64 offsetPos(i64 pos, Direction dir) {
        i32 x, y, z;
        unpackPos(pos, x, y, z);

        switch (dir) {
            case Direction::Down:    --y; break;
            case Direction::Up:      ++y; break;
            case Direction::North:   --z; break;
            case Direction::South:   ++z; break;
            case Direction::West:    --x; break;
            case Direction::East:    ++x; break;
            default: break;
        }

        return packPos(x, y, z);
    }

    /**
     * @brief 世界位置转区块段位置
     */
    [[nodiscard]] static i64 worldToSectionPos(i64 worldPos);

    /**
     * @brief 检查两个方块之间是否有完整的遮挡面
     *
     * 用于判断光线是否可以通过两个相邻方块的接触面。
     * 如果两个方块都是完整的固体方块，且有完全遮挡的面，则返回true。
     *
     * @param world 世界
     * @param stateA 第一个方块的状态
     * @param posA 第一个方块的位置
     * @param stateB 第二个方块的状态
     * @param posB 第二个方块的位置
     * @param dir 从A到B的方向
     * @param opacityA 方块A的光照透明度
     * @return 如果有完整遮挡面返回true
     */
    [[nodiscard]] static bool facesHaveOcclusion(
        IWorld* world,
        const BlockState& stateA, const BlockPos& posA,
        const BlockState& stateB, const BlockPos& posB,
        Direction dir,
        i32 opacityA);

    /**
     * @brief 检查方块是否阻挡特定方向的光线
     *
     * @param state 方块状态
     * @param dir 光线传播方向
     * @return 如果方块在该方向完全阻挡光线返回true
     */
    [[nodiscard]] static bool blocksLightInDirection(
        const BlockState& state,
        Direction dir);

private:
    /**
     * @brief 检查碰撞形状是否在指定方向完全遮挡
     */
    [[nodiscard]] static bool shapeFullyOccludesFace(
        const CollisionShape& shape,
        Direction face);
};

} // namespace mc
