#pragma once

#include "../util/AxisAlignedBB.hpp"
#include "../math/Vector3.hpp"
#include "../world/BlockID.hpp"
#include "../world/chunk/ChunkData.hpp"
#include "collision/BlockCollision.hpp"
#include <vector>

namespace mr {

/**
 * @brief 碰撞世界接口
 *
 * 提供世界碰撞查询的抽象接口。
 * ClientWorld和ServerWorld需要实现此接口以支持物理引擎。
 *
 * 注意：所有坐标参数都是方块坐标（整数）
 */
class ICollisionWorld {
public:
    virtual ~ICollisionWorld() = default;

    /**
     * @brief 获取指定位置的方块状态
     * @param x, y, z 方块坐标
     * @return 方块状态（如果超出世界范围，返回空气）
     */
    [[nodiscard]] virtual BlockState getBlock(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 检查位置是否在世界范围内
     * @param x, y, z 方块坐标
     * @return 是否在有效范围内
     */
    [[nodiscard]] virtual bool isWithinWorldBounds(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取区块数据
     * @param x, z 区块坐标
     * @return 区块数据指针（如果未加载返回nullptr）
     */
    [[nodiscard]] virtual const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const = 0;

    /**
     * @brief 获取世界最小Y坐标
     */
    [[nodiscard]] virtual i32 getMinBuildHeight() const { return 0; }

    /**
     * @brief 获取世界最大Y坐标
     */
    [[nodiscard]] virtual i32 getMaxBuildHeight() const { return 256; }
};

/**
 * @brief 物理引擎
 *
 * 实现Minecraft兼容的物理系统，包括：
 * - 逐轴碰撞检测和解决
 * - 重力和跳跃
 * - 自动步进（stairs/slabs）
 * - 地面检测
 *
 * 使用方法：
 * 1. 创建PhysicsEngine实例，传入ICollisionWorld实现
 * 2. 调用moveEntity()处理带碰撞的移动
 * 3. 使用isOnGround()检测是否在地面
 *
 * 物理常量（来自Minecraft）：
 * - GRAVITY = 0.08 blocks/tick²
 * - JUMP_VELOCITY = 0.42 blocks/tick
 * - DRAG = 0.98（空气阻力）
 * - STEP_HEIGHT = 0.6（玩家步进高度）
 */
class PhysicsEngine {
public:
    // MC物理常量
    static constexpr f32 GRAVITY = 0.08f;            // blocks/tick²
    static constexpr f32 JUMP_VELOCITY = 0.42f;     // blocks/tick
    static constexpr f32 DRAG = 0.98f;               // 空气阻力
    static constexpr f32 STEP_HEIGHT = 0.6f;         // 最大步进高度
    static constexpr f32 PLAYER_WIDTH = 0.6f;        // 玩家宽度
    static constexpr f32 PLAYER_HEIGHT = 1.8f;       // 玩家高度

    explicit PhysicsEngine(ICollisionWorld& world);

    /**
     * @brief 带碰撞检测的实体移动
     *
     * 算法（来自MC Entity.move）：
     * 1. 收集实体AABB扩展范围内的所有方块碰撞箱
     * 2. Y轴优先处理（重力/跳跃）
     * 3. X/Z按移动幅度排序处理
     * 4. 尝试step-up（如果水平碰撞且在地面）
     * 5. 更新onGround状态
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param stepHeight 步进高度（玩家0.6，其他实体可能不同）
     * @return 实际移动向量（碰撞后）
     */
    Vector3 moveEntity(AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight = 0.0f);

    /**
     * @brief 检测实体是否在地面
     * @param entityBox 实体碰撞箱
     * @return 是否在地面（下方有支撑）
     */
    [[nodiscard]] bool isOnGround(const AxisAlignedBB& entityBox) const;

    /**
     * @brief 收集范围内的碰撞箱
     * @param searchBox 搜索范围
     * @param boxes 输出的碰撞箱列表
     */
    void collectCollisionBoxes(const AxisAlignedBB& searchBox,
                               std::vector<AxisAlignedBB>& boxes) const;

    /**
     * @brief 设置碰撞世界
     */
    void setWorld(ICollisionWorld& world) { m_world = &world; }

    /**
     * @brief 获取碰撞世界
     */
    [[nodiscard]] ICollisionWorld* getWorld() const { return m_world; }

private:
    /**
     * @brief 核心碰撞解决（MC的collideBoundingBox）
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param boxes 碰撞箱列表
     * @return 实际移动向量
     */
    Vector3 resolveCollision(AxisAlignedBB& entityBox,
                             const Vector3& movement,
                             const std::vector<AxisAlignedBB>& boxes);

    /**
     * @brief 尝试步进
     *
     * 当水平方向移动受阻时，尝试抬起实体继续移动。
     *
     * @param entityBox 实体碰撞箱（会被修改）
     * @param movement 期望移动向量
     * @param resolvedMovement 碰撞后的移动向量
     * @param boxes 碰撞箱列表
     * @param stepHeight 步进高度
     * @return 实际移动向量
     */
    Vector3 attemptStepUp(AxisAlignedBB& entityBox,
                          const Vector3& movement,
                          const Vector3& resolvedMovement,
                          const std::vector<AxisAlignedBB>& boxes,
                          f32 stepHeight);

    /**
     * @brief 获取方块碰撞箱
     * @param x, y, z 方块坐标
     * @param boxes 输出的碰撞箱列表
     */
    void getBlockCollisionBoxes(i32 x, i32 y, i32 z,
                                std::vector<AxisAlignedBB>& boxes) const;

    ICollisionWorld* m_world;
};

} // namespace mr
