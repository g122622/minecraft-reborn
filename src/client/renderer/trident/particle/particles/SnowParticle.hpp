#pragma once

#include "../Particle.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 雪花粒子
 *
 * 参考 MC 1.16.5 SnowflakeParticle
 *
 * 特性：
 * - 缓慢飘落
 * - 左右摇摆
 * - 随机大小和旋转
 */
class SnowParticle : public Particle {
public:
    /**
     * @brief 构造雪花粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    SnowParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建雪花粒子
     *
     * @param pos 位置
     * @param velocity 速度
     * @return 雪花粒子实例
     */
    static std::unique_ptr<Particle> create(const glm::vec3& pos, const glm::vec3& velocity);

    void tick() override;

    void buildVertices(const glm::vec3& cameraPos,
                       f32 partialTick,
                       std::vector<ParticleVertex>& outVertices) const override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::Translucent;
    }

private:
    static constexpr f32 DEFAULT_GRAVITY = 0.02f;   ///< 雪花重力（比雨滴小）
    static constexpr f32 SWING_AMPLITUDE = 0.1f;    ///< 摇摆振幅
    static constexpr f32 SWING_FREQUENCY = 0.05f;   ///< 摇摆频率

    f32 m_swingPhase;      ///< 摇摆相位（随机初始值）
    f32 m_swingAmplitude;  ///< 摇摆振幅（个体差异）
};

} // namespace mc::client::renderer::trident::particle::particles
