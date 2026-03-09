#pragma once

#include "../core/Types.hpp"
#include "../core/Result.hpp"
#include "../math/Vector3.hpp"
#include "../util/AxisAlignedBB.hpp"
#include <string>
#include <memory>
#include <array>

namespace mr {

// 前向声明
class PhysicsEngine;

// ============================================================================
// 实体类型枚举
// ============================================================================

enum class EntityType : u32 {
    Unknown = 0,
    Player = 1,
    Item = 2,           // 物品实体
    // 后续添加: Mob, Projectile 等
};

// ============================================================================
// 实体姿态
// ============================================================================

enum class EntityPose : u8 {
    Standing = 0,
    FallFlying = 1,     // 使用鞘翅飞行
    Sleeping = 2,
    Swimming = 3,
    SpinAttack = 4,     // 三叉戟激流攻击
    Crouching = 5,      // 潜行
    Dying = 6
};

// ============================================================================
// 实体标志位
// ============================================================================

enum class EntityFlags : u8 {
    None = 0,
    OnFire = 1 << 0,
    Crouching = 1 << 1,
    Sprinting = 1 << 3,
    Swimming = 1 << 4,
    Invisible = 1 << 5,
    Glowing = 1 << 6,
    FallFlying = 1 << 7
};

inline EntityFlags operator|(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline EntityFlags operator&(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasFlag(EntityFlags flags, EntityFlags flag) {
    return (static_cast<u8>(flags) & static_cast<u8>(flag)) != 0;
}

// ============================================================================
// 实体基类
// ============================================================================

/**
 * @brief 实体基类
 *
 * 所有游戏实体（玩家、生物、物品等）的基类。
 * 提供位置、速度、旋转等基本属性，以及碰撞检测和物理模拟支持。
 *
 * 注意：
 * - 位置使用 Vector3 存储，但组件是 f64 精度
 * - 碰撞箱使用 AxisAlignedBB（f32 精度）
 * - 子类应重写 width()、height()、eyeHeight() 方法
 */
class Entity {
public:
    Entity(EntityType type, EntityId id);
    virtual ~Entity() = default;

    // 禁止拷贝
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // 允许移动
    Entity(Entity&&) = default;
    Entity& operator=(Entity&&) = default;

    // ========== 基本属性 ==========

    [[nodiscard]] EntityId id() const { return m_id; }
    [[nodiscard]] EntityType type() const { return m_type; }
    [[nodiscard]] const String& uuid() const { return m_uuid; }
    void setUuid(const String& uuid) { m_uuid = uuid; }

    // ========== 位置 ==========

    [[nodiscard]] Vector3 position() const { return m_position; }
    [[nodiscard]] f32 x() const { return m_position.x; }
    [[nodiscard]] f32 y() const { return m_position.y; }
    [[nodiscard]] f32 z() const { return m_position.z; }

    // 前一帧位置（用于插值）
    [[nodiscard]] Vector3 prevPosition() const { return m_prevPosition; }
    [[nodiscard]] f32 prevX() const { return m_prevPosition.x; }
    [[nodiscard]] f32 prevY() const { return m_prevPosition.y; }
    [[nodiscard]] f32 prevZ() const { return m_prevPosition.z; }

    // ========== 旋转 ==========

    [[nodiscard]] f32 yaw() const { return m_yaw; }
    [[nodiscard]] f32 pitch() const { return m_pitch; }
    [[nodiscard]] f32 prevYaw() const { return m_prevYaw; }
    [[nodiscard]] f32 prevPitch() const { return m_prevPitch; }

    // ========== 速度 ==========

    [[nodiscard]] Vector3 velocity() const { return m_velocity; }
    [[nodiscard]] f32 velocityX() const { return m_velocity.x; }
    [[nodiscard]] f32 velocityY() const { return m_velocity.y; }
    [[nodiscard]] f32 velocityZ() const { return m_velocity.z; }

    // ========== 状态 ==========

    [[nodiscard]] bool onGround() const { return m_onGround; }
    [[nodiscard]] bool isRemoved() const { return m_removed; }
    [[nodiscard]] EntityPose pose() const { return m_pose; }
    [[nodiscard]] EntityFlags flags() const { return m_flags; }

    // ========== 设置属性 ==========

    void setPosition(f32 x, f32 y, f32 z);
    void setPosition(const Vector3& pos) { setPosition(pos.x, pos.y, pos.z); }
    void setRotation(f32 yaw, f32 pitch);
    void setVelocity(f32 x, f32 y, f32 z);
    void setVelocity(const Vector3& vel) { setVelocity(vel.x, vel.y, vel.z); }
    void setOnGround(bool onGround) { m_onGround = onGround; }
    void setPose(EntityPose pose) { m_pose = pose; }
    void setFlags(EntityFlags flags) { m_flags = flags; }

    // 标志操作
    void addFlag(EntityFlags flag) { m_flags = m_flags | flag; }
    void removeFlag(EntityFlags flag) {
        m_flags = static_cast<EntityFlags>(static_cast<u8>(m_flags) & ~static_cast<u8>(flag));
    }
    [[nodiscard]] bool hasFlag(EntityFlags flag) const {
        return mr::hasFlag(m_flags, flag);
    }

    // ========== 尺寸 ==========

    /**
     * @brief 获取实体宽度
     * @return 实体宽度（方块单位）
     */
    [[nodiscard]] virtual f32 width() const { return 0.6f; }

    /**
     * @brief 获取实体高度
     * @return 实体高度（方块单位）
     */
    [[nodiscard]] virtual f32 height() const { return 1.8f; }

    /**
     * @brief 获取眼睛高度
     * @return 眼睛距离脚底的高度（方块单位）
     */
    [[nodiscard]] virtual f32 eyeHeight() const { return 1.62f; }

    /**
     * @brief 获取步进高度
     * @return 实体可以自动步进的最大高度（玩家为0.6）
     */
    [[nodiscard]] virtual f32 stepHeight() const { return 0.0f; }

    // ========== 碰撞箱 ==========

    /**
     * @brief 获取实体碰撞箱
     * @return 基于当前位置的AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB boundingBox() const {
        return AxisAlignedBB::fromPosition(m_position, width(), height());
    }

    // ========== 更新 ==========

    virtual void tick();
    virtual void update();

    // ========== 移动 ==========

    /**
     * @brief 直接移动实体（无碰撞检测）
     * @param dx, dy, dz 移动增量
     */
    void move(f32 dx, f32 dy, f32 dz);

    /**
     * @brief 旋转实体
     * @param deltaYaw 偏航角增量（度）
     * @param deltaPitch 俯仰角增量（度）
     */
    void rotate(f32 deltaYaw, f32 deltaPitch);

    // ========== 物理 ==========

    /**
     * @brief 设置物理引擎
     * @param engine 物理引擎指针（不拥有所有权）
     */
    void setPhysicsEngine(PhysicsEngine* engine) { m_physicsEngine = engine; }

    /**
     * @brief 获取物理引擎
     */
    [[nodiscard]] PhysicsEngine* physicsEngine() const { return m_physicsEngine; }

    /**
     * @brief 带碰撞检测的移动
     *
     * 使用物理引擎进行带碰撞检测的移动。
     * 会更新 m_onGround、m_collidedHorizontally、m_collidedVertically 状态。
     *
     * @param dx, dy, dz 期望移动增量
     * @return 实际移动增量
     */
    Vector3 moveWithCollision(f32 dx, f32 dy, f32 dz);

    /**
     * @brief 应用物理效果（重力、阻力）
     *
     * 更新速度：应用重力和空气阻力。
     * 如果在地面，重置Y速度为0。
     *
     * @param deltaTime 时间增量（秒）
     */
    void applyPhysics(f32 deltaTime);

    // ========== 碰撞状态 ==========

    [[nodiscard]] bool collidedHorizontally() const { return m_collidedHorizontally; }
    [[nodiscard]] bool collidedVertically() const { return m_collidedVertically; }
    [[nodiscard]] f32 fallDistance() const { return m_fallDistance; }

    // ========== 移除 ==========

    void remove() { m_removed = true; }

    // ========== 维度 ==========

    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    // ========== 存活时间 ==========

    [[nodiscard]] u32 ticksExisted() const { return m_ticksExisted; }

protected:
    EntityId m_id;
    EntityType m_type;
    String m_uuid;              // UUID 字符串
    Vector3 m_position;         // 当前位置
    Vector3 m_prevPosition;     // 上一帧位置
    Vector3 m_velocity;         // 速度

    f32 m_yaw = 0.0f;           // 偏航角 (Y轴旋转)
    f32 m_pitch = 0.0f;         // 俯仰角 (X轴旋转)
    f32 m_prevYaw = 0.0f;
    f32 m_prevPitch = 0.0f;

    bool m_onGround = false;
    bool m_removed = false;
    EntityPose m_pose = EntityPose::Standing;
    EntityFlags m_flags = EntityFlags::None;

    // 物理相关
    PhysicsEngine* m_physicsEngine = nullptr;
    bool m_collidedHorizontally = false;
    bool m_collidedVertically = false;
    f32 m_fallDistance = 0.0f;

    DimensionId m_dimension = 0;
    u32 m_ticksExisted = 0;
};

} // namespace mr
