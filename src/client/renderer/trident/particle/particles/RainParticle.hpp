#pragma once

#include "../Particle.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 雨滴粒子
 *
 * 参考 MC 1.16.5 RainParticle
 *
 * 特性：
 * - 快速下落
 * - 碰撞地面时消失或产生溅射效果
 * - 雨滴大小和速度随机变化
 */
class RainParticle : public Particle {
public:
    /**
     * @brief 构造雨滴粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    RainParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建雨滴粒子
     *
     * @param pos 位置
     * @param velocity 速度
     * @return 雨滴粒子实例
     */
    static std::unique_ptr<Particle> create(const glm::vec3& pos, const glm::vec3& velocity);

    void tick() override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::Translucent;
    }

private:
    static constexpr f32 DEFAULT_GRAVITY = 0.06f;   ///< 雨滴重力
    static constexpr f32 DEFAULT_SIZE = 0.01f;      ///< 雨滴大小
    static constexpr f32 TERMINAL_VELOCITY = -3.0f; ///< 终端速度
};

} // namespace mc::client::renderer::trident::particle::particles
