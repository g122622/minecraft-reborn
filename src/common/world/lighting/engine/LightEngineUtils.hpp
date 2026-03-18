#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/Direction.hpp"
#include "../../block/Block.hpp"
#include "../../block/BlockPos.hpp"
#include <climits>

namespace mc {

// 前向声明
class IWorld;
class IChunk;
class BlockState;
class CollisionShape;

/**
 * @brief 光照引擎工具类
 *
 * 提供光照引擎共享的工具方法。
 */
class LightEngineUtils {
public:
    // ========================================================================
    // 常量
    // ========================================================================

    /** 根节点位置标记（用于光源） */
    static constexpr i64 ROOT_POS = LONG_MAX;

    /** 所有6个方向（用于遍历相邻方块） */
    static constexpr Direction ALL_DIRECTIONS[6] = {
        Direction::Down, Direction::Up, Direction::North,
        Direction::South, Direction::West, Direction::East
    };

    /** 水平4个方向 */
    static constexpr Direction HORIZONTAL_DIRECTIONS[4] = {
        Direction::North, Direction::South, Direction::West, Direction::East
    };

    // ========================================================================
    // 位置编码
    // ========================================================================

    /**
     * @brief 世界位置编码
     *
     * 编码格式: X(26位) | Z(26位) | Y(12位)
     * 参考 MC 1.16.5 BlockPos.pack()
     * 支持 X/Z: ±30,000,000 (约 ±2^25)
     * 支持 Y: -2048 到 +2047
     */
    [[nodiscard]] static constexpr i64 packPos(i32 x, i32 y, i32 z) {
        // 26位掩码
        constexpr i64 XZ_MASK = (1LL << 26) - 1;
        constexpr i64 Y_MASK = (1LL << 12) - 1;
        // 位移量
        constexpr i32 Y_BITS = 12;
        constexpr i32 Z_OFFSET = Y_BITS;           // 12
        constexpr i32 X_OFFSET = Y_BITS + 26;       // 38

        return ((static_cast<i64>(x) & XZ_MASK) << X_OFFSET) |
               ((static_cast<i64>(y) & Y_MASK)) |
               ((static_cast<i64>(z) & XZ_MASK) << Z_OFFSET);
    }

    /**
     * @brief 从BlockPos编码位置
     */
    [[nodiscard]] static constexpr i64 packPos(const BlockPos& pos) {
        return packPos(pos.x, pos.y, pos.z);
    }

    /**
     * @brief 世界位置解码
     *
     * 参考 MC 1.16.5 BlockPos.unpackX/Y/Z()
     */
    [[nodiscard]] static constexpr void unpackPos(i64 packed, i32& x, i32& y, i32& z) {
        // 26位掩码
        constexpr i64 XZ_MASK = (1LL << 26) - 1;
        constexpr i64 Y_MASK = (1LL << 12) - 1;
        constexpr i32 Y_BITS = 12;

        // 解码并自动符号扩展（使用算术右移）
        // X: 取高26位，通过左移0位后右移38位实现符号扩展
        x = static_cast<i32>(packed >> (Y_BITS + 26));
        // Y: 取低12位，通过左移52位后右移52位实现符号扩展
        y = static_cast<i32>((packed << (64 - Y_BITS)) >> (64 - Y_BITS));
        // Z: 取中间26位，需要提取后符号扩展
        z = static_cast<i32>((packed >> Y_BITS) & XZ_MASK);
        // Z需要手动符号扩展（26位到32位）
        z = (z << 6) >> 6;
    }

    /**
     * @brief 位置偏移
     */
    [[nodiscard]] static i64 offsetPos(i64 pos, Direction dir) {
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
     * @brief 从编码位置提取NibbleArray索引
     *
     * @param packed 编码位置
     * @param x 输出：局部X坐标 (0-15)
     * @param localY 输出：局部Y坐标 (0-15)
     * @param z 输出：局部Z坐标 (0-15)
     */
    [[nodiscard]] static constexpr void extractNibbleIndices(i64 packed, i32& x, i32& localY, i32& z) {
        // X在高位，偏移38位；Z在中间，偏移12位；Y在低位
        x = static_cast<i32>((packed >> 38) & 0xF);
        i32 y = static_cast<i32>(packed & 0xFFF);
        localY = y & 0xF;
        z = static_cast<i32>((packed >> 12) & 0xF);
    }

    // ========================================================================
    // 方块查询
    // ========================================================================

    /**
     * @brief 从区块获取方块及其透明度
     *
     * @param chunk 区块指针
     * @param worldPos 编码的世界位置
     * @param opacityOut 透明度输出（可选）
     * @return 方块状态指针，如果是空气返回nullptr
     */
    [[nodiscard]] static const BlockState* getBlockAndOpacity(
        const IChunk* chunk,
        i64 worldPos,
        i32* opacityOut);

    /**
     * @brief 获取方块的遮挡形状
     *
     * @param state 方块状态
     * @return 遮挡形状，如果是非固体方块返回空形状
     */
    [[nodiscard]] static const CollisionShape& getVoxelShape(const BlockState& state);

    // ========================================================================
    // 遮挡检测
    // ========================================================================

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
