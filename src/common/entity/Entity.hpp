#pragma once

#include "../core/Types.hpp"
#include "../core/Result.hpp"
#include "../math/Vector3.hpp"
#include <string>
#include <memory>
#include <array>

namespace mr {

// ============================================================================
// 实体类型枚举
// ============================================================================

enum class EntityType : u32 {
    Unknown = 0,
    Player = 1,
    // 后续添加: Item, Mob, Projectile 等
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
// 实体碰撞箱
// ============================================================================

struct BoundingBox {
    f32 minX = 0.0f;
    f32 minY = 0.0f;
    f32 minZ = 0.0f;
    f32 maxX = 0.0f;
    f32 maxY = 0.0f;
    f32 maxZ = 0.0f;

    BoundingBox() = default;
    BoundingBox(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2)
        : minX(std::min(x1, x2)), minY(std::min(y1, y2)), minZ(std::min(z1, z2))
        , maxX(std::max(x1, x2)), maxY(std::max(y1, y2)), maxZ(std::max(z1, z2))
    {}

    static BoundingBox fromPosition(const Vector3& pos, f32 width, f32 height) {
        f32 hw = width / 2.0f;
        return BoundingBox(
            pos.x - hw, pos.y, pos.z - hw,
            pos.x + hw, pos.y + height, pos.z + hw
        );
    }

    [[nodiscard]] f32 width() const { return maxX - minX; }
    [[nodiscard]] f32 height() const { return maxY - minY; }
    [[nodiscard]] f32 depth() const { return maxZ - minZ; }

    [[nodiscard]] Vector3 center() const {
        return Vector3(
            (minX + maxX) / 2.0f,
            (minY + maxY) / 2.0f,
            (minZ + maxZ) / 2.0f
        );
    }

    [[nodiscard]] bool intersects(const BoundingBox& other) const {
        return minX < other.maxX && maxX > other.minX &&
               minY < other.maxY && maxY > other.minY &&
               minZ < other.maxZ && maxZ > other.minZ;
    }

    [[nodiscard]] bool contains(const Vector3& point) const {
        return point.x >= minX && point.x <= maxX &&
               point.y >= minY && point.y <= maxY &&
               point.z >= minZ && point.z <= maxZ;
    }

    void offset(f32 dx, f32 dy, f32 dz) {
        minX += dx; maxX += dx;
        minY += dy; maxY += dy;
        minZ += dz; maxZ += dz;
    }

    void expand(f32 dx, f32 dy, f32 dz) {
        minX -= dx; maxX += dx;
        minY -= dy; maxY += dy;
        minZ -= dz; maxZ += dz;
    }
};

// ============================================================================
// 实体基类
// ============================================================================

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

    // 基本属性
    [[nodiscard]] EntityId id() const { return m_id; }
    [[nodiscard]] EntityType type() const { return m_type; }
    [[nodiscard]] const String& uuid() const { return m_uuid; }
    void setUuid(const String& uuid) { m_uuid = uuid; }

    // 位置
    [[nodiscard]] Vector3 position() const { return m_position; }
    [[nodiscard]] f64 x() const { return m_position.x; }
    [[nodiscard]] f64 y() const { return m_position.y; }
    [[nodiscard]] f64 z() const { return m_position.z; }

    // 前一帧位置
    [[nodiscard]] Vector3 prevPosition() const { return m_prevPosition; }
    [[nodiscard]] f64 prevX() const { return m_prevPosition.x; }
    [[nodiscard]] f64 prevY() const { return m_prevPosition.y; }
    [[nodiscard]] f64 prevZ() const { return m_prevPosition.z; }

    // 旋转
    [[nodiscard]] f32 yaw() const { return m_yaw; }
    [[nodiscard]] f32 pitch() const { return m_pitch; }
    [[nodiscard]] f32 prevYaw() const { return m_prevYaw; }
    [[nodiscard]] f32 prevPitch() const { return m_prevPitch; }

    // 速度
    [[nodiscard]] Vector3 velocity() const { return m_velocity; }
    [[nodiscard]] f64 velocityX() const { return m_velocity.x; }
    [[nodiscard]] f64 velocityY() const { return m_velocity.y; }
    [[nodiscard]] f64 velocityZ() const { return m_velocity.z; }

    // 状态
    [[nodiscard]] bool onGround() const { return m_onGround; }
    [[nodiscard]] bool isRemoved() const { return m_removed; }
    [[nodiscard]] EntityPose pose() const { return m_pose; }
    [[nodiscard]] EntityFlags flags() const { return m_flags; }

    // 设置属性
    void setPosition(f64 x, f64 y, f64 z);
    void setPosition(const Vector3& pos) { setPosition(pos.x, pos.y, pos.z); }
    void setRotation(f32 yaw, f32 pitch);
    void setVelocity(f64 x, f64 y, f64 z);
    void setVelocity(const Vector3& vel) { setVelocity(vel.x, vel.y, vel.z); }
    void setOnGround(bool onGround) { m_onGround = onGround; }
    void setPose(EntityPose pose) { m_pose = pose; }
    void setFlags(EntityFlags flags) { m_flags = flags; }

    // 添加标志
    void addFlag(EntityFlags flag) { m_flags = m_flags | flag; }
    void removeFlag(EntityFlags flag) {
        m_flags = static_cast<EntityFlags>(static_cast<u8>(m_flags) & ~static_cast<u8>(flag));
    }
    [[nodiscard]] bool hasFlag(EntityFlags flag) const {
        return mr::hasFlag(m_flags, flag);
    }

    // 碰撞箱
    [[nodiscard]] BoundingBox boundingBox() const;
    [[nodiscard]] virtual f32 width() const { return 0.6f; }
    [[nodiscard]] virtual f32 height() const { return 1.8f; }
    [[nodiscard]] virtual f32 eyeHeight() const { return 1.62f; }

    // 更新
    virtual void tick();
    virtual void update();

    // 移动
    void move(f64 dx, f64 dy, f64 dz);
    void rotate(f32 deltaYaw, f32 deltaPitch);

    // 移除
    void remove() { m_removed = true; }

    // 维度
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    // 存活时间
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

    DimensionId m_dimension = 0;
    u32 m_ticksExisted = 0;
};

} // namespace mr
