#include "Particle.hpp"
#include "../../../../common/math/MathUtils.hpp"

namespace mc::client::renderer::trident::particle {

Particle::Particle(const glm::vec3& pos, const glm::vec3& velocity)
    : m_position(pos)
    , m_prevPosition(pos)
    , m_velocity(velocity)
{
}

void Particle::tick() {
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 年龄增加
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;  // MC 的重力系数

    // 应用速度
    m_position += m_velocity;

    // 应用空气阻力
    m_velocity *= m_drag;

    // 根据年龄淡出
    if (m_age > m_maxAge * 0.5f) {
        f32 fadeProgress = (m_age - m_maxAge * 0.5f) / (m_maxAge * 0.5f);
        m_color.a = 1.0f - fadeProgress;
    }
}

void Particle::buildVertices(const glm::vec3& cameraPos,
                             f32 partialTick,
                             std::vector<ParticleVertex>& outVertices) const {
    // 插值位置
    glm::vec3 interpPos = m_prevPosition + (m_position - m_prevPosition) * partialTick;

    // 计算 billboard 基向量
    // 相机到粒子的方向
    glm::vec3 toCamera = cameraPos - interpPos;
    f32 dist = glm::length(toCamera);
    if (dist < 0.001f) {
        return;  // 太近了，跳过
    }
    toCamera = glm::normalize(toCamera);

    // 计算右向量和上向量（camera-facing billboard）
    glm::vec3 right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), toCamera);
    if (glm::length(right) < 0.001f) {
        // 相机在正上方或正下方
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(right);
    }
    glm::vec3 up = glm::cross(toCamera, right);

    // 半尺寸
    f32 halfSize = m_size * 0.5f;

    // 四个顶点（quad）
    // 左下
    outVertices.push_back({
        interpPos - right * halfSize - up * halfSize,
        glm::vec2(0.0f, 1.0f),
        m_color,
        m_size,
        m_color.a
    });
    // 右下
    outVertices.push_back({
        interpPos + right * halfSize - up * halfSize,
        glm::vec2(1.0f, 1.0f),
        m_color,
        m_size,
        m_color.a
    });
    // 右上
    outVertices.push_back({
        interpPos + right * halfSize + up * halfSize,
        glm::vec2(1.0f, 0.0f),
        m_color,
        m_size,
        m_color.a
    });
    // 左上
    outVertices.push_back({
        interpPos - right * halfSize + up * halfSize,
        glm::vec2(0.0f, 0.0f),
        m_color,
        m_size,
        m_color.a
    });
}

} // namespace mc::client::renderer::trident::particle
