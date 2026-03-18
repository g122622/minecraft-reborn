#include "RainParticle.hpp"
#include <cmath>
#include <random>

namespace mc::client::renderer::trident::particle::particles {

namespace {
    std::mt19937& getRandomEngine() {
        static std::mt19937 engine(42);
        return engine;
    }

    f32 randomFloat() {
        static std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
        return dist(getRandomEngine());
    }
}

RainParticle::RainParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 雨滴参数
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE);
    setColor(glm::vec4(0.7f, 0.8f, 1.0f, 0.6f));  // 淡蓝色半透明

    // 雨滴生命周期较短
    // 参考 MC: maxAge = (int)(8.0D / (Math.random() * 0.8D + 0.2D))
    f32 lifeMultiplier = 0.2f + randomFloat() * 0.8f;
    setMaxAge(8.0f / lifeMultiplier);

    // 设置阻力
    m_drag = 0.98f;
}

std::unique_ptr<Particle> RainParticle::create(const glm::vec3& pos, const glm::vec3& velocity) {
    return std::make_unique<RainParticle>(pos, velocity);
}

void RainParticle::tick() {
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 年龄增加
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity;

    // 限制下落速度
    if (m_velocity.y < TERMINAL_VELOCITY) {
        m_velocity.y = TERMINAL_VELOCITY;
    }

    // 应用速度
    m_position += m_velocity;

    // 应用阻力
    m_velocity.x *= m_drag;
    m_velocity.z *= m_drag;

    // 检查地面碰撞
    // 注意：实际项目中需要与世界进行碰撞检测
    if (m_onGround) {
        // 雨滴碰撞地面时有 50% 概率消失
        if (randomFloat() < 0.5f) {
            setExpired();
        }
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 雨滴不淡出，直接消失
}

} // namespace mc::client::renderer::trident::particle::particles
