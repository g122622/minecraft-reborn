#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>

namespace mc::client::renderer::trident::particle {

/**
 * @brief 粒子顶点数据
 *
 * 用于传递给 GPU 的顶点格式。
 */
struct ParticleVertex {
    glm::vec3 position;     ///< 粒子位置
    glm::vec2 texCoord;     ///< 纹理坐标
    glm::vec4 color;        ///< RGBA 颜色（含透明度）
    f32 size;               ///< 粒子大小
    f32 alpha;              ///< 额外的 alpha 值（用于淡出）
};

/**
 * @brief 粒子渲染类型
 *
 * 决定粒子的渲染方式（透明度、混合模式等）。
 * 参考 MC 1.16.5 IParticleRenderType
 */
enum class ParticleRenderType : u8 {
    Opaque,         ///< 不透明粒子
    Translucent,    ///< 半透明粒子（正常混合）
    Lit,            ///< 发光粒子（不受光照影响）
    Custom          ///< 自定义渲染
};

/**
 * @brief 粒子基类
 *
 * 所有粒子的基类，定义粒子的基本属性和行为。
 * 参考 MC 1.16.5 Particle 类
 */
class Particle {
public:
    Particle(const glm::vec3& pos, const glm::vec3& velocity);
    virtual ~Particle() = default;

    // 禁止拷贝
    Particle(const Particle&) = delete;
    Particle& operator=(const Particle&) = delete;

    // 允许移动
    Particle(Particle&&) noexcept = default;
    Particle& operator=(Particle&&) noexcept = default;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 更新粒子状态
     *
     * 每帧调用，更新粒子位置、速度等。
     */
    virtual void tick();

    /**
     * @brief 粒子是否存活
     */
    [[nodiscard]] bool isAlive() const { return !m_expired; }

    /**
     * @brief 标记粒子为过期
     */
    void setExpired() { m_expired = true; }

    // ========================================================================
    // 渲染
    // ========================================================================

    /**
     * @brief 获取渲染类型
     */
    [[nodiscard]] virtual ParticleRenderType getRenderType() const {
        return ParticleRenderType::Translucent;
    }

    /**
     * @brief 生成渲染顶点数据
     *
     * @param cameraPos 相机位置（用于 billboard 计算）
     * @param partialTick 部分 tick（用于插值）
     * @param outVertices 输出顶点数组（4 个顶点组成一个 quad）
     */
    virtual void buildVertices(const glm::vec3& cameraPos,
                               f32 partialTick,
                               std::vector<ParticleVertex>& outVertices) const;

    // ========================================================================
    // 属性访问
    // ========================================================================

    [[nodiscard]] const glm::vec3& position() const { return m_position; }
    [[nodiscard]] const glm::vec3& prevPosition() const { return m_prevPosition; }
    [[nodiscard]] const glm::vec3& velocity() const { return m_velocity; }
    [[nodiscard]] f32 age() const { return m_age; }
    [[nodiscard]] f32 maxAge() const { return m_maxAge; }
    [[nodiscard]] f32 gravity() const { return m_gravity; }
    [[nodiscard]] f32 size() const { return m_size; }
    [[nodiscard]] const glm::vec4& color() const { return m_color; }

    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setVelocity(const glm::vec3& vel) { m_velocity = vel; }
    void setGravity(f32 g) { m_gravity = g; }
    void setSize(f32 s) { m_size = s; }
    void setColor(const glm::vec4& c) { m_color = c; }
    void setMaxAge(f32 age) { m_maxAge = age; }

protected:
    glm::vec3 m_position;       ///< 当前位置
    glm::vec3 m_prevPosition;   ///< 上一帧位置（用于插值）
    glm::vec3 m_velocity;       ///< 速度
    glm::vec4 m_color = glm::vec4(1.0f);  ///< RGBA 颜色
    f32 m_age = 0.0f;           ///< 已存活时间（ticks）
    f32 m_maxAge = 1.0f;        ///< 最大存活时间（ticks）
    f32 m_gravity = 0.0f;       ///< 重力加速度
    f32 m_size = 0.1f;          ///< 粒子大小
    f32 m_drag = 0.98f;         ///< 空气阻力
    bool m_expired = false;     ///< 是否已过期
    bool m_onGround = false;    ///< 是否在地面
};

/**
 * @brief 粒子工厂函数类型
 */
using ParticleFactory = std::function<std::unique_ptr<Particle>(
    const glm::vec3& pos, const glm::vec3& velocity)>;

} // namespace mc::client::renderer::trident::particle
