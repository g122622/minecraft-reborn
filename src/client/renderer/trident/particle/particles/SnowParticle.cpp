#include "SnowParticle.hpp"
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

SnowParticle::SnowParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_swingPhase(randomFloat() * 6.28318f)  // 0 - 2π
    , m_swingAmplitude(SWING_AMPLITUDE * (0.5f + randomFloat()))
{
    // 雪花参数
    setGravity(DEFAULT_GRAVITY);
    setSize(0.05f + randomFloat() * 0.05f);  // 0.05 - 0.1
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));  // 白色几乎不透明

    // 雪花生命周期较长
    // 参考 MC: maxAge = (int)(200.0F / (Math.random() * 0.2F + 0.8F))
    f32 lifeMultiplier = 0.8f + randomFloat() * 0.2f;
    setMaxAge(200.0f / lifeMultiplier);

    // 设置阻力
    m_drag = 0.95f;
}

std::unique_ptr<Particle> SnowParticle::create(const glm::vec3& pos, const glm::vec3& velocity) {
    return std::make_unique<SnowParticle>(pos, velocity);
}

void SnowParticle::tick() {
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

    // 雪花摇摆效果
    m_swingPhase += SWING_FREQUENCY;
    f32 swing = std::sin(m_swingPhase) * m_swingAmplitude;
    m_velocity.x += swing * 0.01f;

    // 应用速度
    m_position += m_velocity;

    // 应用阻力
    m_velocity.x *= m_drag;
    m_velocity.z *= m_drag;

    // 根据年龄淡出
    if (m_age > m_maxAge * 0.8f) {
        f32 fadeProgress = (m_age - m_maxAge * 0.8f) / (m_maxAge * 0.2f);
        m_color.a = 0.9f * (1.0f - fadeProgress);
    }
}

void SnowParticle::buildVertices(const glm::vec3& cameraPos,
                                  f32 partialTick,
                                  std::vector<ParticleVertex>& outVertices) const {
    // 插值位置
    glm::vec3 interpPos = m_prevPosition + (m_position - m_prevPosition) * partialTick;

    // 计算 billboard 基向量
    glm::vec3 toCamera = cameraPos - interpPos;
    f32 dist = glm::length(toCamera);
    if (dist < 0.001f) {
        return;
    }
    toCamera = glm::normalize(toCamera);

    glm::vec3 right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), toCamera);
    if (glm::length(right) < 0.001f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(right);
    }
    glm::vec3 up = glm::cross(toCamera, right);

    // 雪花稍大一些
    f32 halfSize = m_size * 0.5f;

    // 四个顶点（quad）
    outVertices.push_back({
        interpPos - right * halfSize - up * halfSize,
        glm::vec2(0.0f, 1.0f),
        m_color,
        m_size,
        m_color.a
    });
    outVertices.push_back({
        interpPos + right * halfSize - up * halfSize,
        glm::vec2(1.0f, 1.0f),
        m_color,
        m_size,
        m_color.a
    });
    outVertices.push_back({
        interpPos + right * halfSize + up * halfSize,
        glm::vec2(1.0f, 0.0f),
        m_color,
        m_size,
        m_color.a
    });
    outVertices.push_back({
        interpPos - right * halfSize + up * halfSize,
        glm::vec2(0.0f, 0.0f),
        m_color,
        m_size,
        m_color.a
    });
}

} // namespace mc::client::renderer::trident::particle::particles
