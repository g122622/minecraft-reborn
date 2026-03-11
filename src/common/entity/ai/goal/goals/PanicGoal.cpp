#include "PanicGoal.hpp"
#include "../../../mob/CreatureEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../Entity.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../math/random/Random.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

PanicGoal::PanicGoal(CreatureEntity* creature, f64 speed)
    : m_creature(creature)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool PanicGoal::shouldExecute() {
    if (!m_creature) return false;

    // 检查是否受到攻击或着火
    LivingEntity* revengeTarget = m_creature->attackTarget();
    bool isBurning = m_creature->isOnFire();

    if (revengeTarget == nullptr && !isBurning) {
        return false;
    }

    // 如果着火，尝试找水
    if (isBurning) {
        Vector3 waterPos = getRandomWaterPosition(5, 4);
        if (waterPos.x != 0.0f || waterPos.y != 0.0f || waterPos.z != 0.0f) {
            m_targetX = waterPos.x;
            m_targetY = waterPos.y;
            m_targetZ = waterPos.z;
            return true;
        }
    }

    // 否则随机逃跑
    return findRandomPosition();
}

bool PanicGoal::shouldContinueExecuting() {
    if (!m_creature) return false;

    // 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return true;
}

void PanicGoal::startExecuting() {
    if (m_creature) {
        m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        m_running = true;
    }
}

void PanicGoal::resetTask() {
    m_running = false;
    if (m_creature) {
        auto* nav = m_creature->navigator();
        if (nav) {
            nav->clearPath();
        }
    }
}

void PanicGoal::tick() {
    // 持续移动到目标位置
    if (m_creature) {
        // 如果距离目标较远，重新设置路径
        f32 dx = m_targetX - m_creature->x();
        f32 dy = m_targetY - m_creature->y();
        f32 dz = m_targetZ - m_creature->z();
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq > 4.0f) {
            m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        }
    }
}

bool PanicGoal::findRandomPosition() {
    if (!m_creature) return false;

    // 使用简单的随机位置生成
    math::Random rng = m_creature->getRandom();

    // 生成远离当前位置的随机方向
    f32 angle = rng.nextFloat() * 2.0f * 3.14159265f;
    f32 distance = 5.0f + rng.nextFloat() * 5.0f; // 5-10 格距离

    m_targetX = m_creature->x() + std::cos(angle) * distance;
    m_targetZ = m_creature->z() + std::sin(angle) * distance;

    // Y坐标保持当前位置或稍低
    m_targetY = m_creature->y();

    return true;
}

Vector3 PanicGoal::getRandomWaterPosition(i32 range, i32 verticalRange) {
    if (!m_creature || !m_creature->world()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    IWorld* world = m_creature->world();
    math::Random rng = m_creature->getRandom();

    // 在范围内搜索水源
    // 简化实现：返回一个随机位置
    // TODO: 完整实现需要检查方块是否是水
    i32 cx = static_cast<i32>(m_creature->x());
    i32 cy = static_cast<i32>(m_creature->y());
    i32 cz = static_cast<i32>(m_creature->z());

    // 随机采样几个位置
    for (i32 i = 0; i < 10; ++i) {
        i32 x = cx + rng.nextInt(-range, range);
        i32 y = cy + rng.nextInt(-verticalRange, verticalRange);
        i32 z = cz + rng.nextInt(-range, range);

        const BlockState* state = world->getBlockState(x, y, z);
        if (state) {
            // 暂时返回第一个非空方块上方的位置
            return Vector3(static_cast<f32>(x), static_cast<f32>(y + 1), static_cast<f32>(z));
        }
    }

    return Vector3(0.0f, 0.0f, 0.0f);
}

} // namespace mr::entity::ai::goal
