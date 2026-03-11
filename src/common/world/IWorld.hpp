#pragma once

#include "../core/Types.hpp"
#include "../math/Vector3.hpp"
#include "../util/AxisAlignedBB.hpp"
#include <vector>

namespace mr {

// 前向声明
class Entity;
class BlockState;
class ChunkData;

/**
 * @brief 世界访问接口
 *
 * 为实体提供世界访问的抽象接口。
 * ServerWorld 和 ClientWorld 将实现此接口。
 *
 * 参考 MC 1.16.5 IWorldReader / World
 */
class IWorld {
public:
    virtual ~IWorld() = default;

    // ========== 方块访问 ==========

    /**
     * @brief 获取方块状态
     * @param x, y, z 方块坐标
     * @return 方块状态指针，如果超出范围返回空气
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 设置方块状态
     * @param x, y, z 方块坐标
     * @param state 方块状态
     * @return 是否成功
     */
    virtual bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) = 0;

    // ========== 区块访问 ==========

    /**
     * @brief 获取区块
     * @param x, z 区块坐标
     * @return 区块数据指针，如果未加载返回 nullptr
     */
    [[nodiscard]] virtual const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 检查区块是否存在
     */
    [[nodiscard]] virtual bool hasChunk(ChunkCoord x, ChunkCoord z) const = 0;

    // ========== 高度查询 ==========

    /**
     * @brief 获取最高可站立方块高度
     * @param x, z 水平坐标
     * @return 最高方块 Y 坐标
     */
    [[nodiscard]] virtual i32 getHeight(i32 x, i32 z) const = 0;

    // ========== 光照查询 ==========

    /**
     * @brief 获取方块光照
     * @param x, y, z 方块坐标
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getBlockLight(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取天空光照
     * @param x, y, z 方块坐标
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] virtual u8 getSkyLight(i32 x, i32 y, i32 z) const = 0;

    // ========== 碰撞检测 ==========

    /**
     * @brief 检查碰撞箱是否与方块碰撞
     * @param box 碰撞箱
     * @return 是否碰撞
     */
    [[nodiscard]] virtual bool hasBlockCollision(const AxisAlignedBB& box) const = 0;

    /**
     * @brief 获取碰撞箱内的所有方块碰撞箱
     * @param box 碰撞箱
     * @return 碰撞箱列表
     */
    [[nodiscard]] virtual std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const = 0;

    // ========== 实体查询 ==========

    /**
     * @brief 获取碰撞箱内的所有实体
     * @param box 碰撞箱
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box,
        const Entity* except = nullptr) const = 0;

    /**
     * @brief 获取范围内的所有实体
     * @param pos 中心位置
     * @param range 范围
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] virtual std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos,
        f32 range,
        const Entity* except = nullptr) const = 0;

    // ========== 维度信息 ==========

    /**
     * @brief 获取维度 ID
     */
    [[nodiscard]] virtual DimensionId dimension() const = 0;

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] virtual u64 seed() const = 0;

    // ========== 时间 ==========

    /**
     * @brief 获取当前 tick
     */
    [[nodiscard]] virtual u64 currentTick() const = 0;

    /**
     * @brief 获取一天内的时间 (0-23999)
     */
    [[nodiscard]] virtual i64 dayTime() const = 0;

    // ========== 难度 ==========

    /**
     * @brief 是否困难模式
     */
    [[nodiscard]] virtual bool isHardcore() const = 0;

    /**
     * @brief 获取难度
     */
    [[nodiscard]] virtual i32 difficulty() const = 0;

protected:
    IWorld() = default;
};

} // namespace mr
