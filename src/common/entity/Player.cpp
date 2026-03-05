#include "Player.hpp"
#include "../physics/PhysicsEngine.hpp"
#include <algorithm>
#include <random>
#include <cmath>

namespace mr {

// ============================================================================
// Player 实现
// ============================================================================

Player::Player(EntityId id, const String& username)
    : Entity(EntityType::Player, id)
    , m_username(username)
{
    // 生成随机XP seed
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<i32> dis;
    m_xpSeed = dis(gen);
}

void Player::setGameMode(GameMode mode) {
    m_gameMode = mode;

    // 更新能力
    m_abilities.creativeMode = (mode == GameMode::Creative);
    m_abilities.canFly = (mode == GameMode::Creative || mode == GameMode::Spectator);
    m_abilities.invulnerable = (mode == GameMode::Creative || mode == GameMode::Spectator);
    m_abilities.allowEdit = (mode != GameMode::Adventure && mode != GameMode::Spectator);

    // 创造模式默认飞行
    if (mode == GameMode::Spectator) {
        m_abilities.flying = true;
    } else if (mode != GameMode::Creative) {
        m_abilities.flying = false;
    }
}

void Player::setHealth(f32 health) {
    m_health = std::clamp(health, 0.0f, m_maxHealth);
}

void Player::heal(f32 amount) {
    if (amount <= 0.0f || isDead()) return;
    setHealth(m_health + amount);
}

void Player::damage(f32 amount) {
    if (m_abilities.invulnerable || amount <= 0.0f) return;
    setHealth(m_health - amount);

    if (m_health <= 0.0f) {
        // 玩家死亡
        m_health = 0.0f;
        deathTime = 0;
    } else {
        hurtTime = 10;
    }
}

void Player::addExperience(i32 amount) {
    m_totalExperience += amount;

    while (amount > 0) {
        i32 capacity = experienceBarCapacity();
        f32 needed = static_cast<f32>(capacity) - m_experienceProgress * capacity;

        if (amount >= static_cast<i32>(needed)) {
            amount -= static_cast<i32>(needed);
            m_experienceLevel++;
            m_experienceProgress = 0.0f;
        } else {
            m_experienceProgress += static_cast<f32>(amount) / capacity;
            amount = 0;
        }
    }
}

void Player::setExperienceLevel(i32 level) {
    m_experienceLevel = std::max(0, level);
    m_experienceProgress = 0.0f;
}

i32 Player::experienceBarCapacity() const {
    // 经验条容量公式
    if (m_experienceLevel < 15) {
        return 7 + m_experienceLevel * 2;
    } else if (m_experienceLevel < 30) {
        return 37 + (m_experienceLevel - 15) * 5;
    } else {
        return 112 + (m_experienceLevel - 30) * 9;
    }
}

void Player::setSprinting(bool sprinting) {
    m_isSprinting = sprinting;
    if (sprinting) {
        addFlag(EntityFlags::Sprinting);
    } else {
        removeFlag(EntityFlags::Sprinting);
    }
}

void Player::setSneaking(bool sneaking) {
    m_isSneaking = sneaking;
    if (sneaking) {
        addFlag(EntityFlags::Crouching);
        setPose(EntityPose::Crouching);
    } else {
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
    }
}

void Player::setSwimming(bool swimming) {
    m_isSwimming = swimming;
    if (swimming) {
        addFlag(EntityFlags::Swimming);
        setPose(EntityPose::Swimming);
    } else {
        removeFlag(EntityFlags::Swimming);
        setPose(EntityPose::Standing);
    }
}

void Player::setSleeping(bool sleeping) {
    m_isSleeping = sleeping;
    if (sleeping) {
        setPose(EntityPose::Sleeping);
    } else {
        setPose(EntityPose::Standing);
    }
}

f32 Player::height() const {
    switch (m_pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return PLAYER_SWIM_HEIGHT;
        case EntityPose::Crouching:
            return PLAYER_CROUCH_HEIGHT;
        default:
            return PLAYER_HEIGHT;
    }
}

f32 Player::eyeHeight() const {
    switch (m_pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return 0.4f;
        case EntityPose::Crouching:
            return 1.27f;
        default:
            return PLAYER_EYE_HEIGHT;
    }
}

void Player::tick() {
    Entity::tick();

    // 更新受伤/死亡计时器
    if (hurtTime > 0) {
        hurtTime--;
    }

    if (isDead()) {
        deathTime++;
    }

    // 睡眠计时器
    if (m_isSleeping) {
        sleepTimer++;
    } else {
        sleepTimer = 0;
    }

    // 饥饿系统
    if (m_gameMode == GameMode::Survival && !isDead()) {
        // 简化的饥饿消耗
        if (m_foodStats.exhaustionLevel >= 4.0f) {
            m_foodStats.addExhaustion(0.0f); // 触发消耗
        }
    }
}

void Player::update() {
    Entity::update();
}

void Player::handleMovementInput(f32 forward, f32 strafe, bool jumping, bool sneaking) {
    // 计算移动速度
    f32 speed = m_abilities.walkSpeed;
    if (m_isSprinting) {
        speed *= 1.3f; // 冲刺速度倍率
    }
    if (sneaking && !m_abilities.flying) {
        speed *= 0.3f; // 潜行速度倍率
    }
    if (m_abilities.flying) {
        speed = m_abilities.flySpeed * 2.0f; // 飞行速度
    }

    // 如果没有输入，不移动
    if (forward == 0.0f && strafe == 0.0f) {
        // 停止水平移动
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
        return;
    }

    // 根据朝向计算移动方向
    f32 yawRad = static_cast<f32>(m_yaw * 3.14159265358979323846 / 180.0);

    // 计算移动向量（相对于玩家朝向）
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    // forward: Z方向，strafe: X方向
    f32 moveX = strafe * cosYaw - forward * sinYaw;
    f32 moveZ = strafe * sinYaw + forward * cosYaw;

    // 归一化并应用速度
    f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (length > 0.0f) {
        moveX = moveX / length * speed;
        moveZ = moveZ / length * speed;
    }

    m_velocity.x = moveX;
    m_velocity.z = moveZ;

    // 处理跳跃
    if (jumping) {
        if (m_abilities.flying) {
            // 飞行模式下向上移动
            m_velocity.y = m_abilities.flySpeed;
        } else if (m_onGround) {
            jump();
        }
    } else if (m_abilities.flying) {
        // 飞行模式下不自动下降
        m_velocity.y *= 0.6f; // 阻力
    }
}

void Player::jump() {
    if (m_onGround) {
        m_velocity.y = PhysicsEngine::JUMP_VELOCITY;
        m_onGround = false;
    }
}

void Player::updatePhysics() {
    // 如果在飞行模式，不应用重力
    if (!m_abilities.flying) {
        // 应用重力
        if (!m_onGround) {
            m_velocity.y -= PhysicsEngine::GRAVITY;
        }
    }

    // 应用阻力
    m_velocity.x *= PhysicsEngine::DRAG;
    m_velocity.y *= PhysicsEngine::DRAG;
    m_velocity.z *= PhysicsEngine::DRAG;

    // 如果在地面，停止Y方向速度
    if (m_onGround && m_velocity.y < 0.0f) {
        m_velocity.y = 0.0f;
    }

    // 使用碰撞检测移动
    if (m_physicsEngine && (m_velocity.x != 0.0f || m_velocity.y != 0.0f || m_velocity.z != 0.0f)) {
        Vector3 actualMovement = moveWithCollision(
            static_cast<f64>(m_velocity.x),
            static_cast<f64>(m_velocity.y),
            static_cast<f64>(m_velocity.z)
        );

        // 如果碰撞停止了移动，重置速度
        if (m_collidedHorizontally) {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }
        if (m_collidedVertically) {
            m_velocity.y = 0.0f;
        }
    }
}

void Player::applyMovementSpeed(f32& speed, bool sneaking) const {
    if (m_abilities.flying) {
        speed = m_abilities.flySpeed;
    } else {
        speed = m_abilities.walkSpeed;
        if (m_isSprinting) {
            speed *= 1.3f;
        }
        if (sneaking) {
            speed *= 0.3f;
        }
    }
}

network::PlayerPosition Player::playerPosition() const {
    return network::PlayerPosition(
        static_cast<f64>(m_position.x),
        static_cast<f64>(m_position.y),
        static_cast<f64>(m_position.z),
        m_yaw,
        m_pitch,
        m_onGround
    );
}

void Player::respawn() {
    m_health = m_maxHealth;
    m_foodStats.foodLevel = 20;
    m_foodStats.saturationLevel = 5.0f;
    m_foodStats.exhaustionLevel = 0.0f;
    deathTime = 0;
    hurtTime = 0;
    m_isSleeping = false;
    sleepTimer = 0;
    setPose(EntityPose::Standing);
}

void Player::serialize(network::PacketSerializer& ser) const {
    // 基本信息
    ser.writeU64(m_playerId);
    ser.writeString(m_username);

    // 位置
    ser.writeF64(static_cast<f64>(m_position.x));
    ser.writeF64(static_cast<f64>(m_position.y));
    ser.writeF64(static_cast<f64>(m_position.z));
    ser.writeF32(m_yaw);
    ser.writeF32(m_pitch);

    // 状态
    ser.writeF32(m_health);
    ser.writeI32(static_cast<i32>(m_gameMode));
    ser.writeBool(m_onGround);
    ser.writeBool(m_isSprinting);
    ser.writeBool(m_isSneaking);

    // 饥饿
    m_foodStats.serialize(ser);

    // 经验
    ser.writeI32(m_experienceLevel);
    ser.writeF32(m_experienceProgress);
    ser.writeI32(m_totalExperience);
}

Result<std::unique_ptr<Player>> Player::deserialize(network::PacketDeserializer& deser) {
    // 读取基本信息
    auto idResult = deser.readU64();
    if (idResult.failed()) return idResult.error();
    PlayerId playerId = idResult.value();

    auto usernameResult = deser.readString();
    if (usernameResult.failed()) return usernameResult.error();
    String username = usernameResult.value();

    auto player = std::make_unique<Player>(static_cast<EntityId>(playerId), username);
    player->m_playerId = playerId;

    // 位置
    auto xResult = deser.readF64();
    if (xResult.failed()) return xResult.error();

    auto yResult = deser.readF64();
    if (yResult.failed()) return yResult.error();

    auto zResult = deser.readF64();
    if (zResult.failed()) return zResult.error();

    player->setPosition(xResult.value(), yResult.value(), zResult.value());

    auto yawResult = deser.readF32();
    if (yawResult.failed()) return yawResult.error();
    player->m_yaw = yawResult.value();

    auto pitchResult = deser.readF32();
    if (pitchResult.failed()) return pitchResult.error();
    player->m_pitch = pitchResult.value();

    // 状态
    auto healthResult = deser.readF32();
    if (healthResult.failed()) return healthResult.error();
    player->m_health = healthResult.value();

    auto gameModeResult = deser.readI32();
    if (gameModeResult.failed()) return gameModeResult.error();
    player->m_gameMode = static_cast<GameMode>(gameModeResult.value());

    auto groundResult = deser.readBool();
    if (groundResult.failed()) return groundResult.error();
    player->m_onGround = groundResult.value();

    auto sprintResult = deser.readBool();
    if (sprintResult.failed()) return sprintResult.error();
    player->m_isSprinting = sprintResult.value();

    auto sneakResult = deser.readBool();
    if (sneakResult.failed()) return sneakResult.error();
    player->m_isSneaking = sneakResult.value();

    // 饥饿
    auto foodResult = FoodStats::deserialize(deser);
    if (foodResult.failed()) return foodResult.error();
    player->m_foodStats = foodResult.value();

    // 经验
    auto levelResult = deser.readI32();
    if (levelResult.failed()) return levelResult.error();
    player->m_experienceLevel = levelResult.value();

    auto progressResult = deser.readF32();
    if (progressResult.failed()) return progressResult.error();
    player->m_experienceProgress = progressResult.value();

    auto totalResult = deser.readI32();
    if (totalResult.failed()) return totalResult.error();
    player->m_totalExperience = totalResult.value();

    return player;
}

} // namespace mr
